#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor_AddBase.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "TAS/Effect/Stat/TacticalEffect_Defense.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

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

void FSkillEffectLayer_Attack::ClearPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    AttributeSetComponentModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetAttackPointAttribute(), ETacticalModOp::Override, 0.f);
}

FActiveTacticalEffectHandle FSkillEffectLayer_Attack::ApplyFactorEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(ActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    FActiveTacticalEffectHandle EffectHandle;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackFactor_AddBase::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetAttackPointAttribute());
        EffectHandle = AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
    }

    return EffectHandle;
}

void FSkillEffectLayer_Attack::ClearFactorEffect(IBoardCombatTarget* ActorModel, FActiveTacticalEffectHandle Handle) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    /* 포인트를 Factor에서 제거 */
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(Handle);
    }
}

void FSkillEffectLayer_Attack::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(Params.mInstigator.GetObject()));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    /* 데미지 적용 */
    const int32 TargetNum = Params.mTargets.Num();
    for (int32 i = 0; i < TargetNum; ++i)
    {
        const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget = Params.mTargets[i];
        UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
        checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Attack::StaticClass(), EffectContext);
        EffectSpec->SetInstigatorSnapshotData(Params.mInstigatorSnapshot);
        EffectSpec->SetTargetSnapshotData(Params.mTargetSnapshots[i]);
        AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
    }
}
