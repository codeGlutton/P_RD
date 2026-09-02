# :computer: P_RD (폐허왕국 용병단)
<div align="center">
<img src="https://github.com/user-attachments/assets/placeholder-banner.png"/>
</div>

This project is a Rogue-lite SRPG developed using Unreal Engine 5.
It features a tactical action-building mechanism based on dice rolls and an optimized dual Model-View architecture for both real-time gameplay and high-speed background simulation.

이 프로젝트는 언리얼 엔진 5(Unreal Engine 5)를 사용하여 개발한 로그라이트 SRPG 게임입니다.
주사위 굴리기를 통한 전략적 행동 배치 메커니즘과, 실시간 인게임 연출 및 백그라운드 고속 시뮬레이션을 동시에 지원하는 이원화 Model-View 아키텍처를 구현했습니다.

## :pushpin: Main Architecture (핵심 아키텍처)
<div align="center">
<img src="https://github.com/user-attachments/assets/placeholder-architecture.png"/>
</div>

### 1. Model-View 이원화 아키텍처 (In-Game vs Simulation)
인게임 렌더링 환경과 백그라운드 고속 연산(시뮬레이션) 환경을 분리하고, 뷰 연출 유무에 따른 성능 및 로직을 최적화하기 위해 **Model-View 이원화 아키텍처**를 설계 및 적용하였습니다.

* **인게임 모드 (`RunningGame`)**: `UGameObjectModelFactory`를 통해 논리 모델(`Model`)과 표현 액터 뷰(`AObjectView`)를 양방향 바인딩하여 실시간 시각적 연출 및 플레이어 조작을 처리합니다.
* **시뮬레이션 모드 (`RunningSimulation`)**: `USimulationSubsystem` 및 `USimulationObjectModelFactory`를 활용해 레벨 스폰 및 렌더링 오버헤드 없이 순수 C++ 데이터 모델만으로 백그라운드에서 초고속 연산을 수행합니다 (AI 연산 예측, 전투 시뮬레이션 및 로깅).
* **컴포넌트 데이터 모델 (`UComponentModel`)**: 패시브, 장비, 스탯 메커니즘을 유연하게 확장 및 조립할 수 있도록 모듈화된 컴포넌트 모델 구조를 제공합니다.

### 2. SRPG 턴 프레임워크 & Action Queue (`SRPGFramework`)
* 원형 리스트 기반으로 유닛 순서를 동적으로 관리하며, 턴 시작 시 주사위 굴리기(Dice Roll)를 통해 이동 및 스킬 행동 포인트를 수급합니다.
* 이동(`SRPGMoveAction`), 스킬(`SRPGSkillAction`) 등의 행동을 Action 큐에 적재하여 비동기 처리하며, 스탯 스냅샷 및 버프/패시브 파이프라인을 통해 정교한 턴 종합 연산을 실행합니다.

### 3. 절차적 노드 방 생성 (PCG Stage System)
* `StageBuilder`를 활용해 보물(Treasure), 상점(Shop), 일반/엘리트 몬스터, 보스(Boss) 방 노드를 동적으로 구성하여 플레이어 선택에 따른 분기형 탐색 경로를 제공합니다.

## :wrench: Tools & Technologies (사용한 기술)
- **Engine** : Unreal Engine 5.7