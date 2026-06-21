/*****************************************************************//**
 * @file   SRPGSkillBuildAction.h
 * @brief  스킬 생성 액션 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "FunctionLibrary/CombatCalculator/CombatResult.h"
#include "SRPGSkillBuildAction.generated.h"

class USRPGSkillBuildAction;
class UStaticSkillData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangeSkillBuildPhase, const USRPGSkillBuildAction* /*Action*/, ESRPGSkillBuildPhase /*Phase*/);


USTRUCT(BlueprintType)
struct FSRPGSkillSelectCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillSelectCommand();

public:
	FOnChangeSkillBuildPhase OnChangeSkillBuildPhase;

public:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillIndex"))
	int32 mSkillIndex = 0;
};

USTRUCT(BlueprintType)
struct FSRPGCDiceSelectCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGCDiceSelectCommand();

public:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceIndex"))
	int32 mDiceIndex = 0;
};

/**
 * @brief  스킬 생성 액션 객체
 */
UCLASS()
class USRPGSkillBuildAction : public USRPGAction
{
	GENERATED_BODY()

public:
	USRPGSkillBuildAction();

	/* FSRPGAction 상속 */
protected:
	void OnBeginAction() override;
	void OnEndAction() override;

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

protected:
	ESRPGCommandResult HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command);

	/* 빌드 로직 처리 */
private:
	void ChangeDices(int32 RequestedDiceIndex);

	void SetSkill(int32 SkillIndex);
	void ResetSkill();
	void SetTargetTile(const FTileIndex& TileIndex);
	void ResetTargetTile();
	void BuildSkill();

private:
	void SetBuildPhase(ESRPGSkillBuildPhase BuildPhase);

protected:
	FOnChangeSkillBuildPhase OnChangeSkillBuildPhase;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceIndex"))
	ESRPGSkillBuildPhase mSkillBuildPhase = ESRPGSkillBuildPhase::None;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AimTileIndexes"))
	TArray<FTileIndex> mAimTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedSkill"))
	TObjectPtr<UStaticSkillData> mSelectedSkill;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedSkillIndex"))
	int32 mSelectedSkillIndex = INDEX_NONE;

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedDices"))
	TArray<int32> mSelectedDices;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedDiceSum"))
	int32 mSelectedDiceSum = 0;

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTileIndexes"))
	TArray<FTileIndex> mEffectTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetIndex"))
	FTileIndex mTargetIndex = FTileIndex::Invalid;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CalculationResult"))
	FSkillCommitResult mCalculationResult;
};

