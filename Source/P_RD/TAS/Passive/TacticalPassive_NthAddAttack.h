/*****************************************************************//**
 * @file   TacticalPassive_NthAddAttack.h
 * @brief  N번째 발동마다 공격력 보너스 패시브 (상태 보유)
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassive.h"
#include "TacticalPassive_NthAddAttack.generated.h"

/**
 * @brief N번째 발동마다 공격력 보너스 패시브
 *
 * @details
 * EvaluatePassive: 러닝 카운터 +1, mThreshold 도달 시 대상 공격력(DamagePoint)에 mAttackBonus 추가 (리셋 안 함).
 * CommitPassive: 러닝 카운터를 mState에 커밋, 임계값 도달 시 0으로 리셋.
 *
 * 카운터는 패시브 내부 상태(FTacticalPassiveState_NthCounter), 임계값(N)은 config라 클래스에 둠.
 */
UCLASS()
class P_RD_API UTacticalPassive_NthAddAttack : public UTacticalPassive
{
	GENERATED_BODY()

protected:
	virtual void EvaluatePassive(
		IN const FPassiveActivateContext& Ctx,
		OUT FBoardCombatTargetSnapshotData& TargetDelta,
		IN OUT TInstancedStruct<FTacticalPassiveState>& PassiveState) override;

public:
	virtual void CommitPassive(
		IN const TInstancedStruct<FTacticalPassiveState>& PassiveState) override;

public:
	// 발동 임계값 (몇 번째마다 터지는지)
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Threshold"))
	int32 mThreshold = 3;

	// 터질 때 더할 공격력
	UPROPERTY(Category = "Passive", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "AttackBonus"))
	float mAttackBonus = 0.f;
};
