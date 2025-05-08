#include "pch.h"
#include "CThreadTimer.h"
#include "CTimer.h"
#include "CTime.h"
#include "CTaskScheduler.h"

CThreadTimer CThreadTimer::__mSingleton;
CThreadTimer::TimerHandler CThreadTimer::__mTimerHandler[ON_TIMER_COUNT] = { NULL, };
CThreadTimer::TaskHandler CThreadTimer::__mTaskHandler[ON_TASK_COUNT] = { NULL, };

unsigned int __stdcall CThreadTimer::ExecuteTimerLoopThread(VOID* pArg)
{
	CThreadTimer* aThreadTimer = static_cast<CThreadTimer*>(pArg);
	if (aThreadTimer)
		return aThreadTimer->TimerLoopWorker();

	return 1;
}

unsigned int __stdcall CThreadTimer::ExecuteTimerHandlerThread(VOID* pArg)
{
	CThreadTimer* aThreadTimer = static_cast<CThreadTimer*>(pArg);
	if (aThreadTimer)
		return aThreadTimer->TimerHandlerWorker();

	return 1;
}

CThreadTimer::CThreadTimer()
{
	__ResetAttr();
}

CThreadTimer::~CThreadTimer()
{
	__Dtor();
}

CThreadTimer& CThreadTimer::This()
{
	return __mSingleton;
}

VOID CThreadTimer::__ResetAttr()
{
	__mHndTimerLoop = NULL;
	__mHndTimerHandler = NULL;
	__mTimer.Open();
	__mIocp.Open();
	__mExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	__mTimerLoopThreadID = 0;
	__mTimerHandlerThreadID = 0;
	
	__SetTimerHandler();
	__SetTaskHandler();
}

VOID CThreadTimer::__Dtor()
{
	__mTimer.Close();
	__mIocp.Close();

	if (NULL != __mExitEvent)
	{
		CloseHandle(__mExitEvent);
		__mExitEvent = NULL;
	}

	if (NULL != __mHndTimerLoop)
	{
		CloseHandle(__mHndTimerLoop);
		__mHndTimerLoop = NULL;
	}
	
	if (NULL != __mHndTimerHandler)
	{
		CloseHandle(__mHndTimerHandler);
		__mHndTimerHandler = NULL;
	}
}

DWORD CThreadTimer::__WaitStopThread(HANDLE pHnd, DWORD pTimeOut)
{
	DWORD aRv = 0;
	while (true)
	{
		aRv = WaitForSingleObjectEx(pHnd, pTimeOut, TRUE);
		if (WAIT_OBJECT_0 == aRv)
		{
			aRv = 0;
			break;
		}
		else if (WAIT_TIMEOUT == aRv)
		{
			break;
		}
		else if (WAIT_FAILED == aRv)
		{
			aRv = GetLastError();
			assert(0);
			break;
		}
		assert(WAIT_ABANDONED != aRv);
	}

	return aRv;
}

DWORD CThreadTimer::RegisterTimer(DWORD pId, DWORD pWait, DWORD pPeriod)
{
	return __mTimer.Register(pId, pWait, pPeriod);	
}

VOID CThreadTimer::StartThread()
{
	__RegisterTimer();

	__mHndTimerLoop = (HANDLE)_beginthreadex(NULL, 0, ExecuteTimerLoopThread, this, 0, &__mTimerLoopThreadID);
	__mHndTimerHandler = (HANDLE)_beginthreadex(NULL, 0, ExecuteTimerHandlerThread, this, 0, &__mTimerHandlerThreadID);
}

VOID CThreadTimer::StopThread()
{
	SetEvent(__mExitEvent);
	__mIocp.Push(CIocp::eKeyStopWorker, 0, NULL);

	__WaitStopThread(__mHndTimerLoop, 3000);
	__WaitStopThread(__mHndTimerHandler, 3000);
}

DWORD CThreadTimer::PushTask(CTask* pTask)
{
	return __mIocp.Push(CIocp::eKeyTask, 0, reinterpret_cast<COverlap*>(pTask));
}

DWORD CThreadTimer::TimerLoopWorker()
{
	srand(__mTimerLoopThreadID);

	DWORD aRv = 0;
	HANDLE aWaitHnd[2];
	aWaitHnd[0] = __mTimer.GetHnd();	// 타이머 핸들
	aWaitHnd[1] = __mExitEvent;			// 스레드 종료 핸들

	ULONGLONG aTick = GetTickCount64();
	while (true)
	{
		aRv = __mTimer.Begin(aTick);
		if (0 < aRv)
		{
			assert(0);
			continue;
		}

		aRv = WaitForMultipleObjects(2, aWaitHnd, FALSE, INFINITE);
		
		aTick = __mTimer.End();
		if (aRv == WAIT_OBJECT_0)
		{
			while (true)
			{
				aRv = __mTimer.Fetch();
				if (aRv == INFINITE)
					break;

				if (ON_TIMER_TASK_SCHEDULER == aRv)
				{
					CTaskScheduler::This().ProcessTasks();
				}
				else
				{
					aRv = __mIocp.Push(CIocp::eKeyTimer, aRv, NULL);
					if (0 < aRv)
					{
						assert(0);
					}
				}				
			}
		}
		else if (aRv == WAIT_OBJECT_0 + 1)
		{
			break;
		}
	}

	return 0;
}

