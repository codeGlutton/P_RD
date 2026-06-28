/*****************************************************************//**
 * @file   TacticalPassive_AddAttackPoint.h
 * @brief  공격력 증가 패시브 (무상태)
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassive.h"
#include "TacticalPassive_AddAttackPoint.generated.h"

/**
 * @brief 공격력 증가 패시브
 *
 * @details
 * 대상의 공격력(AttackPoint)에 mAttackBonus만큼 더하는 무상태 패시브.
 * 패시브 내부 상태를 쓰지 않으므로 CommitPassive는 구현하지 않음(베이스 no-op).
 */
UCLASS()
class P_RD_API UTacticalPassive_AddAttackPoint : public UTacticalPassive
{
	GENERATED_BODY()

public:
	UTacticalPassive_AddAttackPoint();

protected:
	virtual void EvaluatePassive(
		IN const FPassiveActivateContext& Ctx,
		OUT FBoardCombatTargetSnapshotData& TargetDelta,
		IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) override;

public:
	// 더할 공격력 값
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "AttackBonus"))
	float mAttackBonus = 0.f;
};
