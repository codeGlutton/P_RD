#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "TAS/Effect/Stat/TacticalEffect_AttackFactor.h"
#include "TAS/Effect/Stat/TacticalEffect_HP.h"
#include "TAS/Effect/Stat/TacticalEffect_Defense.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

TArray<FActiveTacticalEffectHandle> FSkillEffectLayer_Attack::ApplyFactorEffect(IBoardCombatTarget* ActorModel, const UBoardCombatTargetSnapshotData* Snapshot) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

    TArray<FActiveTacticalEffectHandle> EffectHandles;

    /* 기본 데미지를 Factor에 임시 추가 */
    {
        const FRandomStream& RandomStream = URandomStreamFunctionLibrary::GetEventStream(AttributeSetComponentModel);

        int32 StatusDamage = (
            Snapshot->mEffectCounts.FindRef(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Strength, 0) -
            Snapshot->mEffectCounts.FindRef(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Strength, 0)
            );
        int32 SkillDamage = RandomStream.RandRange(mMinDamage, mMaxDamage);

        int32 StatusCriticalPercent = (
            Snapshot->mEffectCounts.FindRef(EffectTags::GameplayEffect_StatusEffect_Infinite_Buff_Acumeny, 0) -
            Snapshot->mEffectCounts.FindRef(EffectTags::GameplayEffect_StatusEffect_Infinite_Debuff_Acumeny, 0)
            );
        const float CriticalPercent = RandomStream.FRand() * 100.f;

        const bool IsCritical = (CriticalPercent + StatusCriticalPercent) < AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetCriticalFactorAttribute());
        if (IsCritical == true)
        {
            SkillDamage = mMaxDamage;

            TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackFactor_MultiplyCompound::StaticClass(), EffectContext);
            EffectSpec->mDynamicMagnitude = 1.5;
            EffectHandles.Add(AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec));
        }

        TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(UTacticalEffect_AttackFactor_AddBase::StaticClass(), EffectContext);
        EffectSpec->mDynamicMagnitude = SkillDamage + StatusDamage;
        EffectHandles.Add(AttributeSetComponentModel->ApplyTacticalEffectSpecToSelf(*EffectSpec));
    }

    return EffectHandles;
}

void FSkillEffectLayer_Attack::ClearFactorEffect(IBoardCombatTarget* ActorModel, TArray<FActiveTacticalEffectHandle>& Handles) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = ActorModel->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    /* 포인트를 Factor에서 제거 */
    for (FActiveTacticalEffectHandle& Handle : Handles)
    {
        AttributeSetComponentModel->RemoveActiveTacticalEffect(Handle);
    }
}

void FSkillEffectLayer_Attack::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

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

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "SkillEffectLayer_Attack"

FText FSkillEffectLayer_Attack::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("AttackDesc", "대상에게 {0}~{1}의 데미지를 입힙니다."),
		FText::AsNumber(mMinDamage),
		FText::AsNumber(mMaxDamage)
	);
}

#undef LOCTEXT_NAMESPACE
#endif
