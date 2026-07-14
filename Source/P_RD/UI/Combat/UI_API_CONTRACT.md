# 전투 UI ↔ 게임플레이 API 계약

> 작성: 박용수(UI/Dice) · 기준 코드: `UCombatUIModel` / `CombatUITypes.h`
> 회의(2026-06-15) 합의 = **데이터/비주얼 분리 + MVVM**. UI는 게임플레이 객체(`UUnitData`/`UDiceData`/`ATileMap`…)를 **직접 알지 않고**, 이 문서의 뷰 타입과 `UCombatUIModel` 계약만으로 동작한다.

전투 UI와 게임플레이는 **`UCombatUIModel` 한 곳**에서만 만난다. 권장 소유 위치는 `USRPGCombatSubsystem`(전투 수명)이며, 실제 HUD 배선은 후속 PR에서 붙인다.

원칙 3줄:
- **UI는 상태·RNG가 없다.** 진실은 게임플레이. UI는 VM의 캐시를 읽어 그리기만.
- **UI는 행동을 실행하지 않는다.** 탭/터치는 "의도"만 보내고(`Request*`), 게임플레이가 입력 델리게이트를 구독해 실제 처리.
- **게임플레이가 리팩토링돼도 이 계약만 지키면 UI는 무수정.** 게임플레이 미연결 시 `MockCombatDriver`가 같은 `Set*`로 가짜 데이터를 밀어 UI 선개발 가능.

---

## A. 게임플레이가 UI에 **줘야 하는 것** (gameplay → UI, `Set*`)

게임플레이/어댑터가 표시값을 `Set*()`로 push → VM이 `OnUIChanged(도메인)` 또는 상세 전용 알림을 발신 → 위젯이 해당 영역만 다시 그림. **UI는 이걸 못 만들어내므로 게임플레이가 반드시 공급해야 한다.**

| 도메인 | Set 함수 | 뷰 타입 | 게임플레이가 채워야 할 핵심 필드 |
|---|---|---|---|
| Unit | `SetUnitUIs(TArray<FUnitUI>)` | `FUnitUI` | `mUnitId, mIsPlayer, mHP/mMaxHP, mMovementPoint/mMaxMovementPoint, mTile`(ATileMap 점유 거울값 — 권위는 타일맵 파트.mOccupantUnitId), `mWorldLocation`(머리위 HP바 투영용), `mStatusTags`(버프/디버프, enum 아닌 태그) |
| Unit(상세) | `SetUnitDetail(FUnitDetailUI)` → `OnUnitDetailReady` | `FUnitDetailUI` | GameMode 월드 트레이스가 `mName, mLevel, mPortrait, mPassiveDescriptions`를 채우고 UI는 받은 DTO만 표시 |
| Dice | `SetDiceUIs(TArray<FDiceSlotUI>)` | `FDiceSlotUI` | `mDiceId, mResultValue`(0=미굴림), `mIsRolled, mIsSelected, mIsUsed`(이번 턴 잠금), `mRarityColor/mRarityText`(어댑터가 미리 색/문구로 변환), `mPreviewTexture`(3D 프리뷰용 슬롯, 캡처 계층은 후속) |
| Dice(선택) | `SetSelectedDice(TArray<int32>, int32 Sum)` | — | 스킬 빌드에 올린 주사위 인덱스들 + 합계 |
| Skill | `SetSkillUIs(TArray<FSkillUI>)` | `FSkillUI` | `mSkillIndex, mName, mIcon, mDiceCost, mIsUsable, mTargeting`(사거리/형태 조준 가이드 = StaticSkillData Select*/Hit* 미러) |
| Skill(상세) | `SetSkillDetail(FSkillDetailUI)` | `FSkillDetailUI` | 롱프레스 시 `mDescription, mTargeting`(풀스펙 사거리/타격범위/곡사·관통) 등 |
| Turn | `SetTurnUI(FTurnUI)` | `FTurnUI` | `mCurrentUnitId, mRound, mPhase`(=`ECombatBuildPhaseUI`, **UI 전용**: AimSelection/Preview만 develop `ESRPGSkillBuildPhase`와 매핑, SkillSelected/DiceSelect는 어댑터 파생), `mTurnOrderUnitIds` |
| Equipment | `SetEquipmentUIs(TArray<FEquipmentUI>)` | `FEquipmentUI` | `mSlotIndex, mItemId, mName, mIcon, mIsEquipped, mRarityColor` |
| Meta | `SetPlayerMeta(FPlayerMetaUI)` | `FPlayerMetaUI` | `mGold, mLevel, mExp/mMaxExp` (상단 상태바·보상) |

### 예측/연출 큐 (게임플레이 → UI)
- `SetActionQueue(TArray<FCombatQueueNode>)` — 행동/예측 결과를 큐로 통째 전달.
- `ResolveFrontQueueNode()` — 게임플레이가 애니 한 단위를 처리할 때마다 호출 → 맨 앞 노드 비우고 `OnQueueNodeResolved(Node)` 발신 → UI가 머리 위 숫자 등 1단위 재생.
- `FCombatQueueNode` = `mTags`(Combat.Damage/Status/Heal/Push…), `mSourceUnitId, mTargetUnitId, mAmount, mLabel`. **여러 효과를 타입 폭발 없이 태그로 묶어** 한 노드 = 애니 한 단위.

### 빌드 종료 통지
- `NotifyActionResolved()` — 스킬/액션이 확정·취소돼 빌드가 끝났을 때 호출 → `OnActionResolved` → UI가 스킬/주사위 **선택 강조 해제**.

---

