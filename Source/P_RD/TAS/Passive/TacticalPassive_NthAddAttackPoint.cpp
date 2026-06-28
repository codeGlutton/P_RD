/*****************************************************************//**
 * @file   TacticalPassive_NthAddAttackPoint.cpp
 * @brief  N번째 발동마다 공격력 보너스 패시브 구현
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_NthAddAttackPoint.h"
#include "TAS/Passive/DynamicPassiveData_NthCounter.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "GameplayTagType.h"

UTacticalPassive_NthAddAttackPoint::UTacticalPassive_NthAddAttackPoint()
{
	// 관여 시점: 공격 시작(가산 단계, 발동)·공격 끝(커밋·해제). BP 서브클래스에서 교체 가능
	mTimingTags.AddTag(AbilityTags::GameplayAbility_Passive_OnStartAttacking_Add);
	mTimingTags.AddTag(AbilityTags::GameplayAbility_Passive_OnEndAttacking);

	// 기본 적용 이펙트(공격력 가산). 패시브 BP 서브클래스에서 BP 이펙트로 교체 가능
	mEffectClass = UTacticalEffect_AttackPoint::StaticClass();
}

void UTacticalPassive_NthAddAttackPoint::EvaluatePassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta,
	TInstancedStruct<FDynamicPassiveData>& PassiveState)
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
			PassiveState.InitializeAs<FDynamicPassiveData_NthCounter>();
		}
	}

	// 러닝본(NextState 버퍼). mState가 아니라 드라이버가 든 작업 복사본
	FDynamicPassiveData_NthCounter& Running = PassiveState.GetMutable<FDynamicPassiveData_NthCounter>();

	// 완료 횟수(과거 기록)는 읽기만. 이번 차수는 +1 한 로컬 값으로 판단(상태는 안 바꿈)
	const int32 CompletedCount = Running.mCount;
	const int32 Ordinal = CompletedCount + 1;

	// 이번 차수가 임계면 대상 공격력(AttackPoint)에 보너스 누적
	const bool bTriggered = (Ordinal >= mThreshold);
	if (bTriggered)
	{
		TargetDelta.mAttributes.FindOrAdd(UUnitAttributeSet::GetAttackPointAttribute()) += mAttackBonus;
	}

	// 다음 상태(NextState) 기록: 발동이면 사이클 리셋(0), 아니면 완료 횟수 전진
	Running.mCount = bTriggered ? 0 : Ordinal;
}

void UTacticalPassive_NthAddAttackPoint::CommitPassive(
	const TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 러닝본을 그대로 커밋 (리셋은 Evaluate가 NextState에 이미 반영)
	mState = PassiveState;
}
