/*****************************************************************//**
 * @file   TacticalEffect_SpawnBoardActor.h
 * @brief  보드 액터 스폰 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-09-05
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_SpawnBoardActor.generated.h"

/**
 * @brief 보드 액터를 스폰하는 이펙트 클래스
 */
UCLASS()
class P_RD_API UTacticalEffect_SpawnBoardActor : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpawnBoardActor();

	/* UTacticalEffect 상속 */
public:
	void OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const override;
};
