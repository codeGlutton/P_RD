# :computer: P_RD (폐허왕국 용병단)

This project is a Rogue-lite SRPG developed using Unreal Engine 5.7.
It features a tactical turn-based action queue system powered by unit attributes and an optimized dual Model-View architecture for both real-time gameplay and high-speed background simulation.

이 프로젝트는 언리얼 엔진 5.7(Unreal Engine 5.7)을 사용하여 개발한 로그라이트 SRPG 게임입니다.
유닛 속성 기반의 전략적 행동 큐 시스템과, 실시간 인게임 연출 및 백그라운드 고속 시뮬레이션을 동시에 지원하는 이원화 Model-View 아키텍처를 구현했습니다.

## :pushpin: Main Architecture (핵심 아키텍처)

### 1. Model-View 이원화 아키텍처 (In-Game vs Simulation)
인게임 렌더링 환경과 백그라운드 고속 연산(시뮬레이션) 환경을 분리하고, 뷰 연출 유무에 따른 성능 및 로직을 최적화하기 위해 **Model-View 이원화 아키텍처**를 설계 및 적용하였습니다.

| 구분 | 인게임 모드 (`In-Game Mode`) | 시뮬레이션 모드 (`Simulation Mode`) |
| :--- | :--- | :--- |
| **생성 팩토리** | `UGameObjectModelFactory` | `USimulationObjectModelFactory` |
| **뷰/액터 구성** | `Model` + `AObjectView` (양방향 바인딩) | 뷰 액터 없음 (Pure C++ 데이터 모델 상주) |
| **시각 연출** | 3D 레벨 스폰, 메시 렌더링 & VFX 연출 | No-Rendering (월드 스폰 오버헤드 0%) |
| **실행 목적 & 성능** | 실시간 플레이어 조작 & 화면 표현 | 백그라운드 100배 고속 연산, AI 예측 & 밸런스 검증 |
| **이벤트 처리** | 화면 연출용 이벤트 실시간 소모 | `USimulationEventLogger`에 상세 턴/액션 로그 축적 |

* **컴포넌트 데이터 모델 (`UComponentModel`)**: 패시브, 장비, 스탯 메커니즘을 유연하게 확장 및 조립할 수 있도록 모듈화된 컴포넌트 모델 구조를 제공합니다.

### 2. SRPG 턴 프레임워크 & Action Queue (`SRPGFramework`)
유닛의 스피드 포인트(Speed Point) 충전 및 요구치 달성에 따라 라운드별 턴 후보(`FSRPGTurnCandidate`)를 동적으로 선출하고 턴 예보(Turn Forecast)를 지원하는 5단계 파이프라인 시스템입니다.

| 단계 | 파이프라인 | 주요 수행 내용 |
| :--- | :--- | :--- |
| **STEP 1** | **Round Evaluation** | 라운드 시작 시 유닛별 스피드 포인트(`SpeedPoint`) 평가 및 누적 충전 |
| **STEP 2** | **Turn Candidate Selection** | 턴 요구 수치 달성 유닛을 턴 후보(`FSRPGTurnCandidate`)로 선출 및 정렬 (동률 발생 시 Random Tie-Breaker 적용) |
| **STEP 3** | **Turn Registration & Cost** | 턴 후보의 스피드 포인트 차감(`UTacticalEffect_SpeedPoint`) 및 라운드 턴 큐(`RegisterTurn`) 등록 |
| **STEP 4** | **Turn Start & Action Queue** | 턴 시작 시 패시브 능력 트리거 후, 플레이어/AI 라우터(`CommandRouterModel`)를 통해 이동(`SRPGMoveAction`) 및 스킬(`SRPGSkillAction`) 행동 적재 |
| **STEP 5** | **Async Execution & State Check** | 스탯 스냅샷/패시브 연산, 행동 비동기 연출 실행, 턴 종료 처리(`OnEndTurn`) 및 전투 상태 평가(`EvaluateCombatStates`) |

### 3. 절차적 노드 방 생성 (PCG Stage System)
* `StageBuilder`를 활용해 보물(Treasure), 상점(Shop), 일반/엘리트 몬스터, 보스(Boss) 방 노드를 동적으로 구성하여 플레이어 선택에 따른 분기형 탐색 경로를 제공합니다.

## :wrench: Tools & Technologies (사용한 기술)
- **Engine** : Unreal Engine 5.7