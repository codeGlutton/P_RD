// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/Effect/Tag/TacticalEffect_Exhaustion.h"
#include "TacticalEffect_Exhaustion.h"

UTacticalEffect_Exhaustion::UTacticalEffect_Exhaustion()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Exhaustion;
}

UTacticalEffect_GetExhaustion::UTacticalEffect_GetExhaustion()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Exhaustion;
}
