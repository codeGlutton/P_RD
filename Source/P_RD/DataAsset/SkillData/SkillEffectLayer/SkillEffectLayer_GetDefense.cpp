#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_GetDefense.h"
#include "TAS/Effect/Stat/TacticalEffect_DefensePoint.h"
#include "TAS/Effect/Stat/TacticalEffect_DefenseFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_Defense.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/UnitAttributeSet.h"

void FSkillEffectLayer_GetDefense::ClearPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    AttributeSetComponentModel->ApplyModToAttribute(UUnitAttributeSet::GetDefensePointAttribute(), ETacticalModOp::Override, 0.f);
}

void FSkillEffectLayer_GetDefense::ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(ActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_DefensePoint::StaticClass(), EffectContext);
    EffectSpec->mDynamicMagnitude = mDefaultDefenseGain + DiceSum * mDiceRatio;
    AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
}

void FSkillEffectLayer_GetDefense::CommitEffect(IBoardCombatTarget* OwnerActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = OwnerActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(OwnerActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    FActiveTacticalEffectHandle EffectHandle;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_DefenseFactor::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute());
        EffectHandle = AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
    }

    // 민첩성
    const bool IsOwnerFortification = AttributeSetComponentModel->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_Buff_Fortification);
    const float FortificationRatio = IsOwnerFortification == true ? 1.25f : 1.f;

    const float TotalDefense = FortificationRatio * AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefenseFactorAttribute());
    const float DefenseDiff = FMath::Floor(TotalDefense);

    if (DefenseDiff > 0.f)
    {
        /* 방어력 증가 적용 */
        for (const IBoardCombatTarget* OtherCombatTarget : OtherCombatTargets)
        {
            UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
            checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

            TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Defense::StaticClass(), EffectContext);
            EffectSpec->mDynamicMagnitude = DefenseDiff;
            AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
        }
    }

    /* 포인트를 Factor에서 제거 */
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(EffectHandle);
    }
}
