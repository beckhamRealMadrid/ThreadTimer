#pragma once

#include "CLocker.h"
#include "CTime.h"
#include <sstream>

/*
C# System.Threading.Timer�� ����ϰ� �����ϴ°��� ��ǥ
https://learn.microsoft.com/ko-kr/dotnet/api/system.threading.timer?view=net-8.0
ȣ������� Open->Register->{Begin->End->Fetch}->Close
����� ���� �����忡�� �� �� ������ ����ϴ� ������� �ϳ����� �Ѵ�.
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
		INT		mRemain;	// ���� �ð�(1/1000��) 0�̸� Timeout ���ð��� ���ؼ� ������ �����ؾ� �ϹǷ� DWORD�� �ϸ� �ȵȴ�.
		ULONGLONG	mExpectedNextTick;	// ó���� ����ƽ GetTickCount64()�� ���� ��
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
	ULONGLONG	__mStartTick;	// Begin�� ȣ���������� TickCount	
	DWORD	__mMinIdx;			// Timeout�� ���� ���� �ε���
	DWORD	__mCurIdx;			// Fetch ���� ��ġ
	__TList	__mList;
	__TLockerAuto::TLocker	__mLocker;	// __mList�� ���ؼ��� ����ȭ�� �Ѵ�.
};