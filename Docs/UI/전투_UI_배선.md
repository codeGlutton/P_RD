# 전투 UI 배선 — 값이 어디서 와서 어디로 가나

> 작성: 박용수(UI) · 근거: `CombatGameMode.cpp` · `CombatUIModel` · `CombatLayoutHUDWidget.cpp` 를 직접 읽은 것
> **새 값을 화면에 띄우고 싶을 때 어디를 건드리면 되는지**를 적었다.

---

## 0. 한 줄

**전투 UI와 게임플레이는 `UCombatUIModel` 한 곳에서만 만난다.** 게임모드가
`Set*` 로 밀고, 위젯이 `Get*` 로 읽고, 탭은 `Request*` 로 의도만 돌려보낸다.

---

## 1. 세 갈래

```
게임플레이 사건  ──▶  ACombatGameMode::Push*UIData()
                          │  값을 뷰 타입(FUnitUI…)으로 옮겨 담고
                          ▼
                     UCombatUIModel::Set*()
                          │  캐시에 넣고 OnUIChanged(도메인) 방송
                          ▼
              UCombatLayoutHUDWidget::Refresh*()
                          │  Get*() 로 읽어 그린다
                          ▼
                       화면

     탭  ──▶  Request*()  ──▶  OnCombatCommand  ──▶  HandleCombatCommand
                                                          (게임플레이가 처리)
```

**UI 는 상태도 계산도 안 가진다.** 사거리 판정도 카드 켜고 끄기도 게임플레이가
정해서 `mIsUsable` 로 내려준다 — 두 곳에서 세면 어긋나는 날이 온다.

---

## 2. 언제 다시 밀리나 — 계기 표

`ACombatGameMode::BeginPlay` 에서 전투 모델의 사건에 람다를 걸어 둔 것이다.

| 게임플레이 사건 | 무엇을 다시 미나 |
|---|---|
| `OnBeginAnyTurnUI` (차례 시작) | Turn · Unit · Skill · Equipment **넷 다** |
| `OnBeginAnyRoundUI` (라운드 시작) | Turn (배너 숫자) |
| `OnBeginCombatUI` / `OnEndCombatUI` | Meta(돈·경험치), 끝날 때 결과까지 |
| `OnShowCombatResultUI` (이김) | 보상 · 보상 선택지 |
| `OnChangeSkillUI` (스킬 바뀜) | Skill |
| `OnEndAnyTurnActionUI` | `NotifyActionResolved` — 선택 강조 풀기 |
| **속성 바뀜** (HP·MaxHP·Movement·Defense) | Unit |
| **상태이상 태그 붙거나 떨어짐** | Unit |
| 유닛 등록·해제 | Turn · Unit |
| 칸을 톡 침 (`PushCombatTargetUIData`) | Target + **Skill** (겨냥이 바뀌면 쓸 수 있는 게 달라짐) |

> **속성은 델리게이트로 자동이다.** `OnRegisterUnit` 이 HP/Movement/Defense
> 변경에 `PushUnitUIData` 를 걸어 둔다. 새 속성을 화면에 띄우려면 **그 목록에
> 한 줄 더 걸어야** 값이 바뀔 때 다시 그려진다.

---

## 3. 무엇을 미나 — Push 함수 열일곱

| Push | 채우는 것 | 원본 |
|---|---|---|
| `PushUnitUIData` | 유닛 전부 | `USRPGCombatModel::GetUnits()` + 속성 컴포넌트 |
| `PushSkillUIData` | 카드 여섯 | `USkillComponentModel::GetSkills()` + `UStaticSkillData` |
| `PushTurnUIData` | 라운드·차례·턴 순서 | 전투 모델 |
| `PushSkillBuildUIData` / `PushMoveBuildUIData` | 조작 단계 | `ESRPGSkillBuildPhase` 거울 |
| `PushCombatTargetUIData` | 겨냥한 칸 | 월드 트레이스 |
| `PushCombatTargetDetailUIData` | 꾹 누른 대상 상세 | `IBoardSelectionTarget` |
| `PushSkillDetailUIData` | 스킬 상세 | `UStaticSkillData` |
| `PushEquipmentUIData` / `Detail` | 장비 | 장비 컴포넌트 |
| `PushPlayerMetaUIData` | 돈·경험치 | 파티 |
| `PushSimulationFloatingLogs` | 머리 위 숫자 | `FSRPGTurnEventLog` (**예측도 이 길**) |
| `PushCombatResultUIData` / `Reward` / `RewardChoices` | 결과·보상 | |

---

## 4. 계약에 칸은 있는데 안 채우는 것 — 전수

`CombatUITypes.h` 의 모든 뷰 타입 칸을 `CombatGameMode.cpp` 와 맞춰 본 것이다.
**빠진 것은 두 구조체뿐이다.**

### 4.1 `FUnitUI`

| 칸 | 판정 |
|---|---|
| `mActionPoints` · `mMaxActionPoints` | **죽은 칸.** 이 프로젝트에 AP 자원이 따로 없다 — `Movement` 가 곧 행동력이다. UI 는 `mMovementPoint` 를 본다 |
| `mDamagePoint` | `AttackPoint` 속성이 있긴 하다. 다만 **회의에서 기본 공격력 스탯은 없기로** 했다 |
| `mSkillPoint` | 원본 없음 |

### 4.2 `FSkillUI` — 다 얹었다

| 칸 | 어떻게 |
|---|---|
| `mCooldownTurns` | ✔ `GetStaticCooldownDuration` |
| `mRemainingCooldown` | ✔ `IsCooldown` 일 때만 `GetRemainingCooldownTime` |
| `mDamageMin` · `mDamageMax` | ✔ 모션마다 나뉜 `FSkillEffectLayer_Attack::mDamage` 총합. **아직 min == max** |
| `mCriticalDamage` | ✔ `max × 1.5`. 계산에 크리 분기가 없어 UI 가 곱한다 |
| `mActionPointGain` | 원본 없음 |

---

## 5. 새 값을 띄우고 싶을 때 — 순서

1. **원본이 있나 찾는다.** 없으면 여기서 멈추고 게임플레이에 부탁한다
2. `CombatUITypes.h` 뷰 타입에 **칸이 있나** 본다. 없으면 만든다
3. `ACombatGameMode::Push*UIData()` 에서 **원본을 읽어 칸에 넣는다**
4. **값이 변할 때 다시 밀리나** 본다 — 2장 계기 표. 속성이면 `OnRegisterUnit` 에 델리게이트 한 줄
5. 위젯이 `Get*()` 로 읽어 그린다. **위젯 이름이 WBP 에 있어야** 한다 —
   없으면 C++ 이 말없이 건너뛴다
6. 굽는 쪽(`build_hud04.py`)에 그 이름의 글자칸이 있나 본다

**3번만 하고 4번을 빼먹으면** 첫 갱신에만 나오고 그 뒤로는 안 바뀐다.

---

## 6. 아직 원본이 없는 것 — 남에게

| 무엇 | 누구 | 없으면 |
|---|---|---|
| 피해 min/max 분리 | 모호재 | 카드가 `8~8` 로 나온다 |
| 크리티컬 확률 능력치 | 모호재 | 용병 상세창에 못 적는다 |
| 크리 분기 (최종 ×1.5) | 모호재 | UI 가 곱하고 있다. 실제와 갈릴 수 있다 |
| 상태이상 남은 턴 | 모호재 | 중첩 수만 그린다 |
| `FEnemyIntentUI` (다음 스킬 · 위협 칸) | 이문환 | 계약 자체가 없다 |
| 기믹 | 김준형 | 물건이 없다 |
