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
	float& OutMagnitude)
{
	// 데이터/이펙트가 없으면 적용 안 함 (Ctx/PassiveState 미사용 - 무상태)
	if (mStaticData == nullptr || mEffectClass == nullptr)
	{
		return false;
	}

	// DA의 매그니튜드 값을 그대로 사용
	OutMagnitude = mStaticData->mMagnitude;

	// 적용한다고 회신
	return true;
}
