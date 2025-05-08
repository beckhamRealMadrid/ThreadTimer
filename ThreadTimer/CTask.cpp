#include "pch.h"
#include "CTask.h"

CTask::CTask()
{
	Reset();
}

CTask::~CTask()
{

}

VOID CTask::Reset()
{
	mExecuteTick = 0;
	mLng = 0;
	memset(mPayload, 0, sizeof(mPayload));
}

VOID CTask::Set(DWORD pAfter, const void* pPayload, DWORD pLng)
{
	assert(pLng <= sizeof(mPayload));

	mExecuteTick = pAfter;
	mLng = pLng;
	memcpy(mPayload, pPayload, pLng);
}

CTaskPool::CTaskPool(size_t pCapacity) : __mCapacity(pCapacity), __mSize(0)
{
	__mLocker.Open(TRUE);

	for (size_t aIdx = 0; aIdx < __mCapacity; ++aIdx)
	{
		__mPool.push(new CTask());
		++__mSize;
	}
}

CTaskPool::~CTaskPool()
{
	{
		__TLockerAuto aLocker(__mLocker);
		while (!__mPool.empty())
		{
			delete __mPool.top();
			__mPool.pop();
		}
	}

	__mSize = 0;
	__mLocker.Close();
}

CTask* CTaskPool::Allocate()
{
	__TLockerAuto aLocker(__mLocker);

	if (__mPool.empty())
	{
		++__mCapacity;
		return new CTask();
	}

	CTask* pTask = __mPool.top();
	__mPool.pop();
	--__mSize;

	return pTask;
}

VOID CTaskPool::Deallocate(CTask* pTask)
{
	__TLockerAuto aLocker(__mLocker);

	if (!pTask)
		return;

	pTask->Reset();

	__mPool.push(pTask);
	++__mSize;
}

size_t CTaskPool::GetSize()
{
	__TLockerAuto aLocker(__mLocker);
	return __mSize;
}

size_t CTaskPool::GetCapacity() const
{
	__TLockerAuto aLocker(__mLocker);
	return __mCapacity;
}