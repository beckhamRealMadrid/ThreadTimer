#pragma once

#include "CLocker.h"
#include "CTime.h"
#include <sstream>

/*
C# System.Threading.Timer와 비슷하게 동작하는것이 목표
https://learn.microsoft.com/ko-kr/dotnet/api/system.threading.timer?view=net-8.0
호출순서는 Open->Register->{Begin->End->Fetch}->Close
등록은 여러 스레드에서 할 수 있지만 대기하는 스레드는 하나여야 한다.
*/
class CTimer
{
public:
	class CTimerInfo
	{
	public:
		CTimerInfo(const std::string& pRemainTime = "N/A", const std::string& pSignalTime = "N/A", DWORD pPeriod = INFINITE)
			: mRemainTime(pRemainTime), mSignalTime(pSignalTime), mPeriod(pPeriod)
		{}
		std::string ToString() const
		{
			std::ostringstream oss;
			oss << "Remain Time: " << mRemainTime
				<< ", Signal Time: " << mSignalTime
				<< ", Period: " << (mPeriod == INFINITE ? "INFINITE" : std::to_string(mPeriod) + " ms");
			return oss.str();
		}
		std::string GetRemainTime() const { return mRemainTime; }
		std::string GetSignalTime() const { return mSignalTime; }
		std::string GetPeriod() const { return (mPeriod == INFINITE ? "INFINITE" : std::to_string(mPeriod) + " ms"); }
	private:
		std::string	mRemainTime;
		std::string	mSignalTime;
		DWORD	mPeriod;
	};
private:
	class __CElement
	{
	public:
		explicit __CElement(DWORD pId = 0, DWORD pWait = 0, DWORD pPeriod = 0)
			: mId(pId)
			, mWait(pWait)
			, mPeriod(pPeriod)
			, mRemain(static_cast<INT>(pWait))
			, mExpectedNextTick(0)
		{}
		bool operator==(const __CElement& p) const { return p.mId == mId; }
	public:
		DWORD	mId;
		DWORD	mWait;
		DWORD	mPeriod;
		INT		mRemain;	// 남은 시간(1/1000초) 0이면 Timeout 상대시간을 위해서 음수로 설정해야 하므로 DWORD로 하면 안된다.
		ULONGLONG	mExpectedNextTick;	// 처리될 예상틱 GetTickCount64()와 직접 비교
		CTime	mTime;
	};	
public:
	CTimer();
	~CTimer();
public:
	DWORD	Open();
	VOID	Close();
	BOOL	IsOpen() const;
	DWORD	GetSz() const;
	HANDLE	GetHnd() const;	
	DWORD	Register(DWORD pId, DWORD pWait, DWORD pPeriod);
	BOOL	IsExist(DWORD pId) const;
	CTimerInfo	GetTimerInfo(DWORD pId) const;
	DWORD	Begin(ULONGLONG pTick);
	ULONGLONG	End();
	DWORD	Fetch();	
private:
	VOID	__Dtor();
	VOID	__ResetAttr();
private:
	typedef	std::vector<__CElement>	__TList;
	typedef	CLockerAuto<CLocker>	__TLockerAuto;
private:
	HANDLE	__mTimerHnd;
	ULONGLONG	__mStartTick;	// Begin을 호출했을때의 TickCount	
	DWORD	__mMinIdx;			// Timeout이 가장 작은 인덱스
	DWORD	__mCurIdx;			// Fetch 시작 위치
	__TList	__mList;
	__TLockerAuto::TLocker	__mLocker;	// __mList에 대해서만 동기화를 한다.
};