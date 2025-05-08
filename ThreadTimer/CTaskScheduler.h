#pragma once

#include "CLocker.h"
#include "CTask.h"

constexpr int TASK_PROCESS_INTERVAL = 100;  // �۾� ���� ���� (�и��� ����)
constexpr int TASK_SLOT_COUNT = (30 * 60 * 1000) / TASK_PROCESS_INTERVAL;  // �ִ� ���� ������ �۾� ���� ���� (30�� ���� ���� ������ �۾� Ƚ��)

/**
 * CTaskScheduler�� �񵿱� �۾�(Task)�� ���� �� �����ϴ� ����
   - ���� �������� ���� �ð��� ���� �� �����ؾ� �ϴ� Task(�۾�)�� �����ϴ� Ŭ����
   - IOCP ������� ������ Ǯ���� �񵿱� ����
   - RegisterTask()�� ȣ���ϸ� ��� ������� �ʰ�, ������ �ð��� ������ �����. 
   - ���� �ð��� ������ �۾��� ������ �����忡�� IOCP�� ���� �񵿱������� ó��
   - ���� ��ü(�÷��̾�, ����, ������, ����...)���� ���������� Task�� ��� �� ���� ����
   - ��ü�� Task�� ���������� �����Ͽ� Ư�� ��ü�� ���� ��ȭ�� ���� �������� Task ������Ʈ ����
 
 * ���� �������� ����Ǵ� Task�� ��ټ��� ���� Tick + 30�� �̳��� ����Ǹ� 30�� ���Ŀ� ����Ǵ� Task�� ��������� ����.
   �̷��� Task ���� ������ ����ȭ�ϱ� ���� CTaskScheduler�� �� ���� �����̳ʸ� ����Ͽ� ����
   - Task ���� �ð��� 30�� �̳����� ���ο� ���� �� ���� �����̳ʷ� �з��Ͽ� �����ٸ�
     1. __mFixTask[TASK_SLOT_COUNT] 30�� �̳� ������ Task ���� (���� ũ�� �迭 + FIFO ť�� ����ؼ� �������ٰ� ����)
     2. __mFlexibleTask 30�� ���� ������ Task ���� (�켱���� ť ����ؼ� �޸� ����� ȿ���� ����)
 
 * ���� �������� 30�� �̳��� ����Ǵ� Task ����
   - PvP ���, �̺�Ʈ, ���� ���� ����, ����� �� �پ��� �������� ���� �帧: 
     �̺�Ʈ ���� -> �غ� �ܰ� -> ���� -> ���� -> ���� -> ��� ���� -> �ʱ�ȭ �� ��� �ð� ����
   - ���� ������: ���� ���Ͱ� 5��, 30��, 5�� �� ������
   - ���� AI �ൿ: ���Ͱ� 2�ʸ��� �÷��̾ ����, 5�� �� ��ų ���, 10�� �� ���� �Ǵ�	
   - ���� ���� �ð� üũ: �÷��̾ ���� ������ 30��, 2��, 5�� �� �ڵ� ����
   - ����� ����: �ߵ�, ȭ��, ���� ȿ���� 10��, 1�� �� �ڵ� ����
   - ������ �Ҹ� Ÿ�̸�: �ʿ� ����� �������� 1��, 5��, 10�� �� �ڵ� �Ҹ�
   - ���� Ÿ�̸�: ���� ���� �� 10��, 15�� �� ���� ����
   - PvP ��� �ð� ����: PvP ������ 3��, 10�� �� �ڵ� ����
   - ����Ʈ ���� Ÿ�̸�: Ư�� ����Ʈ���� 1�� �� �̺�Ʈ �߻�, 5�� �� ����
   - ���̵� ���� ������ ��ȯ: ���̵� ������ 2�� �� ������ ����
   - ä�� ���� ����: Ư�� ������ 5�� �� ä�� ���� ����
   - �ڵ� ȸ�� ������ �ߵ�: ���� �ð� �� �ڵ����� ���� ���
   - ���� ���� ���� ���� ����: ������ �������� 10�� �� ����
   - ����� ������ ����: ����� ��� �������� 5�� �� ���� ���� ����
   - NPC �̺�Ʈ ��� �ð�: NPC�� 1�� �� �ٽ� ��ȭ ����	
 **/
class CTaskScheduler
{
private:
	class __CTaskLessor
	{
	public:
		bool operator()(const CTask* p1, const CTask* p2) const
		{
			return p2->mExecuteTick < p1->mExecuteTick;
		}
	};	
public:
	CTaskScheduler();
	~CTaskScheduler();
	static CTaskScheduler&	This();
private:
	static CTaskScheduler	__mSingleton;
public:
	DWORD	Open();
	VOID	Close();
	DWORD	RegisterTask(DWORD pAfter, const void* pPayload, DWORD pLng);
	VOID	UnregisterTask(CTask* pTask);
	VOID	ProcessTasks();
private:
	VOID	__ResetAttr();
	VOID	__Dtor();
	CTask*	__AllocateTask();
	VOID	__DeallocateTask(CTask* pTask);
private:
	typedef	CLockerAuto<CLocker>	__TLockerAuto;
	typedef	std::vector<CTask*>	__TFixTask;
	typedef	std::priority_queue<CTask*, std::deque<CTask*>, __CTaskLessor>	__TFlexibleTask;
private:
	DWORD	__mSchedulerTick;
	INT	 __mCurSlotIdx; // ���� ���� ���� Task Slot Index
	__TLockerAuto::TLocker	__mTaskSlotLocker;
	__TLockerAuto::TLocker	__mFlexibleTaskLocker;	
	__TFixTask	__mFixTask[TASK_SLOT_COUNT];
	__TFlexibleTask	__mFlexibleTask;
	CTaskPool	__mTaskPool;
};