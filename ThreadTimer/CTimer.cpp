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

	// ex: 1d 2h 3m 4s 567ms (���� �ð��� 30�� 45.123�ʶ�� 30m 45s 123ms)
	std::ostringstream aRemainTimeOss;
	if (aDays > 0)
		aRemainTimeOss << aDays << "d ";
	if (aHours > 0 || aDays > 0)
		aRemainTimeOss << aHours << "h ";
	if (aMinutes > 0 || aHours > 0 || aDays > 0)
		aRemainTimeOss << aMinutes << "m ";
	aRemainTimeOss << aSeconds << "s " << aMilliseconds << "ms";

	// %04d-%02d-%02d %02d:%02d:%02d.%03d �������� ���ڿ� ����
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
// pWait: ó�� ����� �ð�(1/1000��) 0�̸� �ٷν���
// pPeriod: ���� ȣ���� ����(1/1000��) 0�̸� One Shot Timer
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

// SetWaitableTimer�Լ��� �񵿱��̴�. ���Ǵ� ���� �ƴ϶� �ð��� �Ǹ� �ñ׳� �ȴ�.
// WaitForSingleObject�� ��ٷȴٰ� Ÿ�Ӿƿ��Ǹ� �ٷ� End�� ȣ���ؾ� �Ѵ�.
DWORD CTimer::Begin(ULONGLONG pTick)
{
	assert(IsOpen());

	LARGE_INTEGER aTm;
	{
		__TLockerAuto aLocker(__mLocker);
		assert(!__mList.empty());
		assert(__mMinIdx < __mList.size());

		__mStartTick = pTick;		
		
		// mRemain�� ������ 1/1000���̰� aTm.QuadPart�� ������ 100�������̴�. ���ð��� ���ؼ� ������ �����ؾ� �Ѵ�.
		// ���� ���� �ָ� ���� �ð����κ����� ��� �ð��� ��Ÿ����. ��: -10000000 -> 1�� �Ŀ� ����(100������ ����)
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

// Ÿ�Ӿƿ��� ���� ã�´�. ���� ����� �ð��� ã�ƾ� �ϹǷ� ���������� �ݺ��ؾ� �Ѵ�. ���ϰ��� Ÿ�Ӿƿ��� ID ������ INFINITE��.
// One Shot Timer�� �����ǹǷ� ���ͷ����Ͱ� �ƴ� �ε����� �����Ѵ�.
DWORD CTimer::Fetch()
{
	assert(IsOpen());

	DWORD aId = INFINITE; // Ÿ�Ӿƿ��� ID (������ INFINITE)
	{
		__TLockerAuto aLocker(__mLocker);
		assert(!__mList.empty());

		__TList::iterator aEnd = __mList.end();
		for (__TList::iterator aIt = __mList.begin() + __mCurIdx; aEnd != aIt; ++aIt, ++__mCurIdx)
		{
			assert(__mCurIdx <= __mList.size());

			if (aIt->mRemain == 0) // Ÿ�Ӿƿ��� ���
			{
				aId = aIt->mId;

				if (aIt->mPeriod != 0) // �ֱ��� Ÿ�̸�
				{
					const DWORD aPeriod = aIt->mPeriod;
					const ULONGLONG aCurTick = GetTickCount64();

					// ����� Tick�� ���� Tick�� ���̸� ���
					if (aIt->mExpectedNextTick != 0) // ���� Tick�� ���õǾ� �ִ� ���
					{
						const INT aTimeDrift = static_cast<INT>(aCurTick - aIt->mExpectedNextTick);

						if (aTimeDrift >= static_cast<INT>(aPeriod))
						{
							// ���� Tick���� ���� ������ ��� ������ �ֱ� ����
							const INT aMissedPeriods = aTimeDrift / aPeriod; // ������ �ֱ� ��
							aIt->mRemain = max(0, static_cast<INT>(aPeriod - (aTimeDrift - aMissedPeriods * aPeriod)));
						}
						else
						{
							// ���� Tick�� ����� ���
							aIt->mRemain = max(0, static_cast<INT>(aPeriod - aTimeDrift));
						}
					}
					else
					{
						// ���� ȣ�� �ÿ��� �ֱ⸦ �״�� ���� �ð����� ����
						aIt->mRemain = aPeriod;
					}

					// ���� ���� Tick ������Ʈ
					aIt->mExpectedNextTick = aCurTick + aIt->mRemain;					

					// ���� �ð��� ���� ���� Index ������Ʈ
					if (aIt->mRemain < __mList[__mMinIdx].mRemain)
					{
						__mMinIdx = __mCurIdx;
					}
				}
				else
				{
					// One Shot Timer�� ��� ����
					__mList.erase(aIt);
				}

				// Ÿ�Ӿƿ��� ID�� ã�����Ƿ� �ݺ� ����
				break;
			}
			else
			{
				// ���� �ð��� ���� ���� Index�� ������Ʈ
				if (aIt->mRemain < __mList[__mMinIdx].mRemain)
				{
					__mMinIdx = __mCurIdx;
				}
			}
		}
	}

	return aId;
}