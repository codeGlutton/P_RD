/*****************************************************************//**
 * @file   TacticalPassive_Generic.h
 * @brief  데이터 기반 제네릭 패시브
 * @author 이문환
 * @date   2026-09-01
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassive.h"
#include "TacticalPassive_Generic.generated.h"

/**
 * @brief 데이터 기반 제네릭 패시브
 *
 * @details
 * 조건(mConditions)/수치(mEffects)/대상(mEffectTarget)을 전부 UStaticPassiveData에서 읽음.
 * 런타임 상태는 FDynamicPassiveData_Generic 하나(카운터 + 캡처값).
 * 패시브 종류가 늘어도 클래스 추가 없이 DA만 만들면 됨.
 */
UCLASS()
class P_RD_API UTacticalPassive_Generic : public UTacticalPassive
{
	GENERATED_BODY()

public:
	virtual void CommitPassive(IN const TInstancedStruct<FDynamicPassiveData>& PassiveState) override;

protected:
	virtual void InitializeState(OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) const override;
	virtual void OnCounterReset(IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) override;
	virtual void OnCapture(IN const FPassiveActivateContext& Ctx, IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) override;
	virtual void OnActivate(IN const FPassiveActivateContext& Ctx, IN OUT TInstancedStruct<FDynamicPassiveData>& PassiveState) override;
	virtual bool IsTargetQualified(IN const FPassiveActivateContext& Ctx, IN int32 TargetIndex, IN const TInstancedStruct<FDynamicPassiveData>& State) const override;
};
