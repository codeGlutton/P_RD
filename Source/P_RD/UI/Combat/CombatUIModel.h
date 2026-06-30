#pragma once

/** @brief 전투 UI가 읽는 표시 스냅샷 모델입니다. */
// gameplay -> UI: 게임플레이/표시 계층이 Set*()으로 표시값을 밀어넣고, 위젯은 Get*()으로 읽는다.
// Queue: 행동 결과는 FCombatQueueNode 큐로 받고, ResolveFrontQueueNode() 한 번에 한 연출 단위씩 비운다.
// 계약 의도: 게임플레이가 리팩토링돼도 Set/Get 표시 경계가 유지되면 위젯 변경을 최소화한다.

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"

#include "CombatUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatQueueNodeResolved, FCombatQueueNode, Node);

/** @brief 전투 조작 UI의 뷰모델. PlayerController나 전투 HUD가 하나 소유해 위젯들이 공유한다. */
UCLASS(BlueprintType)
class P_RD_API UCombatUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 큐 노드 하나가 처리(재생)됐음을 알림. 위젯은 머리 위 숫자 등을 띄우고 한 칸 비운다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatQueueNodeResolved OnQueueNodeResolved;

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ─────────
	   각 Set*()은 UI가 그리려면 게임플레이가 반드시 공급해야 하는 값이다(UI는 못 만듦).
	   [소스]=가져올 곳(정해짐), [합의필요]=진짜 소스 미정. */
public:
	/** @brief 전체 유닛 HP/이동력/타일/HP바위치/상태태그. [합의필요] HP/MaxHP 진짜소스=UUnitData(GAS 폐기후). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitUIs(const TArray<FUnitUI>& Units);
	/** @brief 유닛 롱프레스 상세(이름/레벨/초상화/패시브). [합의필요] UUnitData 연결. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitDetail(const FUnitDetailUI& Detail);
	/** @brief 보유 주사위(굴림값/면수/희귀도색/사용잠금/3D프리뷰 슬롯). [소스] APlayerUnit UDicePoolModel(진짜, 비GAS). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetDiceUIs(const TArray<FDiceSlotUI>& Dice);
	/** @brief 스킬에 올린 주사위 index들+합계. [소스] SRPGSkillBuildAction.mSelectedDices. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum);
	/** @brief 스킬 레일(이름/아이콘/주사위코스트/사용가능). [합의필요] 소스=USkillComponent(김준형). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillUIs(const TArray<FSkillUI>& Skills);
	/** @brief 턴유닛/라운드/페이즈/턴순서. mPhase=ECombatBuildPhaseUI(UI 전용). [합의필요] AimSelection/Preview만 ESRPGSkillBuildPhase와 매핑(모호재), SkillSelected/DiceSelect는 UI 표시 계층 파생. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetTurnUI(const FTurnUI& Turn);
	/** @brief 장비 슬롯(아이콘/이름/장착/희귀도). [합의필요] 장비 데이터 소스 미정, 현재 임시. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetEquipmentUIs(const TArray<FEquipmentUI>& Equipment);
	/** @brief 상단 메타(Gold/Lv/Exp). [합의필요] 진짜소스=UUnitData/URunPersistData. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetPlayerMeta(const FPlayerMetaUI& Meta);

	/** @brief 행동/예측 결과 큐를 통째로 설정(예측 표시용). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetActionQueue(const TArray<FCombatQueueNode>& Queue);

	/** @brief 큐 맨 앞 노드 하나를 처리 완료로 비우고 OnQueueNodeResolved를 쏜다(애니 한 단위 끝날 때 호출). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void ResolveFrontQueueNode();

	/* ───────── 위젯이 읽는다 ───────── */
public:
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FUnitUI>& GetUnitUIs() const { return mUnitUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FUnitDetailUI& GetUnitDetail() const { return mUnitDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FDiceSlotUI>& GetDiceUIs() const { return mDiceUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<int32>& GetSelectedDiceIndices() const { return mSelectedDiceIndices; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") int32 GetSelectedDiceSum() const { return mSelectedDiceSum; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FSkillUI>& GetSkillUIs() const { return mSkillUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FCombatQueueNode>& GetActionQueue() const { return mActionQueue; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FTurnUI& GetTurnUI() const { return mTurnUI; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FEquipmentUI>& GetEquipmentUIs() const { return mEquipmentUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FPlayerMetaUI& GetPlayerMeta() const { return mPlayerMeta; }

private:
	/** @brief 마지막으로 push된 유닛 표시 스냅샷. 위젯은 참조로 읽고 수정하지 않는다. */
	UPROPERTY(Transient) TArray<FUnitUI> mUnitUIs;
	/** @brief 마지막 유닛 상세 스냅샷. 롱프레스 상세 패널의 단일 소스다. */
	UPROPERTY(Transient) FUnitDetailUI mUnitDetail;
	/** @brief 마지막 주사위 표시 스냅샷. 굴림값/사용잠금/면 정보가 모두 여기에 모인다. */
	UPROPERTY(Transient) TArray<FDiceSlotUI> mDiceUIs;
	/** @brief 현재 스킬 빌드에 올린 주사위 index 목록. index는 mDiceUIs 배열 기준이다. */
	UPROPERTY(Transient) TArray<int32> mSelectedDiceIndices;
	/** @brief 선택된 주사위 결과 합계. UI가 다시 합산하지 않게 게임플레이가 확정해 push한다. */
	UPROPERTY(Transient) int32 mSelectedDiceSum = 0;
	/** @brief 스킬 레일 표시 스냅샷. SkillIndex payload와 같은 index 계약을 가진다. */
	UPROPERTY(Transient) TArray<FSkillUI> mSkillUIs;
	/** @brief 아직 재생되지 않은 행동 결과 큐. ResolveFrontQueueNode()가 앞에서 하나씩 제거한다. */
	UPROPERTY(Transient) TArray<FCombatQueueNode> mActionQueue;
	/** @brief 현재 턴/라운드/페이즈 표시 스냅샷. */
	UPROPERTY(Transient) FTurnUI mTurnUI;
	/** @brief 장비 슬롯 표시 스냅샷. */
	UPROPERTY(Transient) TArray<FEquipmentUI> mEquipmentUIs;
	/** @brief 플레이어 메타 표시 스냅샷. */
	UPROPERTY(Transient) FPlayerMetaUI mPlayerMeta;
};
