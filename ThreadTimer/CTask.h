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

// 비동기 작업(Task) 객체로 특정 시간 후 실행할 데이터를 저장하는 역할
class CTask
{
private:
	CTask();				// private 생성자: CTaskPool만 생성 가능
	~CTask();
	friend class CTaskPool;	// 무조건 CTaskPool만 생성 가능하도록 설정
public:
	VOID	Reset();
	VOID	Set(DWORD pAfter, const void* pPayload, DWORD pLng);
public:
	DWORD	mExecuteTick;
	DWORD	mLng;
	BYTE	mPayload[PAYLOAD_SIZE];
};

// CTask 객체를 재사용하기 위한 풀을 제공. 기본적으로 TASK_POOL_CAPACITY = 1000개의 CTask 객체를 미리 생성하여 stack에 저장
// stack을 사용한 이유는 가장 최근에 사용한 객체를 가장 먼저 다시 사용하므로 CPU 캐시에 남아 있을 가능성이 높고 LIFO 방식은 큐보다 동기화 오버헤드가 낮다.
// Unity Object Pool, Unreal FObjectPool도 대부분 LIFO 구조
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