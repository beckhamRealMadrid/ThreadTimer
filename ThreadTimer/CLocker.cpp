#include "pch.h"
#include "CLocker.h"

CLocker::CLocker()
{
	__ResetAttr();
}

CLocker::~CLocker()
{
	__Dtor();
}

VOID CLocker::__ResetAttr()
{
	__mIsOpen = false;
}

VOID CLocker::__Dtor()
{
	if (__mIsOpen)
	{
		assert(__mSec.RecursionCount <= 0);
		::DeleteCriticalSection(&__mSec);
		__mIsOpen = false;
	}
}

VOID CLocker::Close()
{
	__Dtor();
	__ResetAttr();
}

bool CLocker::IsOpen() const
{
	return __mIsOpen;
}

DWORD CLocker::Open(BOOL pIsManyRace, DWORD pSpin)
{
	assert(!IsOpen());

	DWORD aRv = 0;	

	// Spin Count�� WIN2K�̻󿡼��� �����ϸ� ���� Spin Count�� ����Ҽ� ������ �Ϲ����� Critical Section�� ����Ѵ�.
	#if WINVER >= 0x0500
	if (0 < pSpin)
	{
		aRv = __InitializeCriticalSectionWithSpin(pIsManyRace, pSpin);
		if (aRv == 0)
		{
			return aRv;
		}

		// ���� �� ���� �ܰ�� ����
		assert(0);
	}
	#endif

	// Spin Count�� ����� �� ���� ��� �Ϲ� Critical Section �ʱ�ȭ
	__try
	{
		::InitializeCriticalSection(&__mSec);
		__mIsOpen = true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		assert(0);
		aRv = STATUS_NO_MEMORY;
	}

	return aRv;
}

DWORD CLocker::__InitializeCriticalSectionWithSpin(BOOL pIsManyRace, DWORD pSpin)
{
	// Thread���� ������ ���� �߻��ϴ��� ����
	if (pIsManyRace)
	{
		pSpin |= MANY_RACE_FLAG;
	}

	// CPU�� 1���̸� API��ü�� �����ϹǷ� ����� ����� CPU���� ����� �ʿ䰡 ����.
	if (::InitializeCriticalSectionAndSpinCount(&__mSec, pSpin))
	{
		__mIsOpen = true;
		return 0;
	}

	return STATUS_NO_MEMORY; // ���� �� ��ȯ
}

VOID CLocker::Lock() const
{
	assert(IsOpen());

	::EnterCriticalSection(&__mSec);	
}

bool CLocker::LockTry() const
{
	assert(IsOpen());

	if (::TryEnterCriticalSection(&__mSec))
	{
		DWORD aThd = GetCurrentThreadId();
		assert(aThd == HandleToUlong(__mSec.OwningThread));
		return true;
	}
	else
	{
		return false;
	}
}

VOID CLocker::Unlock() const
{
	assert(IsOpen());
	assert(0 < __mSec.RecursionCount);
	assert(GetCurrentThreadId() == HandleToUlong(__mSec.OwningThread));

	::LeaveCriticalSection(&__mSec);
}

BOOL CLocker::IsLock() const
{
	assert(IsOpen());

	return (0 < __mSec.RecursionCount) && (GetCurrentThreadId() == HandleToUlong(__mSec.OwningThread));
}