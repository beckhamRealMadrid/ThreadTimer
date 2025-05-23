# ⏱️ ThreadTimer System

![C++](https://img.shields.io/badge/C%2B%2B-High%20Performance-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)

---

## 🚩 **프로젝트 개요**

**ThreadTimer**는 대규모 온라인 게임 서버의 시간 기반 이벤트 처리를 정밀하고 효율적으로 수행하기 위해 설계된 **고성능 타이머 및 작업 스케줄링 시스템**입니다.

기존 서버는 콘텐츠 타이머, 주기적 초기화, One-shot 알림 등을 메인 루프에서 처리하며 다음과 같은 문제가 발생했습니다:

* 각 기능별 타이머 로직이 분산되어 있어 유지보수가 어려움
* 폴링 기반 처리로 CPU 낭비 심각
* 수천 개 이상의 타이머를 정확하게 제어하기 어려움

이를 해결하기 위해 본 시스템은 다음 목표로 설계되었습니다:

* 고정밀 **WaitableTimer 기반 중앙 타이머 큐** ✅
* IOCP 기반 **비동기 이벤트 분산 처리** ✅
* **타이머/핸들러 전용 스레드** 분리로 안정성과 확장성 확보 ✅
* **정기/One-shot/유저 타이머까지 통합 관리** 가능한 구조 ✅

### 실서비스 적용 결과:

* 서버 내 100종 이상의 타이머를 초 단위 정밀도로 운영
* CPU 사용률 60\~80% 감소 (기존 폴링 대비)
* 타이머별 로깅 자동화로 운영 효율 향상
* 이벤트 지연 및 누락률 감소 → 운영 리스크 제거

---

## 🛠️ **구성 요소**

| 구성 요소            | 설명 |
|--------------------|-------|
| **CThreadTimer**   | ✅ IOCP 기반 멀티스레드 타이머 시스템.<br>✅ 정밀 주기 타이머 및 One-shot 타이머 지원.<br>✅ 타이머 ID별 핸들러 매핑 방식. |
| **CTaskScheduler** | ✅ 예약 실행 작업(Task) 스케줄링 시스템.<br>✅ 타이머와 연동되어 실시간 스케줄링 처리.<br>✅ 30분 이하/초과 작업 이중 관리 방식. |

---

## ✨ **주요 특징**

- 🚀 **IOCP 기반 비동기 타이머:** 높은 동시성 & 낮은 지연 시간 보장.
- 🔄 **주기 & 단발 타이머 지원:** 유연한 타이머 등록.
- 🛡️ **완벽한 스레드 안전성:** `CLocker`, `CLockerAuto` 등 커스텀 락 시스템.
- ⏱️ **드리프트 보정 로직:** 누적 지연 없이 정확한 주기 유지.
- 📋 **로깅 기능:** 실시간 이벤트 추적 가능.

---

## 🚀 **빠른 시작 가이드**

### ▶️ 타이머 및 스케줄러 초기화

```cpp
// 1️⃣ 작업 스케줄러 열기
CTaskScheduler::This().Open();

// 2️⃣ 타이머 스레드 시작
CThreadTimer::This().StartThread();
````

### 🛎️ 타이머 등록 예제

```cpp
// ON_TIMER_EVERY_HOUR 타이머 등록 (1시간 주기)
CThreadTimer::This().RegisterTimer(
    ON_TIMER_EVERY_HOUR,  // 타이머 ID
    1000,                 // 최초 실행까지 대기 시간 (1초)
    3600000               // 반복 주기 (1시간)
);
```

### 🛑 서버 종료 시 안전하게 정지

```cpp
CThreadTimer::This().StopThread();
CTaskScheduler::This().Close();
```

---

## 🖥️ **테스트 커맨드 예제**

CLI(커맨드 라인) 테스트 프로그램이 포함돼 있습니다.

| 커맨드      | 설명                   |
| -------- | -------------------- |
| `status` | 타이머 & 스케줄러 스레드 상태 출력 |
| `stop`   | 모든 시스템 안전 종료         |

**예시:**

```
> status
[Status] Timer: RUNNING, TaskScheduler: OPEN
> stop
Stopping CThreadTimer and CTaskScheduler...
All systems stopped successfully.
```

---

## 📂 **프로젝트 디렉토리 구조**

```
ThreadTimer/
├── include/
│   ├── CCpp.h
│   ├── CLocker.h
│   ├── CTask.h
│   ├── CTaskScheduler.h
│   ├── CThreadTimer.h
│   ├── CTime.h
│   ├── CTimer.h
├── src/
│   ├── CCpp.cpp
│   ├── CLocker.cpp
│   ├── CTask.cpp
│   ├── CTaskScheduler.cpp
│   ├── CThreadTimer.cpp
│   ├── CTime.cpp
│   ├── CTimer.cpp
├── ThreadTimer.cpp (테스트 메인 파일)
```

---

## 🛠️ **커스텀 타이머 추가하기**

1️⃣ `ETimerID` enum에 새 타이머 ID 추가
2️⃣ `__SetTimerHandler()` 함수에 핸들러 매핑 추가
3️⃣ 새 핸들러 함수 구현

```cpp
// 예시: 새 타이머 핸들러 매핑
__mTimerHandler[ON_TIMER_MY_CUSTOM] = &CThreadTimer::__OnTimerMyCustom;
```

---

## 📈 **실전 적용 사례**

이 시스템은 실시간 MMO 서버 환경에서 다음과 같은 요구사항을 충족하도록 설계되었습니다:

* 수천 명 동시 접속 환경에서 **안정적 타이머 이벤트 제공**
* **작업 지연 없이** 딜레이 태스크 정확 실행
* 긴 주기의 타이머도 **정확한 시간 유지** (ex: 매일/매시간 이벤트)

---
