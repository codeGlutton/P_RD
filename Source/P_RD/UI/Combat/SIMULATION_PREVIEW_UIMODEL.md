# 시뮬레이션 미리보기 뷰모델 / 실전 전투 뷰모델 분리

> PR #552 의 UI 데이터 경로 분리를 설명하는 문서.
> **이 분리는 새 패턴이 아니라, 모호재님이 게임플레이 층에 세워 둔 Model–View
> 구조와 "예측은 실전에 쓰기를 하지 않는다" 원칙을 UI 층에 한 번 더 적용한 것**이다.
> 그래서 이 문서는 기존 구조를 먼저 정리하고, 그 위에서 UI 쪽 분리를 설명한다.

---

## 1부. 기존 구조 — 게임플레이 Model–View 프레임워크

### 1-1. 모델과 뷰의 분리

게임의 상태·규칙 전부는 월드에서 떼어낸 순수 `UObject` 그래프에 산다:

```
UObjectModel (ObjectModel.h)                IObjectView (ObjectView.h)
  └ UActorModel                               └ IActorView
      └ UBoardActorModel                          └ AActor 구현체들
          ├ UUnitModel        ←→  AUnit (IActorView + IBoardCombatTargetView)
          └ UObstacleModel
              └ UCombatTargetObstacleModel ←→ ACombatTargetObstacle
  └ 서브시스템 모델
      USRPGCombatModel        ←→  USRPGCombatSubsystem
      USRPGCommandRouterModel ←→  USRPGCommandRouterSubsystem
      UTacticalFrameworkModel ←→  UTacticalFrameworkSubsystem
```

역할 규약:

| | 모델 (`UObjectModel` 파생) | 뷰 (액터/서브시스템, `IObjectView`) |
|---|---|---|
| 책임 | 상태·규칙·턴 진행·속성·스킬 판정 | 메시·애니메이션·VFX·카메라 |
| 참조 | 뷰를 **약참조**로만 (`mView`) | 모델을 **약참조**로만 |
| 존재 | 항상 존재 | **인게임에서만** (시뮬 중엔 없음) |

- 모델은 뷰가 없어도 완전히 동작한다.
- 뷰는 "누구를 그릴지" 스스로 결정하지 않는다 — `BindModel` 로 통보받는다.
- 인터페이스가 **모델측/뷰측 쌍**으로 대칭이다: `IBoardCombatTarget`(속성/스킬/이동
  컴포넌트 *모델* 반환) ↔ `IBoardCombatTargetView`(애니/VFX/이동연출 *컴포넌트* 반환).
  커밋 `9f68ec33`(CombatTarget 장애물 모델·뷰 베이스 동시 생성)이 전형적 사례.
- 모델↔뷰 시간 동기화는 `FPresentationBarrier`(RAII 배리어, 커밋 `ab39b662`)로 —
  모델이 뷰의 연출 길이를 모른 채 기다릴 수 있다.

### 1-2. 연결은 코드가 아니라 설정

모델↔뷰 연결은 `DefaultGame.ini` 선언 + 팩토리가 담당한다:

```ini
+mSubsystemModelViewMappings=(mModelClass="/Script/P_RD.SRPGCombatModel", mViewClass="/Script/P_RD.SRPGCombatSubsystem")
+mWorldModelViewMappings=(mModelClass=".../BP_KnightPlayerUnitModel_C", mViewClass=".../BP_KnightPlayerUnit_C")
```

- `UGameObjectModelFactory` 가 모델 생성 → 매핑 조회 → 뷰 스폰/조회 →
  `View->BindModel(Model)` + `Model->PostBindView(View)` 양방향 바인딩
  (`Simulation/Factory/ObjectModelFactory.cpp`).
- `GetWorldSubsystemModel<T>()` (RDMinimal.h) 하나로 어디서든 모델 접근 — 이 함수는
  `mCurrentRoomContext` 를 경유하므로 **호출자는 자기가 실전인지 예측인지 알 필요가 없다.**

### 1-3. 실전/예측 이중 컨텍스트 ★ 이 문서의 원형

`USimulationSubsystem` 이 컨텍스트를 이원화해 뒀다:

```cpp
FRoomContext* mCurrentRoomContext;      // 둘 중 하나를 가리킴
FRoomContext  mGameRoomContext;         // 실전: GameFactory(뷰 스폰 O) + GameEventLogger
FRoomContext  mSimulationRoomContext;   // 예측: SimFactory(뷰 스폰 X) + SimEventLogger
```

전환(`SetSimulationState`, SimulationSubsystem.cpp:90-104)이 아키텍처의 심장이다:

```cpp
// 예측 진입: 실전 모델 그래프를 통째로 딥카피
mSimulationRoomContext.mRoomInstance =
    Cast<URoomInstance>(StaticDuplicateObject(mGameRoomContext.mRoomInstance, this));
// 예측 종료: 롤백하지 않는다. 통째로 버린다.
mSimulationRoomContext.mRoomInstance = nullptr;
```

