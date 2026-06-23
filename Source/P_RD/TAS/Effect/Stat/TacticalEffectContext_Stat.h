// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TAS/Effect/TacticalEffectContext.h"
#include "UObject/Object.h"
#include "TacticalEffectContext_Stat.generated.h"

UCLASS()
class P_RD_API UTacticalEffectContext_Stat : public UTacticalEffectContext
{
	GENERATED_BODY()
public:
	float mBase;
	float mAdd;
	float mMul;
	float GetFinal() const
	{
		return (mBase + mAdd) * mMul;
	}
};