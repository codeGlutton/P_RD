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

class UDiceAdapter;
struct FSRPGSkillBuildAction;

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

private:
	/**
	 * @brief 보유 주사위를 굴려 전투 뷰모델의 주사위 도메인을 구동하는 임시 어댑터.
	 *
	 * @details
	 * UUnitData 데이터 계층이 생기기 전까지의 게임플레이 대역. 전투방 수명 동안 GameMode가 소유한다.
	 */
	UPROPERTY()
	TObjectPtr<UDiceAdapter> mDiceAdapter;
};