**예측이 실전을 오염시키지 못하는 3중 장치:**

1. 모델 전체를 `StaticDuplicateObject` 로 복제 — 진짜 모델을 건드릴 수가 없다.
   (모델을 전부 UObject 파생으로 만든 이유가 이것 — 커밋 `f1a85d30` "복제 가능하도록")
2. `mRoomInstance = nullptr` 로 통째 폐기 — 롤백 로직이 없다. 버리고 GC 에 맡긴다.
3. `USimulationObjectModelFactory` 는 뷰 스폰 훅이 **빈 함수** — 예측 중 유닛이 죽고
   이펙트가 터져도 화면에는 아무 일도 없다.

그리고 값싼 예측은 시뮬레이션조차 쓰지 않는다 — 커밋 `0bb410bc`
"시뮬레이션을 사용하지 않는 턴 예측": UI 계층에 있던 임시 라운드 미리보기 63줄을
지우고 **모델의 `const` 질의**(`GetValidRoundAndOrderedTurnCandidates`, 저장된
시드로 로컬 난수 스트림)로 대체했다. 예측 수단이 두 층이다:

| 예측 대상 | 수단 | 비용 |
|---|---|---|
| 턴 순서 | 모델의 `const` 질의 (사본 위 계산) | 싸다 |
| 스킬 결과 전체 | `StaticDuplicateObject` + 시뮬 컨텍스트 | 비싸다 |

**두 경우 모두 불변식은 하나 — 예측 경로는 실전 상태에 쓰기를 하지 않는다.**

> 저작 경위: `ObjectModel.h` 파일은 이문환님이 먼저 놓았고(`9889189e`), 오늘의
> API 전체(Bind/Unbind/PostBindView/GetView/FinishCreating/ModelId)는 모호재님의
> `ddd90bbe`(63파일)와 후속 커밋에서 확정됐다. `ObjectView/ActorView/ActorModel/
> ModelViewMapping/RoomContext/RoomInstance/SimulationSubsystem` 은 파일 자체가
> 모호재님 저작(@author 모호재). 개념 문서는 `Source/P_RD/ModelViewFramework.md`
> (단, 식별자는 코드가 최신).

---

## 2부. PR #552 — 같은 원칙을 UI 층에 적용

### 2-1. 문제

UI 뷰모델(`UCombatUIModel`, UI 계층 소관)은 하나였는데, 담기는 데이터는 두 종류였다:

- **예측**: 조준할 때마다 뜨는 플로팅 로그·유닛 예측 — 무르기/재조준마다 통째로 버려져야 함
- **실전**: 턴 종료 후 실제 결과 — 절대 버려지면 안 됨

한 모델에 같이 담으면 "버린다"가 곧 실전 상태 오염이다. 게임플레이 층이
`mGameRoomContext` / `mSimulationRoomContext` 를 나눈 것과 정확히 같은 문제.

### 2-2. 해법 — 대응표

