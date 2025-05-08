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

// System�� �ð��� �����Ѵ�.
VOID CTime::Set(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond)
{
	__Set(pYear, pMonth, pDay, pHour, pMinute, pSecond);	
}

VOID CTime::Set(const FILETIME& pFile)
{
	BOOL aRv;
	FILETIME aLocal;

	// FILETIME�� ���� �ð���� ��ȯ
	aRv = ::FileTimeToLocalFileTime(&pFile, &aLocal);
	assert(aRv);

	// ��ȯ�� FILETIME�� SYSTEMTIME���� ��ȯ
	aRv = ::FileTimeToSystemTime(&aLocal, &__mLocalTm);
	assert(aRv);
}

VOID CTime::Set(const SYSTEMTIME& pLocal)
{
	__mLocalTm = pLocal;
}

VOID CTime::__Set(WORD pYear, WORD pMonth, WORD pDay, WORD pHour, WORD pMinute, WORD pSecond)
{
	assert((pYear > 1601) && (pYear < 9999));  // SYSTEMTIME ����
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

// �Ķ���ͷ� ���޵� �ð��� �ɶ������� ���� �ð��� �����ش�.(����:1/1000��)
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

// �� �� ���� ���
DWORD CTime::__GetTotalSeconds(WORD pDayOfWeek, WORD pHour, WORD pMinute, WORD pSecond)
{
	return pSecond + (pMinute * 60) + (pHour * 60 * 60) + (pDayOfWeek * 24 * 60 * 60);
}

// ��� �ð� ���
DWORD CTime::__CalculateWaitTime(DWORD pCurrentSeconds, DWORD pTargetSeconds, DWORD pMaxSeconds)
{
	if (pCurrentSeconds < pTargetSeconds)
	{
		return (pTargetSeconds - pCurrentSeconds) * 1000;  // �и��ʷ� ��ȯ
	}
	else if (pTargetSeconds < pCurrentSeconds)
	{
		return (pMaxSeconds - pCurrentSeconds + pTargetSeconds) * 1000;  // ��ȯ ó��
	}
	else
	{
		return 0;  // ���� �ð�
	}
}

// ����ð��� pVal(����:��)�� ���Ѵ�.
VOID CTime::AddSecond(ULONGLONG pVal)
{
	assert(0 < pVal);

	// ���� �ð��� FILETIME���� ��ȯ
	FILETIME aFt;
	::SystemTimeToFileTime(&__mLocalTm, &aFt);
	
	ULARGE_INTEGER aNum;
	aNum.LowPart = aFt.dwLowDateTime;
	aNum.HighPart = aFt.dwHighDateTime;

	// �� ������ 100������ ������ ��ȯ�Ͽ� �߰�
	aNum.QuadPart += (pVal * 10000000);

	aFt.dwLowDateTime = aNum.LowPart;
	aFt.dwHighDateTime = aNum.HighPart;
	
	// �ٽ� SYSTEMTIME���� ��ȯ
	::FileTimeToSystemTime(&aFt, &__mLocalTm);
}

VOID CTime::AddMilliseconds(ULONGLONG pVal)
{
	assert(0 < pVal);

	// ���� �ð��� FILETIME���� ��ȯ
	FILETIME aFt;
	::SystemTimeToFileTime(&__mLocalTm, &aFt);

	ULARGE_INTEGER aNum;
	aNum.LowPart = aFt.dwLowDateTime;
	aNum.HighPart = aFt.dwHighDateTime;

	// �и��� ������ 100������ ������ ��ȯ�Ͽ� �߰�
	aNum.QuadPart += (pVal * 10000);

	aFt.dwLowDateTime = aNum.LowPart;
	aFt.dwHighDateTime = aNum.HighPart;

	// �ٽ� SYSTEMTIME���� ��ȯ
	::FileTimeToSystemTime(&aFt, &__mLocalTm);
}

// �� �ð��� ��(��)�� ���Ѵ�. INT�� �� 24855���� ���̸� ǥ���Ҽ� �ִ�. �� ���̸� �Ѿ�� �˼� ����.
INT CTime::operator-(const CTime& p) const
{
	FILETIME aFt;

	// ���� ��ü�� SYSTEMTIME�� FILETIME���� ��ȯ
	::SystemTimeToFileTime(&__mLocalTm, &aFt);
	LARGE_INTEGER a1;
	a1.LowPart = aFt.dwLowDateTime;
	a1.HighPart = static_cast<LONG>(aFt.dwHighDateTime);

	// �Է� ��ü�� SYSTEMTIME�� FILETIME���� ��ȯ
	::SystemTimeToFileTime(&p.__mLocalTm, &aFt);
	LARGE_INTEGER a2;
	a2.LowPart = aFt.dwLowDateTime;
	a2.HighPart = static_cast<LONG>(aFt.dwHighDateTime);

	// FILETIME�� �� ������ ��ȯ (100 ������ ���� -> �� ����)
	a1.QuadPart /= 10000000;	// 100-nano�� �ʷ� ��ȯ.
	a2.QuadPart /= 10000000;

	// �� �ð��� ���̸� ��ȯ (�� ����)
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