## B. UI가 게임플레이에 **요구하는 것** (UI → gameplay, `Request*` = 의도만)

UI 버튼은 `Request*()`로 의도만 보낸다. 월드 탭/롱프레스는 UI 계약 밖에서 `CombatCameraPawn`이 구분하고 `CombatGameMode`에 직접 전달한다.

| UI 동작 | Request 함수 | 게임플레이가 받는 신호 | payload |
|---|---|---|---|
| 스킬 선택 | `RequestSelectSkill(SkillIndex)` | `OnCombatCommand(SelectSkill, idx)` | SkillIndex |
| 주사위 올림/내림 | `RequestToggleDice(DiceIndex)` | `OnCombatCommand(ToggleDice, idx)` | DiceIndex |
| 주사위 굴림 | `RequestRollDice()` | `OnCombatCommand(RollDice, INDEX_NONE)` | 없음 |
| 이동 모드 | `RequestMove()` | `OnCombatCommand(Move, INDEX_NONE)` | 없음 |
| 턴 종료 | `RequestEndTurn()` | `OnCombatCommand(EndTurn, INDEX_NONE)` | 없음 |
| 취소(딴 데 탭) | `RequestCancel()` | `OnCombatCommand(Cancel, INDEX_NONE)` | 없음 |
| 스킬 상세 | `RequestLongPressSkill(SkillIndex)` | `OnCombatCommand(LongPressSkill, idx)` | SkillIndex |
| 장비 상세 | `RequestLongPressEquip(SlotIndex)` | `OnCombatCommand(LongPressEquip, idx)` | SlotIndex |
| **월드 터치**(타일/유닛/취소) | UIModel 경유 없음 | `CombatCameraPawn` → `CombatGameMode::HandleCombatWorldTouch` | 스크린 좌표 + 롱프레스 여부 |

> 월드 포인터의 탭/롱프레스/드래그 구분은 `CombatCameraPawn`, 스크린→월드 대상 판정은 GameMode/CommandRouter가 담당한다. HUD와 UIModel은 월드 입력을 처리하지 않는다.

---

## C. UI가 **구독하는 알림** (게임플레이가 발신 → UI가 다시 그림)
- `OnUIChanged(ECombatUIDomain)` — 바뀐 도메인만 부분 갱신.
- `OnQueueNodeResolved(FCombatQueueNode)` — 큐 1노드 재생.
- `OnActionResolved()` — 선택 강조 해제.
- `OnUnitDetailReady(FUnitDetailUI)` — GameMode가 판정·생성한 유닛 상세 패널 표시.

---

## D. 게임플레이(모호재/김준형) 측 연결 지점 — 무엇을 어디에 물릴지
현재는 비GAS **임시 어댑터**(`UCombatUIAdapter`)가 A의 `Set*`를 채우고 B의 입력을 처리한다(플레이스홀더·가상 적 포함). 다이스 데이터는 `APlayerUnit`의 `UDiceComponent`/`UDiceData`가 소유하고, 어댑터가 이를 `FDiceSlotUI`로 변환한다. 실제 게임플레이 연결 시 어댑터 자리를 다음으로 대체:

- **유닛/메타/턴 값**(A) ← `UUnitData`(GAS 폐기 후 일반 런타임 데이터)·`URunPersistData`. 현재 HP/Gold는 플레이스홀더.
- **다이스**(A.Dice / B.Roll·Toggle) ← `UDiceData`(굴림/보유/사용/index 소유) = 회의의 **DiceComponent**. 굴림값은 `SRPGSkillBuildAction`(develop, 스킬에 주사위 적용)로 전달.
- **스킬 빌드 페이즈**(A.Turn.mPhase, `ECombatBuildPhaseUI`=UI 전용) ← AimSelection/Preview만 develop `ESRPGSkillBuildPhase`와 매핑, SkillSelected/DiceSelect는 어댑터가 `SRPGSkillBuildAction` 상태(mSelectedSkillIndex/mSelectedDices)에서 파생. `ACombatGameMode::SelectSkill`/`OnChangeSkillBuildPhase`와 연동.
- **데미지/큐**(A.Queue) ← `UCombatCalculatorFunctionLibrary::CalculateSkillResult`의 결과 델타(`FSkillCommitResult`)를 `FCombatQueueNode`로 변환해 `SetActionQueue` → 애니 단위마다 `ResolveFrontQueueNode`.
- **턴 이벤트** ← develop `USRPGCombatSubsystem` Begin/EndTurn 이벤트에 "쓴 주사위 리셋"·턴 표시 갱신 훅.

---

## E. 아직 안 정해진 것 (게임플레이와 합의 필요)
- 큐 노드 묶음 단위(한 노드+태그 vs 효과별 분리) — 회의 미확정.
- 적 정보 표시 = 작은 팝업 ❌ → **크게 뜨는 정보 패널**(회의 합의) 로 `FUnitDetailUI` 소비.
- 주사위 최대 개수 제한 여부(보유 다이스 레이아웃에 영향).
- `mPhase`의 AimSelection/Preview ↔ develop `ESRPGSkillBuildPhase` 매핑값 확정(SkillSelected/DiceSelect는 UI 파생이라 매핑 대상 아님).
- 스킬 `mTargeting`(SelectShape/HitShape) ↔ develop 최종 SelectType/HitType enum 매핑 확정.
- **예측 1급화(후속 PR)**: `FCombatQueueNode`의 예측 vs 실제 구분 플래그, AoE 다중대상 동시표시, 주사위 토글 시 예측 재계산 루프 — 김준형 예측 API(Mock-up 후) 일정에 맞춰 별도 처리.
