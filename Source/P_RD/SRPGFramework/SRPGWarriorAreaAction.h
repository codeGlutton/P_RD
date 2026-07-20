#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGWarriorAreaAction.generated.h"

class UEnemyUnitModel;
class UBoardActorModel;

/** @brief 별도 타겟 확정창 없이 기사 주변에 즉시 발동하는 광역 행동 명령. */
USTRUCT()
struct FSRPGWarriorAreaCommand : public FSRPGCommand
{
	GENERATED_BODY()

	FSRPGWarriorAreaCommand();

	UPROPERTY()
	ESRPGWarriorAreaActionType mAreaActionType = ESRPGWarriorAreaActionType::Whirlwind;
	UPROPERTY()
	int32 mRadius = 1;
	UPROPERTY()
	int32 mDamage = 0;
};

/** @brief 회전베기/충격파를 한 행동으로 해결하고 모든 밀침 연출 뒤 턴을 넘긴다. */
UCLASS()
class USRPGWarriorAreaAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGWarriorAreaAction();
	void OnBeginAction() override;
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

private:
	bool TryStartAreaApproach();
	void OnAreaApproachFinished();
	void BeginAreaImpact();
	void ApplyHit(UEnemyUnitModel* Target);
	void TryPushTarget(UEnemyUnitModel* Target);
	void OnPushPresentationFinished(UEnemyUnitModel* Target);
	void FinishIfReady();
	UBoardActorModel* FindBlockingActor(const FTileIndex& TileIndex, const UBoardActorModel* MovingActor) const;

	ESRPGWarriorAreaActionType mAreaActionType = ESRPGWarriorAreaActionType::Whirlwind;
	int32 mRadius = 1;
	int32 mDamage = 0;
	int32 mPendingPushPresentations = 0;
	bool mResolvedAnyTarget = false;
	bool mSchedulingPushes = false;
	FTileIndex mApproachFrom = FTileIndex::Invalid;
	FTileIndex mApproachTo = FTileIndex::Invalid;
};
