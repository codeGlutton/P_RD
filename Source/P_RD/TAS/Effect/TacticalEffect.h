// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TacticalEffect.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UTacticalEffect : public UObject
{
	GENERATED_BODY()


public:
	virtual void ActivateEffect(const class UBoardActorModel& Caster, const struct FTileIndex& TargetTile, TArray<class UTacticalEffectContext*>& EffectContexts) PURE_VIRTUAL(UTacticalEffect::ActivateEffect, return;);

};