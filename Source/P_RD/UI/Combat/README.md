# 전투 UI ↔ 게임플레이 경계 (View-Model 계약)

데이터/비주얼 분리 구조에 맞춰, 전투 UI와 게임플레이는 **`UCombatUIModel` 하나**로만 만난다.
게임플레이가 리팩토링돼도 이 계약만 지키면 위젯은 안 바뀐다.

```
[게임플레이] UnitData/SkillBuildAction …
      │ Set*()  (읽기 데이터 push)        ▲ OnCombatCommand/WorldTouch  (의도 구독)
      ▼                                    │
            ┌──────────  UCombatUIModel  ──────────┐
            │  read 캐시 · 입력 델리게이트 · 행동 큐  │
            └────────────────────────────────────────┘
      │ Get*() / OnUIChanged / OnQueueNodeResolved
      ▼
[UI] UCombatUIWidgetBase 상속 WBP들 (HUD / SkillPanel) — 표시·입력만
```

## 구성
| 파일 | 역할 |
|------|------|
| `CombatUITypes.h` | 표시용 struct + 행동 1단위 큐 노드 `FCombatQueueNode` |
| `CombatUIModel.h/.cpp` | 경계 허브. **읽기**(`Set*`/`Get*`+`OnUIChanged`) · **주기**(`Request*`→`OnCombatCommand`/`OnCombatWorldTouch`) · **큐**(`SetActionQueue`/`ResolveFrontQueueNode`→`OnQueueNodeResolved`) |
| `CombatUIWidgetBase.h/.cpp` | 위젯 베이스. `BindUIModel()` 후 `OnUIRefreshed`/`OnQueueNodePlayed`만 구현 |
| `MockCombatDriver.h/.cpp` | 게임플레이 없이 가짜 데이터로 UI를 먼저 만들고 테스트하는 개발용 |

### 다루는 도메인 (`ECombatUIDomain`)
| 도메인 | struct | push | read |
|--------|--------|------|------|
| Unit | `FUnitUI`(HP·스탯·위치·`mStatusTags`) / `FUnitDetailUI` | `SetUnitUIs`/`SetUnitDetail` | `GetUnitUIs`/`GetUnitDetail` |
| Skill | `FSkillUI` / `FSkillDetailUI` | `SetSkillUIs`/`SetSkillDetail` | `GetSkillUIs`/`GetSkillDetail` |
| Turn | `FTurnUI`(라운드·페이즈·순서) | `SetTurnUI` | `GetTurnUI` |
| Equipment | `FEquipmentUI` | `SetEquipmentUIs` | `GetEquipmentUIs` |
| Meta | `FPlayerMetaUI`(돈·경험치·레벨) | `SetPlayerMeta` | `GetPlayerMeta` |
| Queue | `FCombatQueueNode` | `SetActionQueue`/`ResolveFrontQueueNode` | `GetActionQueue` |

## 박용수(UI) 사용법
1. WBP를 `UCombatUIWidgetBase` 상속으로 만든다.
2. `BindUIModel(VM)` 호출.
3. `OnUIRefreshed(Domain)`에서 필요한 표시값을 읽어 자기 화면을 그린다.
4. 탭/터치 → `VM->RequestSelectSkill(i)` / `RequestWorldTouch(pos,long)` …
5. `OnQueueNodePlayed(Node)`에서 머리 위 숫자·효과를 띄운다.
6. 게임플레이 전이라도 `UMockCombatDriver::Start(VM)`로 선택·예측 큐 재생까지 테스트.

## 모호재(게임플레이) 사용법
Mock 자리에 **어댑터**만 구현(위젯 무수정):
- 상태가 바뀌면 `VM->SetUnitUIs/SetSkillUIs/SetTurnUI()`.
- 행동/예측 결과를 `VM->SetActionQueue(Queue)`로 주고, 애니 한 단위 끝날 때마다 `VM->ResolveFrontQueueNode()`.
- `VM->OnCombatCommand`/`OnCombatWorldTouch`를 구독해 입력(의도)을 처리.

## 범위 — 이건 '전투' 뷰모델
전투 화면(HUD/Skill/Tile/Equip)만 담당. 전투 밖 UI는 별도 뷰모델이 같은 패턴을 반복하며, 이미 존재한다:
`URewardUIModel`(전투 보상/획득)·`UShopUIModel`(상점)·`UTreasureUIModel`(보물방). 타이틀·캐릭터선택·프론트엔드맵(방 이동)·설정·인벤토리는 뷰모델 없이 GameMode pull API(`Get*View`)로 그린다 — 필요해지면 같은 패턴으로 감싼다.

## 미합의/맞출 것
- **큐 노드 묶음 방식**: 한 행동의 효과(데미지·상태이상·힐)를 "한 노드+태그(`mTags`)"로 묶음(현재) vs 분리 — 회의 미확정.
- 게임플레이 타입(`FSRPGSkillBuildPhase`, 장비/메타 소스 `PersistentData`)과의 실제 매핑은 어댑터에서.
