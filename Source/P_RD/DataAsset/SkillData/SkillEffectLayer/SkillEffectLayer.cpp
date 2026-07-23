#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"

#include "Actor/ActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "TAS/Effect/TacticalEffectContext.h"

FSkillEffectCommitParams::FSkillEffectCommitParams(
	TScriptInterface<IBoardCombatTarget> Instigator,
	TObjectPtr<UBoardCombatTargetSnapshotData> InstigatorSnapshot,
	TArray<TScriptInterface<IBoardCombatTarget>>& Targets,
	TArray<TObjectPtr<UBoardCombatTargetSnapshotData>>& TargetSnapshots,
	TArray<FTileIndex>& TargetTileIndexes
) :
	mInstigator(Instigator),
	mInstigatorSnapshot(InstigatorSnapshot),
	mTargets(Targets),
	mTargetSnapshots(TargetSnapshots),
	mTargetTileIndexes(TargetTileIndexes)
{

}

TSubclassOf<UTacticalEffect> FSkillEffectLayer_TagBase::GetTagEffectClass() const
{
    return UTacticalEffect::StaticClass();
}

void FSkillEffectLayer_TagBase::CommitEffect(const FSkillEffectCommitParams& Params) const
{
    UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
    checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

    UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();
    EffectContext->SetInstigator(Cast<UActorModel>(Params.mInstigator.GetObject()));
    EffectContext->SetAttributeSetComponentModel(AttributeSetComponentModel);

    const float TagDiff = mTagGain;

    if (TagDiff > 0.f)
    {
        /* 버프 적용 */
        for (const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget : Params.mTargets)
        {
            UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
            checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

            TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(GetTagEffectClass(), EffectContext);
            EffectSpec->mDynamicMagnitude = TagDiff;
            AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
        }
    }
}
