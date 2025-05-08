#pragma once

#define ENUM_TO_STRING(enum_name, enum_value) case enum_name::enum_value: return #enum_value;

class CTime
{
public:
	// SYSTEMTIME::wDayOfWeek의 값과 동일하게 설정한다.
	enum EDayOfWeek
	{ 
		eSunday = 0,
		eMonday,
		eTuesday,
		eWednesday,
		eThursday,
		eFriday,
		eSaturday
	};
public:
	CTime();
	CTime(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond);
	explicit CTime(const FILETIME& pFile);
	explicit CTime(const SYSTEMTIME& pLocal);
	WORD	GetYear() const;
	WORD	GetMonth() const;
	WORD	GetDay() const;
	WORD	GetHour() const;
	WORD	GetMinute() const;
	WORD	GetSecond() const;
	WORD	GetMillSec() const;
	WORD	GetDayOfWeek() const;
	const SYSTEMTIME&	GetSystemTime() const;
	SYSTEMTIME&	GetSystemTime();
	VOID	Reset();
	VOID	Set(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond);
	VOID	Set(const FILETIME& pFile);
	VOID	Set(const SYSTEMTIME& pLocal);
	DWORD	GetWaitTm(EDayOfWeek pDayOfWeek, WORD pHour, WORD pMinute, WORD pSecond);
	DWORD	GetWaitTm(WORD pHour, WORD pMinute, WORD pSecond);
	DWORD	GetWaitTm(WORD pMinute, WORD pSecond);
	DWORD	GetWaitTm(WORD pSecond);
	VOID	AddSecond(ULONGLONG pVal);
	VOID	AddMilliseconds(ULONGLONG pVal);
	INT		operator-(const CTime& p) const;
	const char*	DayOfWeekToString();
	static INT	Compare(const FILETIME& p1, const FILETIME& p2);
private:
	VOID	__Set(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond);
	DWORD	__GetTotalSeconds(WORD pDayOfWeek, WORD pHour, WORD pMinute, WORD pSecond);
	DWORD	__CalculateWaitTime(DWORD pCurrentSeconds, DWORD pTargetSeconds, DWORD pMaxSeconds);
private:
	SYSTEMTIME	__mLocalTm;
};