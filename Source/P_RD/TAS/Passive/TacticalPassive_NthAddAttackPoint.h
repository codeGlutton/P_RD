/*****************************************************************//**
 * @file   TacticalPassive_NthAddAttackPoint.h
 * @brief  N번째 발동마다 공격력 보너스 패시브 (상태 보유)
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassive.h"
#include "TacticalPassive_NthAddAttackPoint.generated.h"

/**
 * @brief N번째 발동마다 공격력 보너스 패시브
 *
 * @details
 * EvaluatePassive: 완료 횟수(mCount)는 읽기만. 이번 차수(=완료+1)가 mThreshold면 공격력(AttackPoint)에
 *                  mAttackBonus 추가하고, 러닝본(NextState)에 발동=리셋(0)/미발동=차수 전진을 기록.
 * CommitPassive: 러닝본을 mState에 그대로 확정(리셋은 Evaluate가 이미 반영).
 *
 * 카운터는 패시브 내부 상태(FTacticalPassiveState_NthCounter, 0-base 완료 횟수), 임계값(N)은 config라 클래스에 둠.
 */
UCLASS()
class P_RD_API UTacticalPassive_NthAddAttackPoint : public UTacticalPassive
{
	GENERATED_BODY()

public:
	UTacticalPassive_NthAddAttackPoint();

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
