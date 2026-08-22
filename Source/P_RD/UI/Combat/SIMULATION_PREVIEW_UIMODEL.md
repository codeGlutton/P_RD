# 시뮬레이션 미리보기 뷰모델 / 실전 전투 뷰모델 분리

> PR #552 (커밋 5개: `c00617ad` → `cd1e8658` → `9f4d9d13` → `d7539204` → `2b6edda4`) 의 구조 설명.
> 조준 시 뜨는 예측 표시와 턴 종료 후의 실전 결과 표시가 왜, 어떻게 다른 모델을 쓰는지 정리한다.

## 한 줄 요약

**미리보기는 언제든 통째로 버려져야 해서, 실전과 다른 모델에 담았다.**

- 예측(조준 미리보기) → `USimulationPreviewUIModel` (신설)
- 실전(턴 종료 후 실제 결과) → `UCombatUIModel` (기존)

원문 주석이 이유를 그대로 말한다 (`SimulationPreviewUIModel.h:7`):

> 예측은 실제와 다를 수 있고, 무르면(취소/새 조준) 실전 표시를 건드리지 않고
> 통째로 버려져야 한다. 한 모델에 실전과 예측을 같이 담으면 "버린다"가 곧
> 실전 상태 오염이 된다.

## 왜 나눴나 — 논거 3개

1. **버림(discard)의 안전성** — 미리보기는 무르기/재조준마다 폐기된다. 실전과 같은
   저장 자리를 쓰면 폐기가 곧 실전 데이터 파괴다.
2. **게임플레이 층과의 대칭** — `USimulationSubsystem` 이 이미
   `mGameRoomContext` / `mSimulationRoomContext` 로 컨텍스트를 이원화해 뒀다.
   UI 층이 같은 모양을 따라간 것 (새 패턴 발명이 아니라 기존 패턴 확장).
   `ClearPreview()` 는 `mSimulationRoomContext.mRoomInstance = nullptr` 의 UI 판이다.
3. **낡은 예측 유입 차단** — 조준을 연달아 바꾸면 예측 결과가 순서 뒤바뀌어
   도착할 수 있다. 세대(Generation) 번호로 거른다.

부수 효과: `ECombatEventDataSourceUI` 는 라우팅 분기에서 **기록용 태그로 강등**됐다.
라우팅은 이제 "어느 모델이냐"가 담당한다 (`CombatUITypes.h` 주석 참조).

## 커밋 구성 — 항상 동작하는 상태로 갈아끼우기

| 커밋 | 역할 |
|---|---|
| `c00617ad` (1/4) | 모델·타입 신설 — 아직 아무도 안 씀 |
| `cd1e8658` (2/4) | GameMode 생산 경로 분리 + **임시 이중기록** (HUD가 아직 실전 모델만 구독하므로 미리보기를 실전 모델에도 흘림) |
| `9f4d9d13` (3/4) | HUD가 예측 모델 구독 시작 |
| `d7539204` (4/4) | **이중기록 제거** — 경로 완전 분리 |
| `2b6edda4` (test) | 계약 3종 테스트로 고정 |

2/4 에서 일부러 이중기록을 넣고 4/4 에서 걷어낸 것이 포인트 — 생산자→소비자
순서로 바꾸는 동안 어느 커밋에서 빌드해도 화면이 깨지지 않는다.

## 구조

### USimulationPreviewUIModel

- `UCombatUIModel` 이 1개를 **지연 생성**으로 소유 (`CombatUIModel.cpp` 의
  `GetSimulationPreviewUIModel()`). GameMode 생성자(CDO 구성 중)에서는
  `NewObject` 가 막히기 때문.
- 보유 상태(전부 Transient): `mIsActive`, `mGeneration`,
  `mPreviewEventBatch`(미리보기 로그), `mPredictedUnits`(유닛별 예측),
  `mPreviewPendingAction`(AP 예정 소모)
- 델리게이트 3종: `OnPreviewChanged(도메인)` · `OnPreviewEventBatch(배치)` ·
  `OnPreviewCleared()`
- **세대 메커니즘**: `BeginPreview()` 가 세대를 +1 하고 번호를 돌려준다.
  모든 `Set*(Generation, ...)` 은 세대가 다르면 *알림조차 없이* 버린다.
  `ClearPreview()` 도 세대를 올려, 비행 중이던 낡은 결과가 빈 자리를 다시
  채우지 못하게 한다. 겹쳐 불려도 `Cleared` 알림은 한 번만 나간다.

### ACombatGameMode — 생산 경로 분기

옛 `PushSimulationFloatingLogs(bool IsPreview)` 하나가 셋으로 쪼개졌다.
bool 파라미터 하나가 겸하던 두 책임을 분해한 것:

