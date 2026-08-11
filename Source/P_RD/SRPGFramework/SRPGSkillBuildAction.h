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
#include "Simulation/Logger/EventLog.h"
#include "SRPGSkillBuildAction.generated.h"

class USRPGSkillBuildAction;
class UStaticSkillData;
class UTileMapModel;
class UDiceModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectSkill, int32 /*SkillIndex*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangeSkillBuildPhase, const USRPGSkillBuildAction* /*Action*/, ESRPGSkillBuildPhase /*Phase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPostSimulateSkillAction, const TArray<FSRPGTurnEventLog>& /*EventLogs*/);
DECLARE_MULTICAST_DELEGATE(FOnCancelSimulateSkillAction);

USTRUCT(BlueprintType)
struct FSRPGSkillSelectCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillSelectCommand();

public:
	FOnSelectSkill OnSelectSkill;
	FOnChangeSkillBuildPhase OnChangeSkillBuildPhase;
	FOnPostSimulateSkillAction OnPostSimulateSkillAction;
	FOnCancelSimulateSkillAction OnCancelSimulateSkillAction;

public:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillIndex"))
	int32 mSkillIndex = 0;
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
	// @brief 겨냥해 둔 타일 (겨냥 전이면 Invalid). 확정 단추가 "그 칸 재탭"을
	//        흉내 낼 때 쓴다.
	const FTileIndex& GetTargetIndex() const { return mTargetIndex; }

	/* FSRPGAction 상속 */
protected:
	void OnBeginAction() override;
	void OnEndAction() override;

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

protected:
	ESRPGCommandResult HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command);

	/**
	 * @brief 짓던 스킬을 무른다. 판에 칠해 둔 사거리까지 지운다.
	 *
	 * @details
	 * 무르는 자리가 셋이라 한 곳에 모았다 -- 같은 스킬 다시 고르기, 판 밖 탭,
	 * 사거리 밖 탭. 흩어 두었을 때 두 곳에서 하이라이트 지우기가 빠져서,
	 * 취소한 뒤에도 사거리가 판에 남아 있었다.
	 */
	void CancelBuild();

	/* 빌드 로직 처리 */
private:
	void SetSkill(int32 SkillIndex);
	void SetTargetTile(const FTileIndex& TargetIndex);
	void BuildSkill();

private:
	void ResetSkill();
	void ResetTargetTile();

private:
	void ClearAllTileHighlights();
	void RefreshAimableTileHighlights();
	void RefreshEffectTileHighlights();

private:
	bool CanSelectTargetTile(const FTileIndex& Index) const;
	bool CanConfirmTargetTile(const FTileIndex& Index) const;
	bool CanBuildSkill() const;

private:
	void SetBuildPhase(ESRPGSkillBuildPhase BuildPhase);

	/* 헬퍼 */
private:
	// @brief 턴 컨텍스트 → 전투 모델 → 타일 맵 모델을 꺼내온다
	UTileMapModel* GetTileMap() const;

protected:
	FOnSelectSkill OnSelectSkill;
	FOnChangeSkillBuildPhase OnChangeSkillBuildPhase;
	FOnPostSimulateSkillAction OnPostSimulateSkillAction;
	FOnCancelSimulateSkillAction OnCancelSimulateSkillAction;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceIndex"))
	ESRPGSkillBuildPhase mSkillBuildPhase = ESRPGSkillBuildPhase::None;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ReachableTileIndexes"))
	TArray<FTileIndex> mReachableTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedSkillIndex"))
	int32 mSelectedSkillIndex = INDEX_NONE;

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTileIndexes"))
	TArray<FTileIndex> mEffectTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetIndex"))
	FTileIndex mTargetIndex = FTileIndex::Invalid;
};

