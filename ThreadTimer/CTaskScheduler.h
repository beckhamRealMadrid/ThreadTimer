#pragma once

#include "CLocker.h"
#include "CTask.h"

constexpr int TASK_PROCESS_INTERVAL = 100;  // 작업 실행 간격 (밀리초 단위)
constexpr int TASK_SLOT_COUNT = (30 * 60 * 1000) / TASK_PROCESS_INTERVAL;  // 최대 예약 가능한 작업 슬롯 개수 (30분 동안 실행 가능한 작업 횟수)

/**
 * CTaskScheduler는 비동기 작업(Task)을 예약 및 실행하는 역할
   - 게임 서버에서 일정 시간이 지난 후 실행해야 하는 Task(작업)를 관리하는 클래스
   - IOCP 기반으로 스레드 풀에서 비동기 실행
   - RegisterTask()를 호출하면 즉시 실행되지 않고, 지정된 시간이 지나야 실행됨. 
   - 실행 시간에 도달한 작업은 별도의 스레드에서 IOCP를 통해 비동기적으로 처리
   - 각각 객체(플레이어, 몬스터, 아이템, 버프...)마다 개별적으로 Task를 등록 및 실행 가능
   - 객체별 Task를 독립적으로 관리하여 특정 개체의 상태 변화에 따른 유동적인 Task 업데이트 가능
 
 * 게임 서버에서 실행되는 Task의 대다수는 현재 Tick + 30분 이내에 실행되며 30분 이후에 실행되는 Task는 상대적으로 적다.
   이러한 Task 실행 패턴을 최적화하기 위해 CTaskScheduler는 두 개의 컨테이너를 사용하여 관리
   - Task 실행 시간이 30분 이내인지 여부에 따라 두 개의 컨테이너로 분류하여 스케줄링
     1. __mFixTask[TASK_SLOT_COUNT] 30분 이내 실행할 Task 저장 (고정 크기 배열 + FIFO 큐를 사용해서 빠른접근과 실행)
     2. __mFlexibleTask 30분 이후 실행할 Task 저장 (우선순위 큐 사용해서 메모리 절약과 효율적 실행)
 
 * 게임 서버에서 30분 이내에 실행되는 Task 예시
   - PvP 경기, 이벤트, 월드 보스 등장, 길드전 등 다양한 콘텐츠의 진행 흐름: 
     이벤트 공지 -> 준비 단계 -> 시작 -> 진행 -> 종료 -> 결과 저장 -> 초기화 및 대기 시간 적용
   - 몬스터 리스폰: 죽은 몬스터가 5초, 30초, 5분 후 리스폰
   - 몬스터 AI 행동: 몬스터가 2초마다 플레이어를 감지, 5초 후 스킬 사용, 10초 후 도주 판단	
   - 버프 지속 시간 체크: 플레이어가 받은 버프가 30초, 2분, 5분 후 자동 종료
   - 디버프 제거: 중독, 화상, 저주 효과가 10초, 1분 후 자동 해제
   - 아이템 소멸 타이머: 맵에 드랍된 아이템이 1분, 5분, 10분 후 자동 소멸
   - 던전 타이머: 던전 입장 후 10분, 15분 후 강제 퇴장
   - PvP 경기 시간 종료: PvP 전투가 3분, 10분 후 자동 종료
   - 퀘스트 진행 타이머: 특정 퀘스트에서 1분 후 이벤트 발생, 5분 후 실패
   - 레이드 보스 페이즈 전환: 레이드 보스가 2분 후 페이즈 변경
   - 채팅 제재 해제: 특정 유저가 5분 후 채팅 금지 해제
   - 자동 회복 아이템 발동: 일정 시간 후 자동으로 포션 사용
   - 보스 전투 진입 제한 해제: 보스방 재입장이 10분 후 가능
   - 경매장 아이템 갱신: 경매장 등록 아이템이 5분 후 가격 갱신 가능
   - NPC 이벤트 대기 시간: NPC가 1분 후 다시 대화 가능	
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
	INT	 __mCurSlotIdx; // 현재 실행 중인 Task Slot Index
	__TLockerAuto::TLocker	__mTaskSlotLocker;
	__TLockerAuto::TLocker	__mFlexibleTaskLocker;	
	__TFixTask	__mFixTask[TASK_SLOT_COUNT];
	__TFlexibleTask	__mFlexibleTask;
	CTaskPool	__mTaskPool;
};