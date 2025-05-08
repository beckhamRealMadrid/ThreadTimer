#include "pch.h"
#include "CTaskScheduler.h"
#include "CThreadTimer.h"

CTaskScheduler	CTaskScheduler::__mSingleton;
CTaskScheduler::CTaskScheduler()
{
	__ResetAttr();
}

CTaskScheduler::~CTaskScheduler()
{
	__Dtor();
}

CTaskScheduler&	CTaskScheduler::This()
{
	return __mSingleton;
}

VOID CTaskScheduler::__ResetAttr()
{
	__mSchedulerTick = 0;
	__mCurSlotIdx = 0;
}

VOID CTaskScheduler::__Dtor()
{

}

DWORD CTaskScheduler::Open()
{
	DWORD aRv = 0;

	aRv = __mTaskSlotLocker.Open(TRUE);
	if (0 < aRv)
	{
		return aRv;
	}

	aRv = __mFlexibleTaskLocker.Open(TRUE);
	if (0 < aRv)
	{
		return aRv;
	}

	return 0;
}

VOID CTaskScheduler::Close()
{
	for (int i = 0; i < TASK_SLOT_COUNT; ++i)
	{
		for (CTask* aTask : __mFixTask[i])
		{
			__DeallocateTask(aTask);
		}

		__mFixTask[i].clear();
	}
	__mTaskSlotLocker.Close();

	while (!__mFlexibleTask.empty())
	{
		CTask* aTask = __mFlexibleTask.top();
		__mFlexibleTask.pop();
		__DeallocateTask(aTask);
	}
	__mFlexibleTaskLocker.Close();

	__Dtor();
	__ResetAttr();
}

CTask* CTaskScheduler::__AllocateTask()
{
	return __mTaskPool.Allocate();
}

VOID CTaskScheduler::__DeallocateTask(CTask* pTask)
{
	if (pTask)
	{
		__mTaskPool.Deallocate(pTask);
	}
}

DWORD CTaskScheduler::RegisterTask(DWORD pAfter, const void* pPayload, DWORD pLng)
{
	assert(1 <= pAfter);

	// Task 할당
	CTask* aTask = __AllocateTask();
	if (NULL == aTask)
	{
		return ERROR_OUTOFMEMORY;
	}

	aTask->Set(pAfter, pPayload, pLng);

	{
		__TLockerAuto aLocker(__mTaskSlotLocker);
		aTask->mExecuteTick += __mSchedulerTick;	// 현재 Tick을 기준으로 실행 시간 설정
		
		// 실행 시간이 30분 이내이면 __mFixTask에 저장
		if (pAfter < TASK_SLOT_COUNT)
		{
			INT aIdx = (__mCurSlotIdx + pAfter) % TASK_SLOT_COUNT; // 실행할 슬롯 계산
			__mFixTask[aIdx].push_back(aTask);
			return 0;
		}
	}

	// 실행 시간이 30분 이후이면 __mFlexibleTask에 저장
	{
		__TLockerAuto aLocker(__mFlexibleTaskLocker);
		__mFlexibleTask.push(aTask);
	}

	return 0;
}

VOID CTaskScheduler::UnregisterTask(CTask* pTask)
{
	if (NULL == pTask)
	{
		return;
	}

	// Task 해제
	__DeallocateTask(pTask);
}

// __mCurSlotIdx는 0 -> 1 -> 2 -> ... -> 17999 -> 0을 반복하며 30분 주기로 순환
VOID CTaskScheduler::ProcessTasks()
{
	INT	aIdx = -1;
	{
		__TLockerAuto aLocker(__mTaskSlotLocker);
		aIdx = __mCurSlotIdx; // 현재 Tick에서 실행할 Slot 가져오기
		__mCurSlotIdx = (__mCurSlotIdx + 1) % TASK_SLOT_COUNT; // 다음 Slot으로 이동 (순환)
		++__mSchedulerTick; // 현재 Tick 증가 (100ms마다 1 증가)
	}

	// 동기화 코드가 없는 이유: RegisterTask가 ProcessTasks와 서로 다른 스레드에서 동시에 실행되더라도 같은 Slot를 사용하지 않으므로 동시성 문제는 발생하지 않는다.
	DWORD aRv = 0;
	{
		// 현재 Tick(Slot 인덱스)에 해당하는 Task 실행
		for (CTask* aTask : __mFixTask[aIdx])
		{
			aRv = CThreadTimer::This().PushTask(aTask);
			if (0 < aRv)
			{
				UnregisterTask(aTask);
			}
		}

		__mFixTask[aIdx].clear();	// 실행된 Task 삭제
	}

	// 30분 이후 실행할 Task 검사
	CTask* aTask = NULL;
	while (true)
	{
		{
			__TLockerAuto aLocker(__mFlexibleTaskLocker);
			if (__mFlexibleTask.empty())
			{
				break;
			}

			aTask = __mFlexibleTask.top();
			if (__mSchedulerTick <= aTask->mExecuteTick) // 아직 실행 시간이 안 됨
			{
				break;
			}

			__mFlexibleTask.pop(); // 실행 시간이 되면 pop
		}

		aRv = CThreadTimer::This().PushTask(aTask);
		if (0 < aRv)
		{
			UnregisterTask(aTask);
		}
	}
}