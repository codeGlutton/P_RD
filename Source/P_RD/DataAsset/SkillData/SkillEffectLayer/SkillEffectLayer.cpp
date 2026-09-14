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
	TArray<FTileIndex>& FinalTileIndexes,
	FTileIndex& AimedTileIndex
) :
	mInstigator(Instigator),
	mInstigatorSnapshot(InstigatorSnapshot),
	mTargets(Targets),
	mTargetSnapshots(TargetSnapshots),
	mFinalTileIndexes(FinalTileIndexes),
	mAimedTileIndex(AimedTileIndex)
{

}

TSubclassOf<UTacticalEffect> FSkillEffectLayer_UniqueTagBase::GetTagEffectClass() const
{
	return UTacticalEffect::StaticClass();
}

void FSkillEffectLayer_UniqueTagBase::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

	/* 버프 적용 */
	for (const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget : Params.mTargets)
	{
		UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
		checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

		TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(GetTagEffectClass(), EffectContext);
		EffectSpec->mDynamicMagnitude = 1.f;
		AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
	}
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_UniqueTagBase"

FText FSkillEffectLayer_UniqueTagBase::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("UniqueTagGainFormat", "{0} 태그를 부여합니다."),
		GetTagDisplayName()
	);
}

#undef LOCTEXT_NAMESPACE

void FSkillEffectLayer_TagBase::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

	const float TagDiff = static_cast<float>(mTagGain);

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

#define LOCTEXT_NAMESPACE "SkillEffectLayer_TagBase"

FText FSkillEffectLayer_TagBase::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("TagGainFormat", "{0} 태그를 {1} 중첩 부여합니다."),
		GetTagDisplayName(),
		FText::AsNumber(mTagGain)
	);
}

#undef LOCTEXT_NAMESPACE

void FSkillEffectLayer_AttributeTagBase::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Params.mInstigator->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UTacticalEffectContext* EffectContext = AttributeSetComponentModel->MakeEffectContext();

	if (mTagGain != 0)
	{
		/* 버프 적용 */
		for (const TScriptInterface<IBoardCombatTarget>& OtherCombatTarget : Params.mTargets)
		{
			UAttributeSetComponentModel* OtherAttributeSetComponentModel = OtherCombatTarget->GetAttributeComponentModel();
			checkf(OtherAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

			TSharedPtr<FTacticalEffectSpec> EffectSpec = AttributeSetComponentModel->MakeOutgoingSpec(GetTagEffectClass(), EffectContext);
			EffectSpec->mDynamicMagnitude = static_cast<float>(FMath::Abs(mTagGain));
			AttributeSetComponentModel->ApplyTacticalEffectSpecToTarget(*EffectSpec, OtherAttributeSetComponentModel);
		}
	}
}

