// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GAS/GASMinimal.h"
#include "UObject/Object.h"
#include "TacticalEffectContext.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UTacticalEffectContext : public UObject
{
	GENERATED_BODY()
public:
	FGameplayTag mGameplayTag;
};