| 게임플레이 층 (기존) | UI 층 (PR #552) |
|---|---|
| `mGameRoomContext` / `mSimulationRoomContext` | `UCombatUIModel` / `USimulationPreviewUIModel` |
| `mSimulationRoomContext.mRoomInstance = nullptr` | `ClearPreview()` — 예측 쪽만 통째 비움 |
| `StaticDuplicateObject` 로 세계 격리 | 별도 UObject 인스턴스로 저장 자리 격리 |
| SimFactory 가 뷰를 안 만듦 (화면 무흔적) | 전용 델리게이트(`OnPreviewEventBatch/Cleared`)로 실전 표시와 분리 |
| `UGameEventLogger` / `USimulationEventLogger` | `mSource == LiveCombat` / `SimulationPreview` (기록용 태그) |
| `checkf(재진입 금지)` 가드 | **세대(Generation)** — 낡은 `Set*()` 결과를 알림 없이 폐기 |

마지막 줄의 세대 메커니즘만 UI 고유의 추가분이다 — 비동기 UI 에서는 조준을
연달아 바꾸면 예측 결과가 순서 뒤바뀌어 도착할 수 있어서, `BeginPreview()` 가
세대를 +1 하고 세대가 다른 결과는 조용히 버린다.

### 2-3. 생산 경로 분기 (ACombatGameMode = 두 세계의 유일한 어댑터)

`ACombatGameMode` 는 원래부터 모델 세계(`GetWorldSubsystemModel<T>`)와 UI 뷰모델
세계(`mCombatUIModel->Set*`)를 잇는 유일한 어댑터다. PR #552 는 이 어댑터의
생산 함수를 경로별로 쪼갰다 — 옛 `PushSimulationFloatingLogs(bool IsPreview)` 하나가:

| 함수 | 역할 |
|---|---|
| `BuildCombatFloatingLogRequests` | 중립 빌더. 변환만. 표시 규칙은 **호출 경로가** 정한다 |
| `PushSimulationPreviewUIData` (예측) | `BeginPreview` → 전부 `mIsPreview=true` → 예측 모델 |
| `PushCombatEventUIData` (실전) | `mIsPreview=false` 그대로 → `SetCombatEventBatch(LiveCombat, ...)` |

### 2-4. 데이터 흐름

```
[예측 — 조준할 때마다]
조준 시뮬 완료 → PushSimulationPreviewUIData
  → USimulationPreviewUIModel (세대 불일치면 조용히 폐기)
  → HUD: 이전 미리보기만 걷고(실전 보존) 즉시 스폰
  → 화면: 고정 예측 숫자

[폐기 — 무름/행동 시작/턴 종료 3개 트리거가 한 지점으로]
  → ClearPreview() (겹쳐 불려도 알림 1번)
  → HUD: mIsPreview==true 인 로그만 퇴장
  ※ UCombatUIModel(실전)은 전혀 관여하지 않음

[실전 — 턴이 끝났을 때만]
턴 종료 → 먼저 ClearPreview() → ConsumeGameEventLogs() → PushCombatEventUIData
  → UCombatUIModel → HUD: 큐 + 0.28s 스태거 → 1.2s 수명 juice
```

(`ConsumeGameEventLogs` 는 기존 API — "시뮬레이션 컨텍스트에는 영향을 주지 않는다"
주석 그대로, 실전 로그만 UI 어댑터가 가져간다.)

### 2-5. 두 경로 대비

| | 예측 | 실전 |
|---|---|---|
| 트리거 | 조준 시뮬 완료 | 턴 종료 |
| 모델 | `USimulationPreviewUIModel` | `UCombatUIModel` |
| 인덱스 결속 | O (`bBindMotionIndices=true`) | X |
| `mIsPreview` | true | false |
| 스폰 | 즉시, 스태거 없음 | 큐 + 0.28s 간격 |
| 소멸 | MotionFinished / ClearPreview | 수명 1.2s 자동 |
| 통째 폐기 / 세대 검증 | O / O | X / X |

### 2-6. 커밋 구성 — 항상 동작하는 상태로 갈아끼우기

| 커밋 | 역할 |
|---|---|
| `c00617ad` (1/4) | 모델·타입 신설 — 아직 아무도 안 씀 |
| `cd1e8658` (2/4) | GameMode 생산 분리 + **임시 이중기록** (HUD가 아직 실전만 구독) |
| `9f4d9d13` (3/4) | HUD 가 예측 모델 구독 시작 |
| `d7539204` (4/4) | **이중기록 제거** — 경로 완전 분리 |
| `2b6edda4` (test) | 계약 3종 고정 |

### 2-7. 테스트 계약 3종 (SimulationPreviewUIModelTests.cpp)

| 테스트 | 고정하는 것 |
|---|---|
| `...SimulationPreviewIsolation` | 미리보기 set/clear 가 실전 배치를 못 건드림 (= 격리) |
| `...SimulationPreviewGeneration` | 낡은 세대는 알림 없이 폐기 (= 낡은 유입 차단) |
| `...SimulationPreviewLifecycle` | 겹친 Clear 에도 알림 1번 (= 중복 취소 안전) |

## 이어받을 지점 (자리만 깔려 있는 것)

- `mPredictedUnits`(`FUnitPredictionUI`: HP 델타·사망 여부·예측 타일)는 GameMode 가
  채우지만 **HUD 소비자가 아직 없다** — 유닛 머리 위 HP 증감 예고 등이 다음 후크.
- `FUnitPredictionUI::mPredictedStatuses` 는 의도적으로 비워 둠 — 이벤트 로그가
  델타만 줘서 절대 스택 수를 지어낼 수 없다.
- `NotifyCombatFloatingLogsCleared` 는 C++ 호출부 0이지만 BlueprintCallable 공개
  API·전체 클리어 안전망으로 의도적 잔존 (4/4 커밋 메시지 근거).

## 근거 파일

- 게임플레이 층: `Source/P_RD/ObjectModel.h` · `ObjectView.h` · `Actor/ActorView.h` ·
  `Setting/ModelViewMapping.h` · `Simulation/Factory/ObjectModelFactory.cpp` ·
  `Singleton/WorldSubsystem/SimulationSubsystem.h/.cpp` ·
  `Singleton/WorldSubsystem/SRPGCombatModel.cpp` (턴 예측 const 질의) ·
  `ModelViewFramework.md`
- UI 층: `UI/Combat/SimulationPreviewUIModel.h/.cpp` · `CombatUIModel.h/.cpp` ·
  `CombatLayoutHUDWidget_CombatLog.cpp` · `GameMode/CombatGameMode.cpp` ·
  `P_RDTests/UI/SimulationPreviewUIModelTests.cpp`
