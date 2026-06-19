// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/TacticalAbility_Skill.h"
#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Stat.h"



UTacticalAbility_Skill::UTacticalAbility_Skill()
{
}

void UTacticalAbility_Skill::ApplyCost() const
{
	
}

void UTacticalAbility_Skill::ActivateAbility(const FTacticalAbilityContext Context)
{
	if (!IsValid(Context.mCasterActor.GetObject()) ||
		!Context.mTargetTile.Num() ||
		Context.mInstigatorData.mTacticalEffectPayloadType != ETacticalEffectPayloadType::Skill)
	{
		EndAbility();
	}

	if (!CanActivateAbility(Context))
	{
		EndAbility();
	}

	// 효과를 계산한다.
	TArray<UTacticalEffectContext*> EffectContext = CalculateEffect_Stat(Context);

	// 효과를 적용한다.
	ApplyEffect(Context, EffectContext);

	EndAbility();
}

void UTacticalAbility_Skill::EndAbility()
{

}

bool UTacticalAbility_Skill::CanActivateAbility(const FTacticalAbilityContext& Context) const
{
	// 주사위의 개수를 검사한다.
	// 주체자의 상태를 검사한다.

	// 우선은 그냥 무조건 가능
	return true;
}

TArray<UTacticalEffectContext*> UTacticalAbility_Skill::CalculateEffect_Stat(const FTacticalAbilityContext& Context)
{
	TArray<UTacticalEffectContext*> EffectContexts;

	// 캐스터의 주사위 눈금을 가져온다.

	// 스킬을 기반으로 효과를 계산한다.
	TSoftObjectPtr<UStaticSkillData> SkillData;

	for (int32 i = 0; i < SkillData.Get()->mSkillMotionLayers.Num(); ++i)
	{
		const FSkillMotionLayer& SkillMotionLayer = SkillData.Get()->mSkillMotionLayers[i];
		

		for (int32 j = 0; j < SkillMotionLayer.mStaticSkillEffectLayers.Num(); ++j)
		{
			// 생성
			UTacticalEffectContext_Stat* EffectContext = NewObject<UTacticalEffectContext_Stat>();

			const UStaticSkillEffect_Stat* Effect_Stat = Cast<UStaticSkillEffect_Stat>(SkillMotionLayer.mStaticSkillEffectLayers[j]);
			checkf(Effect_Stat != 0, TEXT("Stat이 아닙니다."));
			EffectContext->mBase = Effect_Stat->mEffectDefaultValue + Effect_Stat->mEffectRatioValue * 10;
			EffectContext->mGameplayTag = Effect_Stat->mGameplayTag;

			EffectContexts.Add(EffectContext);
		}

	}


	// 효과를 반환한다.
	return EffectContexts;
}
