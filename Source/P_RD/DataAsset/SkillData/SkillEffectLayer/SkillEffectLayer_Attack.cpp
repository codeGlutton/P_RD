#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "TAS/Effect/Stat/TacticalEffect_Defense.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/UnitAttributeSet.h"

void FSkillEffectLayer_Attack::ClearPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    AttributeSetComponentModel->ApplyModToAttribute(UUnitAttributeSet::GetAttackPointAttribute(), ETacticalModOp::Override, 0.f);
}

void FSkillEffectLayer_Attack::ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(ActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackPoint::StaticClass(), EffectContext);
    EffectSpec->mDynamicMagnitude = mDefaultDamage + DiceSum * mDiceRatio;
    AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
}

void FSkillEffectLayer_Attack::CommitEffect(IBoardCombatTarget* OwnerActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = OwnerActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(OwnerActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    FActiveTacticalEffectHandle EffectHandle;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackFactor::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackPointAttribute());
        EffectHandle = AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
    }

    // 약화
    const bool IsOwnerWeakness = AttributeSetComponentModel->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness);
    const float WeaknessRatio = IsOwnerWeakness == true ? 0.75f : 1.f;

    /* 데미지 적용 */
    for (const IBoardCombatTarget* OtherCombatTarget : OtherCombatTargets)
    {
        UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
        checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

        // 취약
        const bool IsTargetVulnerability = OtherAttributeSetComponentModel->HasMatchingGameplayTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability);
        const float VulnerabilityRatio = IsTargetVulnerability == true ? 1.5f : 1.f;
        
        // 최종 공격력과 방어력
        const int32 TotalAttack = FMath::Floor(VulnerabilityRatio * WeaknessRatio * AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackFactorAttribute()));
        const int32 TotalDefense = FMath::Floor(OtherAttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefenseAttribute()));

        /* 방어력 까기 */
        {
            const float DefenseDiff = -FMath::Min(TotalAttack, TotalDefense);
            if (DefenseDiff < 0)
            {
                TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Defense::StaticClass(), EffectContext);
                EffectSpec->mDynamicMagnitude = DefenseDiff;
                AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
            }
        }
        /* 체력 까기 */
        {
            const float HPDiff = TotalDefense - TotalAttack;
            if (HPDiff < 0)
            {
                TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_HP::StaticClass(), EffectContext);
                EffectSpec->mDynamicMagnitude = HPDiff;
                AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
            }
        }
    }

    /* 포인트를 Factor에서 제거 */
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(EffectHandle);
    }
}
