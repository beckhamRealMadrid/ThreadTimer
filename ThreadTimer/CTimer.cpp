#include "pch.h"
#include "CTimer.h"

CTimer::CTimer()
{
	__ResetAttr();
}

CTimer::~CTimer()
{
	__Dtor();
}

VOID CTimer::__ResetAttr()
{
	__mTimerHnd = NULL;
	__mStartTick = 0;
	__mMinIdx = 0;
	__mCurIdx = 0;
}

VOID CTimer::__Dtor()
{
	if (NULL != __mTimerHnd)
	{
		CloseHandle(__mTimerHnd);
		__mTimerHnd = NULL;
	}
}

DWORD CTimer::Open()
{
	assert(!IsOpen());

	DWORD aRv = 0;
	aRv = __mLocker.Open(FALSE);
	if (0 < aRv)
	{
		return aRv; 
	}

	__mTimerHnd = ::CreateWaitableTimer(NULL, FALSE, NULL);	
	if (NULL == __mTimerHnd)
	{
		return GetLastError();
	}

	return 0;
}

VOID CTimer::Close()
{
	__Dtor();
	__mLocker.Close();
	__ResetAttr();
}

BOOL CTimer::IsOpen() const
{
	return NULL != __mTimerHnd;
}

DWORD CTimer::GetSz() const
{
	assert(IsOpen());
	
	__TLockerAuto aLocker(__mLocker);
	return static_cast<DWORD>(__mList.size());
}

HANDLE CTimer::GetHnd() const
{
	assert(IsOpen());
	
	return __mTimerHnd;
}

BOOL CTimer::IsExist(DWORD pId) const
{
	assert(IsOpen());
	assert(INFINITE != pId);

	__TLockerAuto aLocker(__mLocker);
	return __mList.end() != std::find(__mList.begin(), __mList.end(), __CElement(pId));
}

