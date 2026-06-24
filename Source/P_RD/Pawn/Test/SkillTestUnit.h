// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawn/Unit.h"
#include "SkillTestUnit.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API ASkillTestUnit : public AUnit
{
	GENERATED_BODY()
	
	ASkillTestUnit();

public:
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ActivateSkill();
};
