#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_GetMove.h"
#include "TAS/Effect/Stat/TacticalEffect_MovementPoint.h"
#include "TAS/Effect/Stat/TacticalEffect_MovementFactor_AddBase.h"
#include "TAS/Effect/Stat/TacticalEffect_Movement.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include "Setting/GameBalanceSettings.h"

int32 FSkillEffectLayer_GetMove::CalculateMoveGain(IBoardCombatTarget* ActorModel, float DiceSum) const
{
    const UGameBalanceSettings* GameBalanceSettings = GetDefault<UGameBalanceSettings>();
    checkf(GameBalanceSettings != nullptr, TEXT("게임 밸런스 세팅 nullptr"));

    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    const float MovePoint = mDefaultMoveGain + DiceSum * mDiceRatio;
    const float MovementFactor = AttributeSetComponentModel->EvaluateAttributeWithAdditionalModifier(
        UCombatTargetAttributeSet::GetMovementFactorAttribute(), ETacticalModOp::AddBase, MovePoint);

    const bool IsOwnerAgility = AttributeSetComponentModel->HasMatchingGameplayTag(
        EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility);
    const float AgilityRatio = IsOwnerAgility
        ? GameBalanceSettings->mGlobalStatusEffectSetting.mEffectRatios[
            EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility]
        : 1.0f;

    return FMath::Max(0, FMath::FloorToInt(AgilityRatio * MovementFactor));
}

void FSkillEffectLayer_GetMove::ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(ActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_MovementPoint::StaticClass(), EffectContext);
    EffectSpec->mDynamicMagnitude = mDefaultMoveGain + DiceSum * mDiceRatio;
    AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
}

void FSkillEffectLayer_GetMove::ClearPointEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    AttributeSetComponentModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetMovementPointAttribute(), ETacticalModOp::Override, 0.f);
}

FActiveTacticalEffectHandle FSkillEffectLayer_GetMove::ApplyFactorEffect(IBoardCombatTarget* ActorModel) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(ActorModel));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    FActiveTacticalEffectHandle EffectHandle;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_MovementFactor_AddBase::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMovementPointAttribute());
        EffectHandle = AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec);
    }

    return EffectHandle;
}

void FSkillEffectLayer_GetMove::ClearFactorEffect(IBoardCombatTarget* ActorModel, FActiveTacticalEffectHandle Handle) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    /* 포인트를 Factor에서 제거 */
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(Handle);
    }
}

void FSkillEffectLayer_GetMove::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(Params.mInstigator.GetObject()));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    /* 이동 증가 적용 */
    const int32 TargetNum = Params.mTargets.Num();
    for (int32 i = 0; i < TargetNum; ++i)
    {
        const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget = Params.mTargets[i];
        UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
        checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_GetMovement::StaticClass(), EffectContext);
        EffectSpec->SetInstigatorSnapshotData(Params.mInstigatorSnapshot);
        EffectSpec->SetTargetSnapshotData(Params.mTargetSnapshots[i]);
        AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
    }
}
