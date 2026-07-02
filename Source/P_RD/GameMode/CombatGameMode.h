/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGCommand.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "CombatGameMode.generated.h"

class USRPGTurnContext;
class UUnitModel;
class UCombatUIAdapter;
class UCombatUIModel;

struct FTacticalAttributeChangeData;

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

DECLARE_MULTICAST_DELEGATE(FOnRefreshAllUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshUnitUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshDiceUI);

DECLARE_MULTICAST_DELEGATE(FOnRefreshSelectedDiceUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshSelectedSkillUI);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRefreshSkillBuildPhase, ESRPGSkillBuildPhase /*Phase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRefreshMoveBuildPhase, ESRPGMoveBuildPhase /*Phase*/);

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	ACombatGameMode();

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

public:
	UCombatUIModel* GetCombatUIModel() const;

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
	 * @brief 스킬 디테일 정보를 가져온다.
	 * @param SkillIndex 원하는 스킬 대상
	 * @return 스킬 런타임 정보
	 */
	// const FSkillEntry* GetSkillDetail(int32 SkillIndex);

	/**
	 * @brief 장비 디테일 정보를 가져온다.
	 * @param EquipmentType 원하는 장비 타입
	 * @return 장비 런타임 정보
	 */
	const FEquippedEntry* GetEquipmentDetail(EEquipmentType EquipmentType);

protected:
	void OnRegisterUnit(UUnitModel* Unit);
	void OnUnregisterUnit(UUnitModel* Unit);

protected:
	void PushTurnUIData() const;
	void PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const;
	void PushMoveBuildUIData(ESRPGMoveBuildPhase Phase) const;
	void PushUnitUIData() const;
	void PushDiceUIData() const;
	void PushSelectedDiceUIData() const;
	void PushSkillUIData() const;
	void PushEquipmentUIData() const;
	void PushPlayerMetaUIData() const;

	/* 연출용 대리자 */
public:
	FOnBeginCombatUI OnBeginCombatUI;
	FOnEndCombatUI OnEndCombatUI;
	FOnBeginAnyTurnUI OnBeginAnyTurnUI;
	FOnEndAnyTurnUI OnEndAnyTurnUI;
	FOnBeginAnyTurnActionUI OnBeginAnyTurnActionUI;
	FOnEndAnyTurnActionUI OnEndAnyTurnActionUI;

public:
	UPROPERTY(Category = "UI", VisibleAnywhere, DuplicateTransient, meta = (DisplayName = "CombatUIModel"))
	TObjectPtr<UCombatUIModel> mCombatUIModel;

public:
	/** @brief 전투 상태를 CombatUIModel로 push + HUD 입력 의도를 처리하는 임시 비GAS 어댑터(전투 수명 동안 보유). */
    // TODO : 추후 삭제
	UPROPERTY()
	TObjectPtr<UCombatUIAdapter> mCombatUIAdapter;
};
