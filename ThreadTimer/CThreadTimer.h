#pragma once

#include "CTimer.h"
#include "CIocp.h"
#include "CTask.h"

/*
CPU 업그레이드(코어 수: 16개)
https://www.intel.com/content/www/us/en/products/sku/212457/intel-xeon-gold-6346-processor-36m-cache-3-10-ghz/specifications.html
*/

#define CHECK_RANGE(VALUE, MIN, MAX)	((VALUE) >= (MIN) && (VALUE) <= (MAX))

// TimerID가 추가되게 되면 TimerIDToString->SetTimerHandler->RegisterTimer 순으로 작업을 해준다.
enum ETimerID
{
	ON_TIMER_DEFAULT,	
	
	ON_TIMER_TASK_SCHEDULER,	// 태스크 스케줄러	
	ON_TIMER_EVERY_HOUR,		// 시간 단위 갱신
	ON_TIMER_EVERY_DAY,			// 하루 단위 갱신
	
	ON_TIMER_COUNT
};

enum EPeriodID
{
	ePeriodEverySecond	= 1*1000,
	ePeriodEveryMinute	= 1*60*1000,
	ePeriodEveryHour	= 1*60*60*1000,
	ePeriodEveryDay		= 24*60*60*1000,
	ePeriodEveryWeek	= 7*24*60*60*1000,
};

class CTask;

class CThreadTimer
{
public:
	CThreadTimer();
	~CThreadTimer();
	static	CThreadTimer&	This();
	static unsigned int __stdcall	ExecuteTimerLoopThread(VOID* pArg);
	static unsigned int __stdcall	ExecuteTimerHandlerThread(VOID* pArg);
	static	std::pair<WORD, WORD>	ShiftTime(WORD pInitHour, WORD pInitMinute, int pOffsetMinutes);
	VOID	StartThread();
	VOID	StopThread();
	DWORD	PushTask(CTask* pTask);
	DWORD	TimerLoopWorker();
	DWORD	TimerHandlerWorker();
	DWORD	RegisterTimer(DWORD pId, DWORD pWait, DWORD pPeriod);	
private:
	typedef	VOID(CThreadTimer::* TimerHandler)(DWORD pId);
	typedef	VOID(CThreadTimer::* TaskHandler)(const void* pPayload, DWORD pLng);
private:
	VOID	__ResetAttr();
	VOID	__Dtor();
	DWORD	__WaitStopThread(HANDLE pHnd, DWORD pTimeOut = INFINITE);	
	const char*	__TimerIDToString(DWORD pId);
	VOID	__SetTimerHandler();
	VOID	__RegisterTimer();			
	VOID	__LogTimerDetails(DWORD pId);
	VOID	__LogTimerStatus(DWORD pId, const char* pTimerID, const char* pFunctionName, const char* pStatus);
	VOID	__OnTimerHandler(DWORD pId);
	VOID	__OnTimerDefault(DWORD pId);
	VOID	__OnTimerEveryHour(DWORD pId);
	VOID	__OnTimerEveryDay(DWORD pId);
	VOID	__SetTaskHandler();
	VOID	__OnTaskDefault(const void* pPayload, DWORD pLng);
	VOID	__OnTaskHandler(const void* pPayload, DWORD pLng);	
private:
	static	CThreadTimer	__mSingleton;
	HANDLE	__mExitEvent;
	HANDLE	__mHndTimerLoop;
	HANDLE	__mHndTimerHandler;
	CTimer	__mTimer;
	CIocp	__mIocp;
	UINT	__mTimerLoopThreadID;
	UINT	__mTimerHandlerThreadID;	
	static	TimerHandler __mTimerHandler[ON_TIMER_COUNT];
	static	TaskHandler __mTaskHandler[ON_TASK_COUNT];
};