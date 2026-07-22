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
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Simulation/Logger/EventLog.h"
#include "UI/Reward/RewardUITypes.h"
#include "CombatGameMode.generated.h"

class USRPGTurnContext;
class UUnitModel;
class IBoardSelectionTarget;

class UCombatUIModel;
class URewardUIModel;
enum class ECombatInputType : uint8;

// Combat Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	ACombatGameMode();

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

public:
	UCombatUIModel* GetCombatUIModel() const;
	URewardUIModel* GetRewardUIModel() const;

	/* UI 진입점 */
public:
	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectSkill(int32 SkillIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectMove();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool EndTurn();

	/**
	 * @brief UIModel에서 올라온 버튼 입력을 전투 명령으로 보낸다.
	 */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/**
	 * @brief 전장 탭을 조준/시전 입력으로 처리한다.
	 * @details
	 * 플레이어 턴이 아닐 때는 입력을 무시한다.
	 */
	UFUNCTION()
	void HandleCombatWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	UFUNCTION()
	void HandleRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	UFUNCTION()
	void HandleAbandonRun();

protected:
	/** @brief UI 없이 남아 있는 DicePrepare 액션을 즉시 진행시킨다. Dice 시스템 후속 정리 때 함께 제거한다. */
	bool RollDices();

	/**
	 * @brief 터치/클릭 지점의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @param ScreenPosition 입력 지점의 화면 좌표(픽셀). 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @return 이벤트 성공 여부
	 */
	bool ResolveWorldTouchEvent(FVector2D ScreenPosition);

	/**
	 * @brief 긴 터치/클릭 지점의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @param ScreenPosition 입력 지점의 화면 좌표(픽셀). 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @return 이벤트 성공 여부
	 */
	bool ResolveWorldLongPressEvent(FVector2D ScreenPosition);

protected:
	void OnRegisterUnit(UUnitModel* Unit);
	void OnUnregisterUnit(UUnitModel* Unit);

protected:
	void PushCombatResultUIData(ESRPGCombatResult Result) const;
	void PushTurnUIData() const;
	void PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const;
	void PushMoveBuildUIData(ESRPGMoveBuildPhase Phase) const;
	void PushUnitUIData() const;
	void PushSkillUIData() const;
	void PushSelectedSkillUIData(int32 SkillIndex) const;

	void PushCombatTargetDetailUIData(IBoardSelectionTarget* Target) const;

	void PushSkillDetailUIData(int32 SkillIndex) const;
	void PushEquipmentUIData() const;
	void PushEquipmentDetailUIData(int32 EquipmentIndex) const;
	void PushPlayerMetaUIData() const;

	void PushSimulationFloatingLogs(const TArray<FSRPGTurnEventLog>& TurnEventLogs, bool IsPreview = true) const;

	void PushCombatRewardUIData() const;
	void PushCombatRewardChoicesUIData() const;

public:
	UPROPERTY(Category = "UI", VisibleAnywhere, DuplicateTransient, meta = (DisplayName = "CombatUIModel"))
	TObjectPtr<UCombatUIModel> mCombatUIModel;
	UPROPERTY(Category = "UI", VisibleAnywhere, DuplicateTransient, meta = (DisplayName = "RewardUIModel"))
	TObjectPtr<URewardUIModel> mRewardUIModel;
};
