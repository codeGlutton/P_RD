/*****************************************************************//**
 * @file   TacticalPassive_AddStat.cpp
 * @brief  데이터 구동 고정 스탯 가산 패시브 구현
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_AddStat.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"

void UTacticalPassive_AddStat::EvaluatePassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta,
	TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 데이터/이펙트가 없으면 기여 없음 (Ctx/PassiveState 미사용 - 무상태)
	if (mStaticData == nullptr || mEffectClass == nullptr)
	{
		return;
	}

	// 이펙트가 정의한 속성에 데이터 수치(mMagnitude)를 그대로 누적
	const UTacticalEffect* EffectCDO = mEffectClass.GetDefaultObject();
	if (EffectCDO == nullptr || EffectCDO->mModifiers.Num() == 0)
	{
		return;
	}
	TargetDelta.mAttributes.FindOrAdd(EffectCDO->mModifiers[0].mAttribute) += mStaticData->mMagnitude;
}
