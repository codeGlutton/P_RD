// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/Effect/Tag/TacticalEffect_Haste.h"

UTacticalEffect_Haste::UTacticalEffect_Haste()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Haste;
}

UTacticalEffect_GetHaste::UTacticalEffect_GetHaste()
{
	mStatusTag = EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Haste;
}
