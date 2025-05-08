#include "pch.h"
#include "CTime.h"

CTime::CTime()
{ 
	Reset();
}

CTime::CTime(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond)
{
	__Set(pYear, pMonth, pDay, pHour, pMinute, pSecond);
}

CTime::CTime(const FILETIME& pFile)
{
	Set(pFile);
}

CTime::CTime(const SYSTEMTIME& pLocal)
{
	Set(pLocal);
}

WORD CTime::GetYear() const
{ 
	return __mLocalTm.wYear;
}

WORD CTime::GetMonth() const
{ 
	return __mLocalTm.wMonth;
}

WORD CTime::GetDay() const
{ 
	return __mLocalTm.wDay;
}

WORD CTime::GetHour() const
{ 
	return __mLocalTm.wHour;
}

WORD CTime::GetMinute() const 
{ 
	return __mLocalTm.wMinute;
}

WORD CTime::GetSecond() const
{ 
	return __mLocalTm.wSecond;
}

WORD CTime::GetMillSec() const 
{ 
	return __mLocalTm.wMilliseconds;
}

WORD CTime::GetDayOfWeek() const 
{ 
	return __mLocalTm.wDayOfWeek;
}

const SYSTEMTIME& CTime::GetSystemTime() const 
{ 
	return __mLocalTm;
}

SYSTEMTIME& CTime::GetSystemTime() 
{ 
	return __mLocalTm;
}

VOID CTime::Reset() 
{ 
	::GetLocalTime(&__mLocalTm);
}

// System의 시간을 설정한다.
VOID CTime::Set(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond)
{
	__Set(pYear, pMonth, pDay, pHour, pMinute, pSecond);	
}

VOID CTime::Set(const FILETIME& pFile)
{
	BOOL aRv;
	FILETIME aLocal;

	// FILETIME을 로컬 시간대로 변환
	aRv = ::FileTimeToLocalFileTime(&pFile, &aLocal);
	assert(aRv);

	// 변환된 FILETIME을 SYSTEMTIME으로 변환
	aRv = ::FileTimeToSystemTime(&aLocal, &__mLocalTm);
	assert(aRv);
}

VOID CTime::Set(const SYSTEMTIME& pLocal)
{
	__mLocalTm = pLocal;
}

VOID CTime::__Set(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond)
{
	assert((pYear > 1601) && (pYear < 9999));  // SYSTEMTIME 범위
	assert((pMonth >= 1) && (pMonth <= 12));
	assert((pDay >= 1) && (pDay <= 31));
	assert((pHour < 24) && (pMinute < 60) && (pSecond < 60));

	__mLocalTm.wYear = pYear;
	__mLocalTm.wMonth = pMonth;
	__mLocalTm.wDayOfWeek = 0;
	__mLocalTm.wDay = pDay;
	__mLocalTm.wHour = pHour;
	__mLocalTm.wMinute = pMinute;
	__mLocalTm.wSecond = pSecond;
	__mLocalTm.wMilliseconds = 0;
}

// 파라미터로 전달된 시간이 될때까지의 남은 시간을 돌려준다.(단위:1/1000초)
DWORD CTime::GetWaitTm(EDayOfWeek pDayOfWeek, WORD pHour, WORD pMinute, WORD pSecond)
{
	assert((eSunday <= pDayOfWeek) && (pDayOfWeek <= eSaturday));
	assert(pHour < 24);
	assert(pMinute < 60);
	assert(pSecond < 60);

	DWORD aCurrentSeconds = __GetTotalSeconds(__mLocalTm.wDayOfWeek, __mLocalTm.wHour, __mLocalTm.wMinute, __mLocalTm.wSecond);
	DWORD aTargetSeconds = __GetTotalSeconds(pDayOfWeek, pHour, pMinute, pSecond);

	return __CalculateWaitTime(aCurrentSeconds, aTargetSeconds, 7 * 24 * 60 * 60);
}

DWORD CTime::GetWaitTm(WORD pHour, WORD pMinute, WORD pSecond)
{
	assert(pHour < 24);
	assert(pMinute < 60);
	assert(pSecond < 60);

	DWORD aCurrentSeconds = __GetTotalSeconds(0, __mLocalTm.wHour, __mLocalTm.wMinute, __mLocalTm.wSecond);
	DWORD aTargetSeconds = __GetTotalSeconds(0, pHour, pMinute, pSecond);

	return __CalculateWaitTime(aCurrentSeconds, aTargetSeconds, 24 * 60 * 60);
}

DWORD CTime::GetWaitTm(WORD pMinute, WORD pSecond)
{
	assert(pMinute < 60);
	assert(pSecond < 60);

	DWORD aCurrentSeconds = __GetTotalSeconds(0, 0, __mLocalTm.wMinute, __mLocalTm.wSecond);
	DWORD aTargetSeconds = __GetTotalSeconds(0, 0, pMinute, pSecond);

	return __CalculateWaitTime(aCurrentSeconds, aTargetSeconds, 60 * 60);
}

