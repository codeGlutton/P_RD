// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatResult.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CombatCalculatorFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UCombatCalculatorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	static bool CalculateSkillResult(const class UStaticSkillData* SkillData, TArray<FTileIndex> Tiles, FSkillCommitResult& Out_Result);
};
