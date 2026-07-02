// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "UObject/Object.h"
#include "InputData.generated.h"

UCLASS()
class P_RD_API UInputData : public UObject
{
	GENERATED_BODY()

public:
	UInputData();

public:
	TObjectPtr<UInputMappingContext>	mContext;

protected:
	TMap<FString, TObjectPtr<UInputAction>>	mActions;

public:
	TObjectPtr<UInputAction> FindAction(const FString& Name)	const;
};

UCLASS()
class P_RD_API UCombatInputData : public UInputData
{
	GENERATED_BODY()

public:
	UCombatInputData();
};