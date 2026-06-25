/*****************************************************************//**
 * @file   TacticalPassive_NthAddAttack.cpp
 * @brief  N번째 발동마다 공격력 보너스 패시브 구현
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_NthAddAttack.h"
#include "TAS/Passive/TacticalPassiveState_NthCounter.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"

void UTacticalPassive_NthAddAttack::EvaluatePassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta,
	TInstancedStruct<FTacticalPassiveState>& PassiveState)
{
	// 러닝 상태를 커밋된 상태로 시드 (커밋된 게 없으면 새로 생성)
	if (!PassiveState.IsValid())
	{
		if (mState.IsValid())
		{
			PassiveState = mState;
		}
		else
		{
			PassiveState.InitializeAs<FTacticalPassiveState_NthCounter>();
		}
	}

	// 러닝 카운터 증가 (리셋은 CommitPassive에서)
	FTacticalPassiveState_NthCounter& Counter = PassiveState.GetMutable<FTacticalPassiveState_NthCounter>();
	++Counter.mCount;

	// 임계값 도달 시 대상 공격력(DamagePoint)에 보너스 누적
	if (Counter.mCount >= mThreshold)
	{
		TargetDelta.mAttributes.FindOrAdd(UUnitAttributeSet::GetDamagePointAttribute()) += mAttackBonus;
	}
}

void UTacticalPassive_NthAddAttack::CommitPassive(
	const TInstancedStruct<FTacticalPassiveState>& PassiveState)
{
	// 러닝 상태를 커밋
	mState = PassiveState;

	// 임계값 도달 시 카운터 0으로 리셋
	if (mState.IsValid())
	{
		FTacticalPassiveState_NthCounter& Counter = mState.GetMutable<FTacticalPassiveState_NthCounter>();
		if (Counter.mCount >= mThreshold)
		{
			Counter.mCount = 0;
		}
	}
}
