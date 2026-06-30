/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGCommand.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "CombatGameMode.generated.h"

class UCombatUIAdapter;

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
	// FSkillEntry* GetSkillDetail(int32 SkillIndex);

	/**
	 * @brief 장비 디테일 정보를 가져온다.
	 * @param EquipmentIndex 원하는 장비 대상
	 * @return 장비 런타임 정보
	 */
	FEquippedEntry* GetEquipmentDetail(int32 EquipmentIndex);

	/* UI 갱신 대리자 */
public:
	FOnRefreshAllUI OnRefreshAllUI;
	FOnRefreshUnitUI OnRefreshUnitUI;
	FOnRefreshDiceUI OnRefreshDiceUI;
	FOnRefreshSelectedDiceUI OnRefreshSelectedDiceUI;
	// FOnRefreshSelectedSkillUI OnRefreshSelectedSkillUI;

	/* 연출용 대리자 */
public:
	FOnBeginCombatUI OnBeginCombatUI;
	FOnEndCombatUI OnEndCombatUI;
	FOnBeginAnyTurnUI OnBeginAnyTurnUI;
	FOnEndAnyTurnUI OnEndAnyTurnUI;
	FOnBeginAnyTurnActionUI OnBeginAnyTurnActionUI;
	FOnEndAnyTurnActionUI OnEndAnyTurnActionUI;

	/* 디테일 패널 대리자 */
public:
	FOnShowTargetDetailPanelUI OnShowTargetDetailPanelUI;

	/* 빌드 과정 대리자 */
public:
	FOnRefreshSkillBuildPhase OnRefreshSkillBuildPhase;
	FOnRefreshMoveBuildPhase OnRefreshMoveBuildPhase;

public:
	/** @brief 전투 상태를 CombatUIModel로 push + HUD 입력 의도를 처리하는 임시 비GAS 어댑터(전투 수명 동안 보유). */
    // TODO : 추후 삭제
	UPROPERTY()
	TObjectPtr<UCombatUIAdapter> mCombatUIAdapter;
};
