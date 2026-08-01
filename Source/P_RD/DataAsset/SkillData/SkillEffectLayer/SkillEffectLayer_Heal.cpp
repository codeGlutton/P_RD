#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Heal.h"
#include "TAS/Effect/Stat/TacticalEffect_HealPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_HealFactor_AddBase.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

void FSkillEffectLayer_Heal::ApplyPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

    TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_HealPoint::StaticClass(), EffectContext);
    EffectSpec->mDynamicMagnitude = mHealGain;
    AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
}

void FSkillEffectLayer_Heal::ClearPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    AttributeSetComponentModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetHealPointAttribute(), ETacticalModOp::Override, 0.f);
}

FActiveTacticalEffectHandle FSkillEffectLayer_Heal::ApplyFactorEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

    FActiveTacticalEffectHandle EffectHandle;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_HealFactor_AddBase::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealPointAttribute());
        EffectHandle = AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
    }

    return EffectHandle;
}

void FSkillEffectLayer_Heal::ClearFactorEffect(IBoardCombatTarget* ActorModel, FActiveTacticalEffectHandle Handle) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    /* 포인트를 Factor에서 제거 */
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(Handle);
    }
}

void FSkillEffectLayer_Heal::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

    const float TotalHeal = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHealFactorAttribute());
    const float HealDiff = FMath::Floor(TotalHeal);

    if (HealDiff > 0.f)
    {
        /* 힐 적용 */
        for (const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget : Params.mTargets)
        {
            UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
            checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

            TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_HP::StaticClass(), EffectContext);
            EffectSpec->mDynamicMagnitude = HealDiff;
            AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
        }
    }
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Heal"

FText FSkillEffectLayer_Heal::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("HealDesc", "대상의 체력을 {0} 회복시킵니다."),
		FText::AsNumber(mHealGain)
	);
}

#undef LOCTEXT_NAMESPACE
#endif
