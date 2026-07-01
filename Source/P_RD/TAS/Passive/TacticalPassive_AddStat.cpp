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

bool UTacticalPassive_AddStat::EvaluateActivate(
	const FPassiveActivateContext& Ctx,
	TInstancedStruct<FDynamicPassiveData>& PassiveState,
	FBoardCombatTargetSnapshotData& TargetDelta)
{
	// 데이터/이펙트가 없으면 적용 안 함 (Ctx/PassiveState 미사용 - 무상태)
	if (mStaticData == nullptr || mEffectClass == nullptr)
	{
		return false;
	}

	// 이펙트가 정의한 속성에 데이터 수치(mMagnitude)를 그대로 누적
	const UTacticalEffect* EffectCDO = mEffectClass.GetDefaultObject();
	if (EffectCDO == nullptr || EffectCDO->mModifiers.Num() == 0)
	{
		return false;
	}
	TargetDelta.mAttributes.FindOrAdd(EffectCDO->mModifiers[0].mAttribute) += mStaticData->mMagnitude;

	// 적용한다고 회신
	return true;
}
