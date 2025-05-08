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

	// Task �Ҵ�
	CTask* aTask = __AllocateTask();
	if (NULL == aTask)
	{
		return ERROR_OUTOFMEMORY;
	}

	aTask->Set(pAfter, pPayload, pLng);

	{
		__TLockerAuto aLocker(__mTaskSlotLocker);
		aTask->mExecuteTick += __mSchedulerTick;	// ���� Tick�� �������� ���� �ð� ����
		
		// ���� �ð��� 30�� �̳��̸� __mFixTask�� ����
		if (pAfter < TASK_SLOT_COUNT)
		{
			INT aIdx = (__mCurSlotIdx + pAfter) % TASK_SLOT_COUNT; // ������ ���� ���
			__mFixTask[aIdx].push_back(aTask);
			return 0;
		}
	}

	// ���� �ð��� 30�� �����̸� __mFlexibleTask�� ����
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

	// Task ����
	__DeallocateTask(pTask);
}

// __mCurSlotIdx�� 0 -> 1 -> 2 -> ... -> 17999 -> 0�� �ݺ��ϸ� 30�� �ֱ�� ��ȯ
VOID CTaskScheduler::ProcessTasks()
{
	INT	aIdx = -1;
	{
		__TLockerAuto aLocker(__mTaskSlotLocker);
		aIdx = __mCurSlotIdx; // ���� Tick���� ������ Slot ��������
		__mCurSlotIdx = (__mCurSlotIdx + 1) % TASK_SLOT_COUNT; // ���� Slot���� �̵� (��ȯ)
		++__mSchedulerTick; // ���� Tick ���� (100ms���� 1 ����)
	}

	// ����ȭ �ڵ尡 ���� ����: RegisterTask�� ProcessTasks�� ���� �ٸ� �����忡�� ���ÿ� ����Ǵ��� ���� Slot�� ������� �����Ƿ� ���ü� ������ �߻����� �ʴ´�.
	DWORD aRv = 0;
	{
		// ���� Tick(Slot �ε���)�� �ش��ϴ� Task ����
		for (CTask* aTask : __mFixTask[aIdx])
		{
			aRv = CThreadTimer::This().PushTask(aTask);
			if (0 < aRv)
			{
				UnregisterTask(aTask);
			}
		}

		__mFixTask[aIdx].clear();	// ����� Task ����
	}

	// 30�� ���� ������ Task �˻�
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
			if (__mSchedulerTick <= aTask->mExecuteTick) // ���� ���� �ð��� �� ��
			{
				break;
			}

			__mFlexibleTask.pop(); // ���� �ð��� �Ǹ� pop
		}

		aRv = CThreadTimer::This().PushTask(aTask);
		if (0 < aRv)
		{
			UnregisterTask(aTask);
		}
	}
}