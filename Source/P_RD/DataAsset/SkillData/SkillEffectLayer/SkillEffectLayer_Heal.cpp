#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Heal.h"
#include "TAS/Effect/Stat/TacticalEffect_HealFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

TArray<FActiveTacticalEffectHandle> FSkillEffectLayer_Heal::ApplyFactorEffect(IBoardCombatTarget* ActorModel, const UBoardCombatTargetSnapshotData* Snapshot) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

    TArray<FActiveTacticalEffectHandle> EffectHandles;

    /* 포인트를 Factor에 임시 추가 */
    {
        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_HealFactor_AddBase::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = mHealGain;
        EffectHandles.Add(AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec));
    }

    return EffectHandles;
}

void FSkillEffectLayer_Heal::ClearFactorEffect(IBoardCombatTarget* ActorModel, TArray<FActiveTacticalEffectHandle>& Handles) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    /* 포인트를 Factor에서 제거 */
    for (FActiveTacticalEffectHandle& Handle : Handles)
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(Handle);
    }
}

void FSkillEffectLayer_Heal::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

    /* 힐 적용 */
    const int32 TargetNum = Params.mTargets.Num();
    for (int32 i = 0; i < TargetNum; ++i)
    {
        const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget = Params.mTargets[i];
        UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
        checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_Heal::StaticClass(), EffectContext);
        EffectSpec->SetInstigatorSnapshotData(Params.mInstigatorSnapshot);
        EffectSpec->SetTargetSnapshotData(Params.mTargetSnapshots[i]);
        AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
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
