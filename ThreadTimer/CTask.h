#pragma once

#include <stack>
#include "CLocker.h"

enum ETaskID
{
	ON_TASK_DEFAULT,

	ON_TASK_COUNT
};

struct OnParentMsg
{
	DWORD	mMsgType;
};

constexpr int PAYLOAD_SIZE = 300;
constexpr int TASK_POOL_CAPACITY = 1000;

// �񵿱� �۾�(Task) ��ü�� Ư�� �ð� �� ������ �����͸� �����ϴ� ����
class CTask
{
private:
	CTask();				// private ������: CTaskPool�� ���� ����
	~CTask();
	friend class CTaskPool;	// ������ CTaskPool�� ���� �����ϵ��� ����
public:
	VOID	Reset();
	VOID	Set(DWORD pAfter, const void* pPayload, DWORD pLng);
public:
	DWORD	mExecuteTick;
	DWORD	mLng;
	BYTE	mPayload[PAYLOAD_SIZE];
};

// CTask ��ü�� �����ϱ� ���� Ǯ�� ����. �⺻������ TASK_POOL_CAPACITY = 1000���� CTask ��ü�� �̸� �����Ͽ� stack�� ����
// stack�� ����� ������ ���� �ֱٿ� ����� ��ü�� ���� ���� �ٽ� ����ϹǷ� CPU ĳ�ÿ� ���� ���� ���ɼ��� ���� LIFO ����� ť���� ����ȭ ������尡 ����.
// Unity Object Pool, Unreal FObjectPool�� ��κ� LIFO ����
class CTaskPool
{
public:
	explicit CTaskPool(size_t pCapacity = TASK_POOL_CAPACITY);
	~CTaskPool();
	CTask*	Allocate();
	VOID	Deallocate(CTask* pTask);
	size_t	GetSize();
	size_t	GetCapacity() const;
private:
	typedef	CLockerAuto<CLocker>	__TLockerAuto;
private:
	__TLockerAuto::TLocker	__mLocker;
	std::stack<CTask*>	__mPool;
	size_t	__mSize;
	size_t	__mCapacity;
};