// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TacticalAttributeSet.generated.h"

// 최소한의 데이터 구조체
struct FTacticalAttribute
{
    float BaseValue;    // 장비나 레벨업 등으로 영구적으로 변하는 값
    float CurrentValue; // 버프/디버프가 적용된 실제 값
    float MinValue;
    float MaxValue;

    // 변경 로직(Clamp 포함)
    void ModifyValue(float Delta) { CurrentValue = FMath::Clamp(CurrentValue + Delta, MinValue, MaxValue); }
};

/**
 * 
 */
UCLASS()
class P_RD_API UTacticalAttributeSet : public UObject
{
	GENERATED_BODY()

public:
    virtual void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) {};
    virtual void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) {};
	
};
