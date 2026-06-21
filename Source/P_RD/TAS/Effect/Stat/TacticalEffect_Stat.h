// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Stat.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UTacticalEffect_Stat : public UTacticalEffect
{
	GENERATED_BODY()
public:
	virtual void ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, const UTacticalEffectContext* EffectContext) override {};
};
