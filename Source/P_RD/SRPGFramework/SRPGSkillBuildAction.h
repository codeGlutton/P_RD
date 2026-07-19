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
class UUnitModel;
class UBoardActorModel;

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

USTRUCT(BlueprintType)
struct FSRPGDiceSelectCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGDiceSelectCommand();

public:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceIndex"))
	int32 mDiceIndex = 0;
};

/** @brief 타일을 다시 누르지 않고 HUD의 실행 버튼으로 현재 프리뷰를 확정한다. */
USTRUCT(BlueprintType)
struct FSRPGSkillConfirmCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillConfirmCommand();
};

/** @brief HUD의 다시 선택 버튼으로 현재 스킬 빌드를 안전하게 취소한다. */
USTRUCT(BlueprintType)
struct FSRPGSkillCancelCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillCancelCommand();
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

	/** @brief HUD가 게임 판정을 복제하지 않고 현재 위치 개입 프리뷰를 그리기 위한 읽기 전용 값. */
	bool IsPullDisplacementPreview() const;
	bool IsThrowDisplacementPreview() const;
	UUnitModel* GetDisplacementTarget() const;
	const TArray<FTileIndex>& GetDisplacementTrajectory() const { return mEffectTileIndexes; }
	const TArray<FTileIndex>& GetDisplacementDestinationCandidates() const { return mThrowDestinationIndexes; }
	const FTileIndex& GetDisplacementDestination() const { return mDisplacementDestination; }
	UBoardActorModel* GetDisplacementCollisionBlocker() const;

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
	void SetSkill(int32 SkillIndex);
	void ChangeDices(int32 RequestedDiceIndex);
	void SetTargetTile(const FTileIndex& TargetIndex);
	void LockDisplacementTarget(const FTileIndex& TargetIndex);
	void SetThrowDestinationTile(const FTileIndex& DestinationIndex);
	void BuildSkill();

private:
	void ResetSkill();
	void ResetDice();
	void ResetTargetTile();

private:
	void ClearAllTileHighlights();
	void RefreshAimableTileHighlights();
	void RefreshThrowDestinationHighlights();
	void RefreshEffectTileHighlights();

private:
	bool CanSelectTargetTile(const FTileIndex& Index) const;
	bool CanSelectThrowDestinationTile(const FTileIndex& Index) const;

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
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedSkill"))
	TObjectPtr<UStaticSkillData> mSelectedSkill;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectedSkillIndex"))
	int32 mSelectedSkillIndex = INDEX_NONE;

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTileIndexes"))
	TArray<FTileIndex> mEffectTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetIndex"))
	FTileIndex mTargetIndex = FTileIndex::Invalid;
	/** @brief 던지기에서 사용자가 고른 방향의 최종 착지/충돌 칸. 당기기에서는 Invalid다. */
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DisplacementDestination"))
	FTileIndex mDisplacementDestination = FTileIndex::Invalid;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ThrowDestinationIndexes"))
	TArray<FTileIndex> mThrowDestinationIndexes;
};

