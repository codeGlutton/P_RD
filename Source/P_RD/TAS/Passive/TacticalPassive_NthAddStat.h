/*****************************************************************//**
 * @file   TacticalPassive_NthAddStat.h
 * @brief  데이터 구동 N번째 발동마다 스탯 가산 패시브 (상태 보유)
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassive.h"
#include "TacticalPassive_NthAddStat.generated.h"

/**
 * @brief N번째 발동마다 스탯 가산하는 계산형 패시브
 *
 * @details
 *
 * 임계값(mStaticData->mThreshold) 다다를 때마다 이펙트 속성에 mMagnitude를 더하고 카운터를 리셋.
 * 카운터는 내부 상태(FDynamicPassiveData_NthCounter)에 보관.
 * EffectClass/Magnitude/Threshold 모두 데이터에서 읽으므로 동작별로 클래스 하나만 둠.
 */
UCLASS()
class P_RD_API UTacticalPassive_NthAddStat : public UTacticalPassive
{
	GENERATED_BODY()

protected:
	virtual bool EvaluateActivate(
		IN const FPassiveActivateContext& Ctx,
		IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState,
		OUT float& OutMagnitude) override;

public:
	virtual void CommitPassive(
		IN const TInstancedStruct<FDynamicPassiveData>& PassiveState) override;
};