| 함수 | 역할 |
|---|---|
| `BuildCombatFloatingLogRequests` | 중립 빌더. 로그→요청 변환만. 인덱스 결속 여부는 `bBindMotionIndices` 가, 표시 규칙(`mIsPreview`)은 **호출 경로가** 정한다 |
| `PushSimulationPreviewUIData` (예측) | `BeginPreview` → 빌드(인덱스 결속 O) → 전부 `mIsPreview=true` → 예측 모델에 Set |
| `PushCombatEventUIData` (실전) | 빌드(결속 X) → `mIsPreview=false` 그대로 → `SetCombatEventBatch(LiveCombat, ...)` |

## 데이터 흐름

### A. 예측 — 조준할 때마다

```
조준 시뮬 완료 (OnPostSimulateSkillAction)
  → PushSimulationPreviewUIData
      BeginPreview() → 세대 +1 / 요청 전부 mIsPreview = true
  → USimulationPreviewUIModel.SetPreviewEventBatch / SetPredictedUnits
      (세대 불일치면 여기서 조용히 폐기)
  → HUD::HandleSimulationPreviewBatch (CombatLayoutHUDWidget_CombatLog.cpp)
      이전 미리보기만 걷고(실전 보존) → 큐 우회 즉시 스폰
  → 화면: 고정 예측 숫자 (수명으로 안 사라짐)
```

### B. 미리보기 폐기 — 트리거 3개가 한 지점으로

```
무름(OnCancelSimulateSkillAction) / 행동 시작(OnBeginAnyTurnActionUI) / 턴 종료(OnEndAnyTurnUI)
  → ClearPreview()   — mIsActive=false, 세대 +1
  → HUD: mIsPreview==true 인 로그만 퇴장 (RetireSimulationPreviewFloatingLogs)
  ※ UCombatUIModel(실전)은 전혀 관여하지 않음
```

### C. 실전 — 턴이 끝났을 때만

```
턴 종료 (OnEndAnyTurnUI)
  ① 먼저 ClearPreview() 로 예측 폐기 (B)
  ② ConsumeGameEventLogs() → PushCombatEventUIData
  → UCombatUIModel.SetCombatEventBatch(LiveCombat, ...)
  → HUD: 대기 큐 적재 → 0.28s 간격 스태거 스폰
  → 화면: 1.2s 수명 juice, 떠오르며 페이드아웃
```

## 두 경로 대비

| | 예측 | 실전 |
|---|---|---|
| 트리거 | 조준 시뮬 완료 | 턴 종료 |
| 모델 | `USimulationPreviewUIModel` | `UCombatUIModel` |
| 인덱스 결속 | O (`bBindMotionIndices=true`) | X |
| `mIsPreview` | true | false |
| 스폰 | 즉시, 스태거 없음 | 큐 + 0.28s 간격 |
| 소멸 | MotionFinished / ClearPreview | 수명 1.2s 자동 |
| 통째 폐기 | O (수시로) | X |
| 세대 검증 | O | X |

## 테스트 계약 3종 (SimulationPreviewUIModelTests.cpp)

논거 3개와 1:1 로 대응한다:

| 테스트 | 고정하는 것 |
|---|---|
| `P_RD.UI.Combat.SimulationPreviewIsolation` | 미리보기 set/clear 가 실전 배치의 리비전·로그 수·내용·알림 횟수를 못 건드림 (= 격리) |
| `...SimulationPreviewGeneration` | 낡은 세대 배치는 알림 없이 폐기, 현재 세대만 반영 (= 낡은 유입 차단) |
| `...SimulationPreviewLifecycle` | 열기 전 Clear 무음, 겹친 Clear 에도 알림 1번 (= 중복 취소 안전) |

## 이어받을 지점 (자리만 깔려 있는 것)

- `mPredictedUnits`(`FUnitPredictionUI`: HP 델타·사망 여부·예측 타일)는 GameMode 가
  채우지만 **HUD 소비자가 아직 없다** — 유닛 머리 위 HP 증감 예고 같은 표현이
  다음 작업 후크다.
- `FUnitPredictionUI::mPredictedStatuses` 는 의도적으로 비워 둠 — 이벤트 로그가
  델타만 줘서 절대 스택 수를 지어낼 수 없다 (주석에 근거).
- `NotifyCombatFloatingLogsCleared` 는 C++ 호출부가 0이지만 BlueprintCallable
  공개 API·전체 클리어 안전망으로 **의도적 잔존** (4/4 커밋 메시지 근거).

## 근거 파일

- `Source/P_RD/UI/Combat/SimulationPreviewUIModel.h` / `.cpp`
- `Source/P_RD/UI/Combat/CombatUIModel.h` / `.cpp`
- `Source/P_RD/UI/Combat/CombatLayoutHUDWidget_CombatLog.cpp`
- `Source/P_RD/GameMode/CombatGameMode.cpp`
- `Source/P_RDTests/UI/SimulationPreviewUIModelTests.cpp`