DWORD CThreadTimer::TimerHandlerWorker()
{
	srand(__mTimerHandlerThreadID);

	COverlap* aOver = NULL;
	DWORD aRv = 0;
	DWORD aKey = 0;
	DWORD aLng = 0;
	bool aRunning = true;

	while (aRunning)
	{
		aKey = CIocp::eKeyIocpError;
		aRv = __mIocp.Pop(aOver, aKey, aLng, INFINITE);
		switch (aKey)
		{
			case CIocp::eKeyStopWorker:
			{
				aRunning = false;
			}
			break;

			case CIocp::eKeyTimer:
			{
				__OnTimerHandler(aLng);
			}
			break;

			case CIocp::eKeyTask:
			{
				CTask* aTask = reinterpret_cast<CTask*>(aOver);
				__OnTaskHandler(aTask->mPayload, aTask->mLng);
				CTaskScheduler::This().UnregisterTask(aTask);
			}
			break;

			case CIocp::eKeyIocpError:
			{
				assert(0);
			}
			break;

			default:
			{
				assert(0);
			}
			break;
		}
	}

	return 0;
}

// 초기 시간에서 특정 분을 더하거나 뺀다.
std::pair<WORD, WORD> CThreadTimer::ShiftTime(WORD pInitHour, WORD pInitMinute, int pOffsetMinutes)
{
	WORD aAdjustedHour = 0;
	WORD aAdjustedMinute = 0;

	// 초기 분에 pOffsetMinutes를 더하거나 뺀 결과
	int aTotalMinutes = pInitMinute + pOffsetMinutes;

	// 분이 음수일 경우 처리
	if (aTotalMinutes < 0)
	{
		aAdjustedHour = pInitHour + (aTotalMinutes / 60); // 시(hour)를 감소
		aAdjustedMinute = (aTotalMinutes % 60 + 60) % 60; // 음수를 양수로 변환

		if (aAdjustedMinute > 0)
		{
			aAdjustedHour -= 1; // 분 단위 계산에서 1시간 초과한 경우 처리
		}
	}
	else // 분이 60 이상일 경우 처리
	{
		aAdjustedHour = pInitHour + (aTotalMinutes / 60);
		aAdjustedMinute = aTotalMinutes % 60;
	}

	// 시간이 24 이상 또는 음수일 경우 처리
	if (aAdjustedHour >= 24)
	{
		aAdjustedHour %= 24; // 24시간제로 조정
	}
	else if (aAdjustedHour < 0)
	{
		aAdjustedHour = (aAdjustedHour + 24) % 24; // 음수 시간을 24시간제로 변환
	}

	return { aAdjustedHour, aAdjustedMinute };
}

const char* CThreadTimer::__TimerIDToString(DWORD pId)
{
	switch (pId)
	{
		ENUM_TO_STRING(ETimerID, ON_TIMER_TASK_SCHEDULER)
		ENUM_TO_STRING(ETimerID, ON_TIMER_EVERY_HOUR)
		ENUM_TO_STRING(ETimerID, ON_TIMER_EVERY_DAY)
		default: return "Unknown";
	}
}

VOID CThreadTimer::__SetTimerHandler()
{
	for (int i = 0; i < sizeof(__mTimerHandler) / sizeof(__mTimerHandler[0]); i++)
	{
		__mTimerHandler[i] = &CThreadTimer::__OnTimerDefault;
	}

	__mTimerHandler[ON_TIMER_EVERY_HOUR] = &CThreadTimer::__OnTimerEveryHour;
	__mTimerHandler[ON_TIMER_EVERY_DAY] = &CThreadTimer::__OnTimerEveryDay;
}

VOID CThreadTimer::__RegisterTimer()
{
	CTime aTime;

	__mTimer.Register(ON_TIMER_TASK_SCHEDULER, TASK_PROCESS_INTERVAL, TASK_PROCESS_INTERVAL);
	__mTimer.Register(ON_TIMER_EVERY_HOUR, aTime.GetWaitTm(0, 0), ePeriodEveryHour);
	__mTimer.Register(ON_TIMER_EVERY_DAY, aTime.GetWaitTm(0, 1, 0), ePeriodEveryDay);

	for (DWORD aId = ON_TIMER_DEFAULT + 1; aId < ON_TIMER_COUNT; ++aId)
	{
		__LogTimerDetails(aId);		
	}
}

VOID CThreadTimer::__LogTimerDetails(DWORD pId)
{
	
}

VOID CThreadTimer::__LogTimerStatus(DWORD pId, const char* pTimerID, const char* pFunctionName, const char* pStatus)
{
	
}

VOID CThreadTimer::__OnTimerHandler(DWORD pId)
{
	assert((0 <= pId) && (pId < ON_TIMER_COUNT));

	(this->*__mTimerHandler[pId])(pId);

	__LogTimerDetails(pId);
}

VOID CThreadTimer::__OnTimerDefault(DWORD pId)
{
	assert(0);
	
	__LogTimerStatus(pId, __TimerIDToString(pId), __FUNCTION__, "Default Timer Handler Invoked");
}

VOID CThreadTimer::__SetTaskHandler()
{
	for (int i = 0; i < sizeof(__mTaskHandler) / sizeof(__mTaskHandler[0]); i++)
	{
		__mTaskHandler[i] = &CThreadTimer::__OnTaskDefault;
	}	
}

VOID CThreadTimer::__OnTaskDefault(const void* pPayload, DWORD pLng)
{
	assert(0);
}

VOID CThreadTimer::__OnTimerEveryHour(DWORD pId)
{

}

VOID CThreadTimer::__OnTimerEveryDay(DWORD pId)
{

}

VOID CThreadTimer::__OnTaskHandler(const void* pPayload, DWORD pLng)
{
	const OnParentMsg* aOnMsg = static_cast<const OnParentMsg*>(pPayload);
	(this->*__mTaskHandler[aOnMsg->mMsgType])(pPayload, pLng);
}