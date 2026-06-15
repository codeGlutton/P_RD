# 전투 UI ↔ 게임플레이 경계 (View-Model 계약)

데이터/비주얼 분리 구조에 맞춰, 전투 UI와 게임플레이는 **`UCombatViewModel` 하나**로만 만난다.
게임플레이가 리팩토링돼도 이 계약만 지키면 위젯은 안 바뀐다.

```
[게임플레이] UnitData/DiceData/SkillBuildAction …
      │ Set*()  (읽기 데이터 push)        ▲ OnCombatCommand/WorldTouch  (의도 구독)
      ▼                                    │
            ┌──────────  UCombatViewModel  ──────────┐
            │  read 캐시 · 입력 델리게이트 · 행동 큐  │
            └────────────────────────────────────────┘
      │ Get*() / OnViewChanged / OnQueueNodeResolved
      ▼
[UI] UCombatViewWidgetBase 상속 WBP들 (HUD / DicePanel / SkillPanel) — 표시·입력만
```

## 구성
| 파일 | 역할 |
|------|------|
| `CombatViewTypes.h` | 표시용 struct + 행동 1단위 큐 노드 `FCombatQueueNode` |
| `CombatViewModel.h/.cpp` | 경계 허브. **읽기**(`Set*`/`Get*`+`OnViewChanged`) · **주기**(`Request*`→`OnCombatCommand`/`OnCombatWorldTouch`) · **큐**(`SetActionQueue`/`ResolveFrontQueueNode`→`OnQueueNodeResolved`) |
| `CombatViewWidgetBase.h/.cpp` | 위젯 베이스. `BindViewModel()` 후 `OnViewRefreshed`/`OnQueueNodePlayed`만 구현 |
| `MockCombatDriver.h/.cpp` | 게임플레이 없이 가짜 데이터로 UI를 먼저 만들고 테스트하는 개발용 |

### 다루는 도메인 (`ECombatViewDomain`)
| 도메인 | struct | push | read |
|--------|--------|------|------|
| Unit | `FUnitView`(HP·스탯·위치·`mStatusTags`) / `FUnitDetailView` | `SetUnitViews`/`SetUnitDetail` | `GetUnitViews`/`GetUnitDetail` |
| Dice | `FDiceSlotView`(값·희귀도·선택·**`mPreviewTexture` 3D 굴림면**) | `SetDiceViews`/`SetSelectedDice` | `GetDiceViews`/`GetSelectedDice*` |
| Skill | `FSkillView` / `FSkillDetailView` | `SetSkillViews`/`SetSkillDetail` | `GetSkillViews`/`GetSkillDetail` |
| Tile | `FTileViewState`(하이라이트·점유) | `SetTileStates` | `GetTileStates` |
| Turn | `FTurnView`(라운드·페이즈·순서) | `SetTurnView` | `GetTurnView` |
| Equipment | `FEquipmentView` | `SetEquipmentViews` | `GetEquipmentViews` |
| Meta | `FPlayerMetaView`(돈·경험치·레벨) | `SetPlayerMeta` | `GetPlayerMeta` |
| Queue | `FCombatQueueNode` | `SetActionQueue`/`ResolveFrontQueueNode` | `GetActionQueue` |

> 다이스의 3D 굴림 프리뷰는 `CombatDiceCaptureActor`(비주얼)가 렌더타깃에 그리고, 어댑터가
> 그 텍스처를 `FDiceSlotView.mPreviewTexture`로 넣는다. UI는 표시만 — 직접 캡처/회전하지 않는다.

## 박용수(UI) 사용법
1. WBP를 `UCombatViewWidgetBase` 상속으로 만든다.
2. `BindViewModel(VM)` 호출.
3. `OnViewRefreshed(Domain)`에서 `VM->GetDiceViews()` 등을 읽어 자기 화면을 그린다.
4. 탭/터치 → `VM->RequestSelectSkill(i)` / `RequestToggleDice(i)` / `RequestRollDice()` / `RequestWorldTouch(pos,long)` …
5. `OnQueueNodePlayed(Node)`에서 머리 위 숫자·효과를 띄운다.
6. 게임플레이 전이라도 `UMockCombatDriver::Start(VM)`로 굴림·선택·예측 큐 재생까지 테스트.

## 모호재(게임플레이) 사용법
Mock 자리에 **어댑터**만 구현(위젯 무수정):
- 상태가 바뀌면 `VM->SetUnitViews/SetDiceViews/SetSkillViews/SetTileStates/SetTurnView()`.
- 행동/예측 결과를 `VM->SetActionQueue(Queue)`로 주고, 애니 한 단위 끝날 때마다 `VM->ResolveFrontQueueNode()`.
- `VM->OnCombatCommand`/`OnCombatWorldTouch`를 구독해 입력(의도)을 처리.

## 범위 — 이건 '전투' 뷰모델
전투 화면(HUD/Dice/Skill/Tile/Equip)만 담당. 아래는 **전투 밖 UI**라 별도 뷰모델로 같은 패턴 반복:
타이틀·캐릭터선택·프론트엔드맵(방 이동)·설정·**전투 보상/획득**·인벤토리. (이미 대부분 WBP로 존재 → 필요할 때 뷰모델로 감싸면 됨.)

## 미합의/맞출 것
- **큐 노드 묶음 방식**: 한 행동의 효과(데미지·상태이상·힐)를 "한 노드+태그(`mTags`)"로 묶음(현재) vs 분리 — 회의 미확정.
- 게임플레이 타입(`FSRPGSkillBuildPhase`,`UDiceData`,장비/메타 소스 `PersistentData`)과의 실제 매핑은 어댑터에서.
