// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TacticalEffect.generated.h"

class UBoardActorModel;
struct FTileIndex;
class UTacticalEffectContext;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class P_RD_API UTacticalEffect : public UObject
{
	GENERATED_BODY()


public:
	virtual void ActivateEffect(const UBoardActorModel& Caster, const FTileIndex& TargetTile, const UTacticalEffectContext* EffectContext) PURE_VIRTUAL(UTacticalEffect::ActivateEffect, return;);

};