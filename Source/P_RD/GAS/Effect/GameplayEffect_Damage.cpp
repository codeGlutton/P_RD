// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayEffect_Damage.h"

UGameplayEffect_Damage::UGameplayEffect_Damage()
{
	// 얼마나 지속될 것인지를 결정한다.
	// Instant : 즉시 동작.
	// HasDuration : 일정시간 동작.
	// Infinite : 무한하게 동작.
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Attribute를 어떻게 바꿀지 방법을 적용한다.
	FGameplayModifierInfo	Modifier;

	// 바꿀 Attribute를 지정한다.
	Modifier.Attribute = UUnitAttributeSet::GetHPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	// 소모값이 바뀔 경우 아래 기능을 이용해서 처리하고 고정이라면 고정값을 지정한다.
	FSetByCallerFloat	Caller;

	Caller.DataTag = EffectTags::GameplayEffect_Skill_Data_Damage;

	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);

	// Modifier 배열에 추가한다.
	Modifiers.Add(Modifier);

	// Cue
	//FGameplayEffectCue	Cue;
	//Cue.GameplayCueTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Battle.Attack")));

	//GameplayCues.Add(Cue);
}