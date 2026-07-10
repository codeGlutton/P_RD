/*****************************************************************//**
 * @file   TacticalPassive_AddStat.h
 * @brief  데이터 구동 고정 스탯 가산 패시브 (무상태)
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassive.h"
#include "TacticalPassive_AddStat.generated.h"

/**
 * @brief 스탯 가산하는 패시브 (내부 상태 없음)
 *
 * @details
 * 이펙트가 정의한 속성에 mStaticData->mMagnitude를 더하는 고정형 패시브.
 * 속성/연산/지속은 EffectClass가, 양은 Magnitude가 결정하므로 스탯별로 만들지 않고 이 클래스 하나만 둠
 * 무상태이므로 CommitPassive는 구현하지 않음
 */
UCLASS()
class P_RD_API UTacticalPassive_AddStat : public UTacticalPassive
{
	GENERATED_BODY()

protected:
	virtual bool EvaluateActivate(
		IN const FPassiveActivateContext& Ctx,
		IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState,
		OUT float& OutMagnitude) override;
};
