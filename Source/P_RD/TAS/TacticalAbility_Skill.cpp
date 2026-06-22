// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/TacticalAbility_Skill.h"
#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Stat.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"




UTacticalAbility_Skill::UTacticalAbility_Skill()
{
}

void UTacticalAbility_Skill::ApplyCost() const
{
	
}

void UTacticalAbility_Skill::ActivateAbility(const FTacticalAbilityContext Context)
{
	Super::ActivateAbility(Context);

	if (!IsValid(Context.mCasterActor.Get()) ||
		!Context.mTargetTile.Num() ||
		Context.mInstigatorData->mTacticalEffectPayloadType != ETacticalEffectPayloadType::Skill)
	{
		EndAbility(Context);
	}

	if (!CanActivateAbility(Context))
	{
		EndAbility(Context);
	}


	// 스킬을 사용한다.
	ActivateSkill(Context);

	EndAbility(Context);
}

void UTacticalAbility_Skill::EndAbility(const FTacticalAbilityContext& Context)
{
	Super::EndAbility(Context);

}

bool UTacticalAbility_Skill::CanActivateAbility(const FTacticalAbilityContext& Context) const
{
	// 주사위의 개수를 검사한다.
	// 주체자의 상태를 검사한다.

	// 우선은 그냥 무조건 가능
	return true;
}

void UTacticalAbility_Skill::CallStartCalculatePassive(const FTacticalAbilityContext& Context, TArray<UTacticalEffectContext*>& EffectContext)
{
}

void UTacticalAbility_Skill::CallEndCalculatePassive(const FTacticalAbilityContext& Context, TArray<UTacticalEffectContext*>& EffectContext)
{
}

void UTacticalAbility_Skill::ActivateSkill(const FTacticalAbilityContext& Context)
{
	// 스킬을 기반으로 효과를 계산한다.
	TSoftObjectPtr<UStaticSkillData> SkillData;

	TWeakObjectPtr<UTacticalEffectPayload_Skill> Payload_Skill = Cast<UTacticalEffectPayload_Skill>(Context.mInstigatorData);
	checkf(!Payload_Skill.IsValid(), TEXT("잘못된 Context 구성체"));

	SkillData = Payload_Skill.Get()->mSkillData;

	for (int32 i = 0; i < SkillData.Get()->mSkillMotionLayers.Num(); ++i)
	{
		TArray<UTacticalEffectContext*> EffectContexts;
		const FSkillMotionLayer& SkillMotionLayer = SkillData.Get()->mSkillMotionLayers[i];

		// Context 오브젝트를 생성합니다.
		// 추후 팩토리 구성으로 Context 생성하도록 희망
		UTacticalEffectContext* EffectContext = SkillMotionLayer.mStaticSkillEffectLayers->CreateContext(Context.mCasterActor);
		EffectContexts.Add(EffectContext);

		// 효과를 계산합니다.
		CallStartCalculatePassive(Context, EffectContexts);

		// 효과를 적용한다.
		ApplyEffect(Context, EffectContexts);

		// 효과를 정산합니다.
		CallEndCalculatePassive(Context, EffectContexts);

	}

}
