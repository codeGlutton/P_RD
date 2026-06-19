/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "CombatGameMode.generated.h"

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

struct FSRPGSkillBuildAction;
class UCombatUIAdapter;

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

protected:
	void OnChangeSkillBuildPhase(const FSRPGSkillBuildAction& Action, ESRPGSkillBuildPhase Phase);

	/** @brief 전투 상태를 CombatUIModel로 push + HUD 입력 의도를 처리하는 임시 비GAS 어댑터(전투 수명 동안 보유). */
	UPROPERTY()
	TObjectPtr<UCombatUIAdapter> mCombatUIAdapter;
};
