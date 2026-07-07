#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Fortification.h"
#include "TAS/Effect/Tag/TacticalEffect_Fortification.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"

void FSkillEffectLayer_Fortification::CommitEffect(IBoardCombatTarget* OwnerActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = OwnerActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(OwnerActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    const float TagDiff = FMath::Floor(mDefaultTagGain + DiceSum * mDiceRatio);

    if (TagDiff > 0.f)
    {
        /* 버프 적용 */
        for (const IBoardCombatTarget* OtherCombatTarget : OtherCombatTargets)
        {
            UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
            checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

            TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Fortification::StaticClass(), EffectContext);
            EffectSpec->mDynamicMagnitude = TagDiff;
            AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
        }
    }
}
