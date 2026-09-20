#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_ReduceCooldown.h"
#include "TAS/Effect/Cooldown/TacticalEffect_ReduceCooldown.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"

void FSkillEffectLayer_ReduceCooldown::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

	/* 쿨다운 감소 이펙트 적용 */
	const int32 TargetNum = Params.mTargets.Num();
	for (int32 i = 0; i < TargetNum; ++i)
	{
		const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget = Params.mTargets[i];
        UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
        checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_ReduceCooldown::StaticClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = mCooldownReduction;
		AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
	}
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_ReduceCooldown"

FText FSkillEffectLayer_ReduceCooldown::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("ReduceCooldownDesc", "쿨다운을 {0} 감소시킵니다."),
		FText::AsNumber(mCooldownReduction)
	);
}

#undef LOCTEXT_NAMESPACE
