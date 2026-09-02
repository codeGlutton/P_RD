# :computer: P_RD (폐허왕국 용병단)

이 프로젝트는 언리얼 엔진 5.7(Unreal Engine 5.7)을 사용하여 개발한 로그라이트 SRPG 게임입니다.
Slay the Spire를 모티브로 한 방 탐색 구조에 턴제 기반 SRPG 전투 시스템을 융합했습니다.

## :pushpin: Main Architecture (핵심 아키텍처)

### 1. Model-View 이원화 아키텍처 (In-Game vs Simulation)
인게임 렌더링 환경과 백그라운드 고속 연산(시뮬레이션) 환경을 분리하고, 뷰 연출 유무에 따른 성능 및 로직을 최적화하기 위해 **Model-View 이원화 아키텍처**를 설계 및 적용하였습니다.

| 구분 | 인게임 모드 (`In-Game Mode`) | 시뮬레이션 모드 (`Simulation Mode`) |
| :--- | :--- | :--- |
| **생성 팩토리** | `UGameObjectModelFactory` | `USimulationObjectModelFactory` |
| **뷰/액터 구성** | `Model` + `AObjectView` (양방향 바인딩) | 뷰 액터 없음 (Pure C++ 데이터 모델 상주) |
| **시각 연출** | 3D 레벨 스폰, 메시 렌더링 & VFX 연출 | No-Rendering (랜더링 오버헤드 제거) |
| **실행 목적 & 성능** | 실시간 플레이어 조작 & 화면 표현 | 턴 진행, 스킬 결과 예측 |
| **이벤트 처리** | 화면 연출용 이벤트 실시간 소모 | `USimulationEventLogger`에 상세 턴/액션 로그 축적 |

### 2. SRPG 턴 프레임워크 & Action Queue (`SRPGFramework`)
유닛의 스피드 포인트(Speed Point) 충전 및 요구치 달성에 따라 라운드별 턴 후보(`FSRPGTurnCandidate`)를 동적으로 선출하고 차례로 진행하는 5단계 파이프라인 시스템입니다.

| 단계 | 파이프라인 | 주요 수행 내용 |
| :--- | :--- | :--- |
| **STEP 1** | **Round Evaluation** | 라운드 시작 시 유닛별 스피드 포인트(`SpeedPoint`) 평가 및 누적 충전 |
| **STEP 2** | **Turn Candidate Selection** | 턴 요구 수치 달성 유닛을 턴 후보(`FSRPGTurnCandidate`)로 선출 및 정렬 (동률 발생 시 Random Tie-Breaker 적용) |
| **STEP 3** | **Turn Registration & Cost** | 턴 후보의 스피드 포인트 차감(`UTacticalEffect_SpeedPoint`) 및 라운드 턴 큐(`RegisterTurn`) 등록 |
| **STEP 4** | **Turn Start & Action Queue** | 턴 진행 및 플레이어/AI 라우터(`CommandRouterModel`)를 통해 입력된 이동(`SRPGMoveAction`) 및 스킬(`SRPGSkillAction`) 행동 적재 |
| **STEP 5** | **Async Execution & State Check** | 액션 및 턴 종료 처리(`OnEndTurn`)시에 전투 상태 평가(`EvaluateCombatStates`)로 전투 종료 판단 |

### 3. 절차적 노드 방 생성 (PCG Stage System)
* `StageBuilder`를 활용해 보물(Treasure), 상점(Shop), 일반/엘리트 몬스터, 보스(Boss) 방 노드로 절차적으로 월드맵을 구성하여 플레이어가 선택가능한 다양한 경로를 제공합니다.

## :wrench: Tools & Technologies (사용한 기술)
- **Engine** : Unreal Engine 5.7
