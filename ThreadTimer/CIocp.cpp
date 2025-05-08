#include "pch.h"
#include "CIocp.h"

COverlap::COverlap(EType pType) : __mType(pType)
{
	assert((0 <= pType) && (pType < eTypeCnt));

	OVERLAPPED::Internal = OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = OVERLAPPED::OffsetHigh = 0;
	OVERLAPPED::hEvent = NULL;
}

COverlap::~COverlap()
{
	
}

VOID COverlap::SetOverType(EType pType)
{
	assert((0 <= pType) && (pType < eTypeCnt));

	__mType = pType;
}

COverlap::EType COverlap::GetOverType() const
{
	return __mType;
}

void COverlap::_Assign(const COverlap* p)
{
	assert((NULL != p) && (p != this));

	OVERLAPPED::Internal = OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = OVERLAPPED::OffsetHigh = 0;
	OVERLAPPED::hEvent = NULL;
	__mType = p->__mType;
}

COverlap::operator WSAOVERLAPPED*()
{
	return this;
}

CIocp::CIocp() : __mIocpHnd(NULL)
{
	__ResetAttr();
}

CIocp::~CIocp()
{
	__Dtor();
}

VOID CIocp::__Dtor()
{
	if (NULL != __mIocpHnd)
	{
		CloseHandle(__mIocpHnd);
		__mIocpHnd = NULL;
	}
}

VOID CIocp::__ResetAttr()
{
	__mIocpHnd = NULL;
}

DWORD CIocp::Open(DWORD pThdCnt)
{
	assert(!IsOpen());

	DWORD aRv = 0;
	__mIocpHnd = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, pThdCnt);
	if (NULL == __mIocpHnd)
	{
		aRv = GetLastError();
	}

	return aRv;	
}

VOID CIocp::Close()
{
	__Dtor();
	__ResetAttr();
}

BOOL  CIocp::IsOpen() const
{
	return NULL != __mIocpHnd;
}

DWORD CIocp::Attach(HANDLE pHnd, DWORD pKey)
{
	assert(IsOpen());
	assert(INVALID_HANDLE_VALUE != pHnd);

	DWORD aRv = 0;
	if (NULL == ::CreateIoCompletionPort(pHnd, __mIocpHnd, pKey, 0))
	{
		aRv = GetLastError();
		assert(ERROR_INVALID_PARAMETER != aRv);
	}

	return aRv;
}

DWORD CIocp::Attach(SOCKET pSock, DWORD pKey)
{
	assert(IsOpen());
	assert(INVALID_SOCKET != pSock);

	return Attach((HANDLE)pSock, pKey);
}

DWORD CIocp::Pop(COverlap*& pOver, DWORD& pKey, DWORD& pLng, DWORD pTimeout)
{
	assert(IsOpen());

	OVERLAPPED*	aOver = NULL;
	DWORD aRv = 0;
	ULONG_PTR aKey = 0;

	if (0 == ::GetQueuedCompletionStatus(__mIocpHnd, &pLng, &aKey, &aOver, pTimeout))
	{
		aRv = GetLastError();
	}

	pOver = static_cast<COverlap*>(aOver);
	pKey = static_cast<DWORD>(aKey);
	assert(pKey == aKey);
	
	return aRv;
}

DWORD CIocp::Push(DWORD pKey, DWORD pLng, COverlap* pOver)
{
	assert(IsOpen());

	return ::PostQueuedCompletionStatus(__mIocpHnd, pLng, pKey, pOver) ? 0 : GetLastError();
}