CTimer::CTimerInfo CTimer::GetTimerInfo(DWORD pId) const
{
	INT aRemain = 0;
	DWORD aPeriod = INFINITE;
	CTime aTime;
	{
		__TLockerAuto aLocker(__mLocker);
		__TList::const_iterator aIt = std::find(__mList.begin(), __mList.end(), __CElement(pId));
		if (aIt == __mList.end())
		{
			return CTimerInfo("Timer ID Not Found.", "Timer ID Not Found.", INFINITE);
		}

		aRemain = aIt->mRemain;
		aPeriod = aIt->mPeriod;
		aTime = aIt->mTime;
	}

	DWORD aTotalMilliseconds = static_cast<DWORD>(std::abs(aRemain));
	DWORD aMilliseconds = aTotalMilliseconds % 1000;
	DWORD aTotalSeconds = aTotalMilliseconds / 1000;
	DWORD aDays = aTotalSeconds / (24 * 60 * 60);
	aTotalSeconds %= (24 * 60 * 60);
	DWORD aHours = aTotalSeconds / (60 * 60);
	aTotalSeconds %= (60 * 60);
	DWORD aMinutes = aTotalSeconds / 60;
	DWORD aSeconds = aTotalSeconds % 60;

	// ex: 1d 2h 3m 4s 567ms (남은 시간이 30분 45.123초라면 30m 45s 123ms)
	std::ostringstream aRemainTimeOss;
	if (aDays > 0)
		aRemainTimeOss << aDays << "d ";
	if (aHours > 0 || aDays > 0)
		aRemainTimeOss << aHours << "h ";
	if (aMinutes > 0 || aHours > 0 || aDays > 0)
		aRemainTimeOss << aMinutes << "m ";
	aRemainTimeOss << aSeconds << "s " << aMilliseconds << "ms";

	// %04d-%02d-%02d %02d:%02d:%02d.%03d 포맷으로 문자열 생성
	if (aRemain > 0)
	{
		aTime.AddMilliseconds(aRemain);
	}

	char aSignalTimeBuffer[40];
	snprintf(aSignalTimeBuffer, sizeof(aSignalTimeBuffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
		aTime.GetYear(), aTime.GetMonth(), aTime.GetDay(),
		aTime.GetHour(), aTime.GetMinute(), aTime.GetSecond(), aTime.GetMillSec());

	return CTimerInfo(aRemainTimeOss.str(), std::string(aSignalTimeBuffer), aPeriod);
}

// pId: Key
// pWait: 처음 대기할 시간(1/1000초) 0이면 바로시작
// pPeriod: 다음 호출의 간격(1/1000초) 0이면 One Shot Timer
DWORD CTimer::Register(DWORD pId, DWORD pWait, DWORD pPeriod)
{
	assert(IsOpen());
	assert(INFINITE != pId);
	assert((0 < pWait) || (0 < pPeriod));

	__TLockerAuto aLocker(__mLocker);
	if (__mList.end() != std::find(__mList.begin(), __mList.end(), __CElement(pId)))
	{
		return ERROR_ALREADY_EXISTS;
	}

	__mList.push_back(__CElement(pId, pWait, pPeriod));
	if (pWait < __mList[__mMinIdx].mWait)
	{
		__mMinIdx = static_cast<DWORD>(__mList.size() - 1);
	}

	return 0;
}

// SetWaitableTimer함수는 비동기이다. 블럭되는 것이 아니라 시간이 되면 시그널 된다.
// WaitForSingleObject로 기다렸다가 타임아웃되면 바로 End를 호출해야 한다.
DWORD CTimer::Begin(ULONGLONG pTick)
{
	assert(IsOpen());

	LARGE_INTEGER aTm;
	{
		__TLockerAuto aLocker(__mLocker);
		assert(!__mList.empty());
		assert(__mMinIdx < __mList.size());

		__mStartTick = pTick;		
		
		// mRemain은 단위가 1/1000초이고 aTm.QuadPart는 단위가 100나노초이다. 상대시간을 위해서 음수로 설정해야 한다.
		// 음수 값을 주면 현재 시간으로부터의 상대 시간을 나타낸다. 예: -10000000 -> 1초 후에 만료(100나노초 단위)
		aTm.QuadPart = static_cast<LONGLONG>(__mList[__mMinIdx].mRemain) * -10000;		

		return ::SetWaitableTimer(__mTimerHnd, &aTm, 0, NULL, NULL, FALSE) ? 0 : GetLastError();
	}
}

ULONGLONG CTimer::End()
{
	assert(IsOpen());

	const ULONGLONG aCurTick = GetTickCount64();
	{
		__TLockerAuto aLocker(__mLocker);
		assert(!__mList.empty());

		ULONGLONG aTick = aCurTick - __mStartTick;

		__TList::iterator aEnd = __mList.end();
		for (__TList::iterator aIt = __mList.begin(); aEnd != aIt; ++aIt)
		{
			if (aIt->mRemain <= aTick)
			{
				aIt->mRemain = 0;
				aIt->mTime.Reset();
			}
			else
			{
				aIt->mRemain -= aTick;
			}
		}

		__mCurIdx = 0;
		__mMinIdx = 0;
	}

	return aCurTick;
}

// 타임아웃된 것을 찾는다. 다음 대기할 시간을 찾아야 하므로 없을때까지 반복해야 한다. 리턴값은 타임아웃된 ID 없으면 INFINITE다.
// One Shot Timer는 삭제되므로 이터레이터가 아닌 인덱스로 관리한다.
DWORD CTimer::Fetch()
{
	assert(IsOpen());

	DWORD aId = INFINITE; // 타임아웃된 ID (없으면 INFINITE)
	{
		__TLockerAuto aLocker(__mLocker);
		assert(!__mList.empty());

		__TList::iterator aEnd = __mList.end();
		for (__TList::iterator aIt = __mList.begin() + __mCurIdx; aEnd != aIt; ++aIt, ++__mCurIdx)
		{
			assert(__mCurIdx <= __mList.size());

			if (aIt->mRemain == 0) // 타임아웃된 경우
			{
				aId = aIt->mId;

				if (aIt->mPeriod != 0) // 주기적 타이머
				{
					const DWORD aPeriod = aIt->mPeriod;
					const ULONGLONG aCurTick = GetTickCount64();

					// 예상된 Tick과 실제 Tick의 차이를 계산
					if (aIt->mExpectedNextTick != 0) // 예상 Tick이 셋팅되어 있는 경우
					{
						const INT aTimeDrift = static_cast<INT>(aCurTick - aIt->mExpectedNextTick);

						if (aTimeDrift >= static_cast<INT>(aPeriod))
						{
							// 예상 Tick보다 많이 지연된 경우 누락된 주기 보정
							const INT aMissedPeriods = aTimeDrift / aPeriod; // 누락된 주기 수
							aIt->mRemain = max(0, static_cast<INT>(aPeriod - (aTimeDrift - aMissedPeriods * aPeriod)));
						}
						else
						{
							// 예상 Tick과 가까운 경우
							aIt->mRemain = max(0, static_cast<INT>(aPeriod - aTimeDrift));
						}
					}
					else
					{
						// 최초 호출 시에는 주기를 그대로 남은 시간으로 설정
						aIt->mRemain = aPeriod;
					}

					// 다음 예상 Tick 업데이트
					aIt->mExpectedNextTick = aCurTick + aIt->mRemain;					

					// 남은 시간이 가장 적은 Index 업데이트
					if (aIt->mRemain < __mList[__mMinIdx].mRemain)
					{
						__mMinIdx = __mCurIdx;
					}
				}
				else
				{
					// One Shot Timer인 경우 삭제
					__mList.erase(aIt);
				}

				// 타임아웃된 ID를 찾았으므로 반복 종료
				break;
			}
			else
			{
				// 남은 시간이 가장 적은 Index를 업데이트
				if (aIt->mRemain < __mList[__mMinIdx].mRemain)
				{
					__mMinIdx = __mCurIdx;
				}
			}
		}
	}

	return aId;
}