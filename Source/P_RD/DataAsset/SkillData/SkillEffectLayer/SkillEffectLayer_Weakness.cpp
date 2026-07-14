#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Weakness.h"
#include "TAS/Effect/Tag/TacticalEffect_Weakness.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"

void FSkillEffectLayer_Weakness::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(Params.mInstigator.GetObject()));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    const float TagDiff = FMath::Floor(mDefaultTagGain + Params.mDiceSum * mDiceRatio);

    if (TagDiff > 0.f)
    {
        /* 디버프 적용 */
        for (const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget : Params.mTargets)
        {
            UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
            checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

            TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Weakness::StaticClass(), EffectContext);
            EffectSpec->mDynamicMagnitude = TagDiff;
            AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
        }
    }
}