DWORD CTime::GetWaitTm(WORD pSecond)
{
	assert(pSecond < 60);

	DWORD aCurrentSeconds = __mLocalTm.wSecond;
	DWORD aTargetSeconds = pSecond;

	return __CalculateWaitTime(aCurrentSeconds, aTargetSeconds, 60);
}

// 총 초 단위 계산
DWORD CTime::__GetTotalSeconds(WORD pDayOfWeek, WORD pHour, WORD pMinute, WORD pSecond)
{
	return pSecond + (pMinute * 60) + (pHour * 60 * 60) + (pDayOfWeek * 24 * 60 * 60);
}

// 대기 시간 계산
DWORD CTime::__CalculateWaitTime(DWORD pCurrentSeconds, DWORD pTargetSeconds, DWORD pMaxSeconds)
{
	if (pCurrentSeconds < pTargetSeconds)
	{
		return (pTargetSeconds - pCurrentSeconds) * 1000;  // 밀리초로 변환
	}
	else if (pTargetSeconds < pCurrentSeconds)
	{
		return (pMaxSeconds - pCurrentSeconds + pTargetSeconds) * 1000;  // 순환 처리
	}
	else
	{
		return 0;  // 동일 시간
	}
}

// 현재시간에 pVal(단위:초)를 더한다.
VOID CTime::AddSecond(ULONGLONG pVal)
{
	assert(0 < pVal);

	// 현재 시간을 FILETIME으로 변환
	FILETIME aFt;
	::SystemTimeToFileTime(&__mLocalTm, &aFt);
	
	ULARGE_INTEGER aNum;
	aNum.LowPart = aFt.dwLowDateTime;
	aNum.HighPart = aFt.dwHighDateTime;

	// 초 단위를 100나노초 단위로 변환하여 추가
	aNum.QuadPart += (pVal * 10000000);

	aFt.dwLowDateTime = aNum.LowPart;
	aFt.dwHighDateTime = aNum.HighPart;
	
	// 다시 SYSTEMTIME으로 변환
	::FileTimeToSystemTime(&aFt, &__mLocalTm);
}

VOID CTime::AddMilliseconds(ULONGLONG pVal)
{
	assert(0 < pVal);

	// 현재 시간을 FILETIME으로 변환
	FILETIME aFt;
	::SystemTimeToFileTime(&__mLocalTm, &aFt);

	ULARGE_INTEGER aNum;
	aNum.LowPart = aFt.dwLowDateTime;
	aNum.HighPart = aFt.dwHighDateTime;

	// 밀리초 단위를 100나노초 단위로 변환하여 추가
	aNum.QuadPart += (pVal * 10000);

	aFt.dwLowDateTime = aNum.LowPart;
	aFt.dwHighDateTime = aNum.HighPart;

	// 다시 SYSTEMTIME으로 변환
	::FileTimeToSystemTime(&aFt, &__mLocalTm);
}

// 두 시간의 차(초)를 구한다. INT로 약 24855일의 차이를 표현할수 있다. 이 차이를 넘어가면 알수 없다.
INT CTime::operator-(const CTime& p) const
{
	FILETIME aFt;

	// 현재 객체의 SYSTEMTIME을 FILETIME으로 변환
	::SystemTimeToFileTime(&__mLocalTm, &aFt);
	LARGE_INTEGER a1;
	a1.LowPart = aFt.dwLowDateTime;
	a1.HighPart = static_cast<LONG>(aFt.dwHighDateTime);

	// 입력 객체의 SYSTEMTIME을 FILETIME으로 변환
	::SystemTimeToFileTime(&p.__mLocalTm, &aFt);
	LARGE_INTEGER a2;
	a2.LowPart = aFt.dwLowDateTime;
	a2.HighPart = static_cast<LONG>(aFt.dwHighDateTime);

	// FILETIME을 초 단위로 변환 (100 나노초 단위 -> 초 단위)
	a1.QuadPart /= 10000000;	// 100-nano를 초로 변환.
	a2.QuadPart /= 10000000;

	// 두 시간의 차이를 반환 (초 단위)
	return static_cast<INT>(a1.QuadPart - a2.QuadPart);
}

const char* CTime::DayOfWeekToString()
{
	switch (__mLocalTm.wDayOfWeek)
	{
		ENUM_TO_STRING(EDayOfWeek, eSunday)
		ENUM_TO_STRING(EDayOfWeek, eMonday)
		ENUM_TO_STRING(EDayOfWeek, eTuesday)
		ENUM_TO_STRING(EDayOfWeek, eWednesday)
		ENUM_TO_STRING(EDayOfWeek, eThursday)
		ENUM_TO_STRING(EDayOfWeek, eFriday)
		ENUM_TO_STRING(EDayOfWeek, eSaturday)
		default: return "Unknown";
	}
}

INT	CTime::Compare(const FILETIME& p1, const FILETIME& p2)
{ 
	return ::CompareFileTime(&p1, &p2);
}