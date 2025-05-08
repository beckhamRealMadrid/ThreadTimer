#pragma once

class COverlap : public ::OVERLAPPED
{
public:
	enum EType
	{
		eTypeCnt
	};
public:
	COverlap(EType pType);
	virtual ~COverlap();	
public:
	VOID	SetOverType(EType pType);
	EType	GetOverType() const;
	operator WSAOVERLAPPED*();
protected:
	void	_Assign(const COverlap* p);
private:
	EType	__mType;
};

/*
IOCP 참고 자료
https://chfhrqnfrhc.tistory.com/entry/IOCP
*/

class CIocp
{
public:
	enum EKey
	{
		eKeyStopWorker,
		eKeyTimer,
		eKeyTask,
		eKeyIocpError
	};
public:
	CIocp();
	~CIocp();
	DWORD	Open(DWORD pThdCnt = 0);
	VOID	Close();
	BOOL	IsOpen() const;
	DWORD	Attach(HANDLE pHnd, DWORD pKey);
	DWORD	Attach(SOCKET pSock, DWORD pKey);
	DWORD	Pop(COverlap*& pOver, DWORD& pKey, DWORD& pLng, DWORD pTimeout);
	DWORD	Push(DWORD pKey, DWORD pLng, COverlap* pOver);
private:
	VOID	__Dtor();
	VOID	__ResetAttr();
private:
	HANDLE	__mIocpHnd;
};