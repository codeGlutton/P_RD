/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "UI/Combat/CombatUITypes.h"
#include "CombatGameMode.generated.h"

struct FEquippedEntry;
struct FPresentationBarrier;
class UCombatUIModel;
class UUnitModel;

class USRPGSkillBuildAction;

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

DECLARE_MULTICAST_DELEGATE(FOnRefreshAllUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshUnitUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshDiceUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshSelectedDiceUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshSkillUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshEquipmentUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshTurnUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshPlayerMetaUI);
DECLARE_MULTICAST_DELEGATE(FOnCombatActionResolvedUI);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatPresentationUI, TSharedPtr<FPresentationBarrier> /*Barrier*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatEndedUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, bool /*bPlayerWin*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnPresentationUI, TSharedPtr<FPresentationBarrier> /*Barrier*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActionPresentationUI, TSharedPtr<FPresentationBarrier> /*Barrier*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRefreshCombatBuildPhase, ECombatBuildPhaseUI /*Phase*/);
DECLARE_MULTICAST_DELEGATE(FOnCombatShowDicePanelUI);
DECLARE_MULTICAST_DELEGATE(FOnCombatShowTargetDetailPanelUI);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowSkillDetailPanelUI, int32 /*SkillIndex*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowEquipmentDetailPanelUI, int32 /*SlotIndex*/);

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

	/* UI 진입점 */
public:
	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectSkill(int32 SkillIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectDice(int32 DiceIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool RollDices();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectMove();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool EndTurn();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool ShowSkillDetail(int32 SkillIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool ShowEquipmentDetail(int32 SlotIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	void PushAllCombatUI();

	bool GetEquipmentUIs(TArray<FEquipmentUI>& OutEquipmentUIs) const;

public:
	/**
	 * @brief 터치 입력 아래의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @return 이벤트 성공 여부
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool ResolveWorldTouchEvent();

	/**
	 * @brief 긴 터치 입력 아래의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @return 이벤트 성공 여부
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool ResolveWorldLongPressEvent();

public:
	/**
	 * @brief 장비 디테일 정보를 가져온다.
	 * @param EquipmentType 원하는 장비 대상
	 * @return 장비 런타임 정보
	 */
	const FEquippedEntry* GetEquipmentDetail(EEquipmentType EquipmentType);

	/* 연출용 대리자 (전투 수명/턴 연출 배리어 — TopMenuBar 등 UI가 구독) */
public:
	FOnCombatPresentationUI OnBeginCombatUI;
	FOnCombatEndedUI OnEndCombatUI;
	FOnTurnPresentationUI OnBeginAnyTurnUI;
	FOnTurnPresentationUI OnEndAnyTurnUI;
	FOnActionPresentationUI OnBeginAnyTurnActionUI;
	FOnActionPresentationUI OnEndAnyTurnActionUI;

public:
	FOnRefreshAllUI OnRefreshAllUI;
	FOnRefreshUnitUI OnRefreshUnitUI;
	FOnRefreshDiceUI OnRefreshDiceUI;
	FOnRefreshSelectedDiceUI OnRefreshSelectedDiceUI;
	FOnRefreshSkillUI OnRefreshSkillUI;
	FOnRefreshEquipmentUI OnRefreshEquipmentUI;
	FOnRefreshTurnUI OnRefreshTurnUI;
	FOnRefreshPlayerMetaUI OnRefreshPlayerMetaUI;
	FOnCombatActionResolvedUI OnCombatActionResolvedUI;
	FOnRefreshCombatBuildPhase OnRefreshSkillBuildPhase;
	FOnRefreshCombatBuildPhase OnRefreshMoveBuildPhase;
	FOnCombatShowDicePanelUI OnShowDicePanelAnyTurnUI;
	FOnCombatShowTargetDetailPanelUI OnShowTargetDetailPanelUI;
	FOnShowSkillDetailPanelUI OnShowSkillDetailPanelUI;
	FOnShowEquipmentDetailPanelUI OnShowEquipmentDetailPanelUI;

protected:
	void OnRegisterUnit(UUnitModel* Unit);
	void OnUnregisterUnit(UUnitModel* Unit);
	void BindPlayerDicePoolUIEvents();
	void BindPlayerMetaUIEvents();

	void PushUnitUI();
	void PushDiceUI();
	void PushSelectedDiceUI();
	void PushTurnUI();
	void PushSkillUI();
	void PushEquipmentUI();
	void PushPlayerMetaUI();

	UCombatUIModel* GetCombatUIModel() const;
};
