#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

TArray<FTileIndex> FSkillPhaseLayer::FilterTileIndexes(const FTileIndex& SelfIndex, const TArray<FTileIndex>& TargetTileIndexes) const
{
    TArray<FTileIndex> FilteredTileIndexes;
    if (EnumHasAllFlags(StaticCast<ETargetIndexFilter>(mTargetIndexFilter), ETargetIndexFilter::IncludeTargetIndexes) == true)
    {
        FilteredTileIndexes.Append(TargetTileIndexes);
    }
    if (EnumHasAllFlags(StaticCast<ETargetIndexFilter>(mTargetIndexFilter), ETargetIndexFilter::IncludeSelfIndex) == true)
    {
        FilteredTileIndexes.AddUnique(SelfIndex);
    }
    return FilteredTileIndexes;
}

TArray<IBoardCombatTarget*> FSkillPhaseLayer::FilterCombatTargets(const UTileMapModel* MapModel, const IBoardCombatTarget* SelfInstigator, const TArray<FTileIndex>& FilteredTileIndexes) const
{
    TArray<IBoardCombatTarget*> FilteredCombatTargets;
    for (const FTileIndex& FilteredTileIndex : FilteredTileIndexes)
    {
        TArray<UBoardActorModel*> BoardActors = MapModel->GetActorsOnTile(FilteredTileIndex);
        for (UBoardActorModel*& BoardActor : BoardActors)
        {
            IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(BoardActor);
            if (CombatTarget != nullptr && CombatTarget->IsTargetable() == true)
            {
                ETeamAttitudeFilter CombatTargetAttitude = StaticCast<ETeamAttitudeFilter>(1 << SelfInstigator->GetTeamAttitudeTowards(*BoardActor));
                if (EnumHasAnyFlags(StaticCast<ETeamAttitudeFilter>(mTeamAttitudeFilter), CombatTargetAttitude) == true)
                {
                    FilteredCombatTargets.Add(CombatTarget);
                }
            }
        }
    }
    return FilteredCombatTargets;
}

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "StaticSkillData"

FText UStaticSkillData::MakeDescription() const
{
	TArray<FText> DescriptionLines;

	/* 1. 패턴 이름 및 헤더 정보 현지화 합성 (RDMinimal.h의 EnumToText 활용) */
	FText BaseHeader = FText::Format(
		LOCTEXT("SkillDescriptionHeader", "[행동력: {0} | 사거리: {1} ({2}) | 적용 범위: {3} ({4}) | 쿨다운: {5}턴]"),
		FText::AsNumber(mRequiredActionPoint),
		FText::AsNumber(mAimRange),
		EnumToText(mAimPattern),
		FText::AsNumber(mEffectArea),
		EnumToText(mEffectPattern),
		FText::AsNumber(mCooldownDuration)
	);
	DescriptionLines.Add(BaseHeader);

	/* 2. 모션 및 이펙트 레이어 순회 */
	const int32 MotionCount = mSkillPhaseLayers.Num();
	for (int32 MotionIndex = 0; MotionIndex < MotionCount; ++MotionIndex)
	{
		const FSkillPhaseLayer& MotionLayer = mSkillPhaseLayers[MotionIndex];

		FText MotionHeader = FText::Format(
			LOCTEXT("MotionHeaderFormat", "■ {0}타 모션:"),
			FText::AsNumber(MotionIndex + 1)
		);
		DescriptionLines.Add(MotionHeader);

		for (const TInstancedStruct<FSkillEffectLayer>& InstancedEffect : MotionLayer.mSkillEffectLayers)
		{
			if (InstancedEffect.IsValid() == true)
			{
				const FSkillEffectLayer& EffectLayer = InstancedEffect.Get();
				FText EffectText = EffectLayer.MakeDescription();
				if (EffectText.IsEmpty() == false)
				{
					FText FormattedEffect = FText::Format(
						LOCTEXT("EffectLineFormat", "- {0}"),
						EffectText
					);
					DescriptionLines.Add(FormattedEffect);
				}
			}
		}
	}

	/* 3. 줄바꿈 조합 반환 (FText::Join 활용) */
	return FText::Join(FText::FromString(TEXT("\n")), DescriptionLines);
}

EDataValidationResult UStaticSkillData::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult SuperResult = Super::IsDataValid(Context);
    EDataValidationResult ThisResult = EDataValidationResult::Valid;

    if (mName.IsEmpty() == true)
    {
        Context.AddError(FText::FromString(TEXT("스킬 이름 미지정")));
        ThisResult = EDataValidationResult::Invalid;
    }
    if (mJobType == EPlayerJobType::None)
    {
        Context.AddError(FText::FromString(TEXT("스킬 직업 분류 미지정")));
        ThisResult = EDataValidationResult::Invalid;
    }
    if (mCooldownEffectClass.ToSoftObjectPath().IsValid() == false)
    {
        Context.AddError(FText::FromString(TEXT("쿨다운 타입 미지정")));
        ThisResult = EDataValidationResult::Invalid;
    }
    if (mSkillPhaseLayers.IsEmpty() == true)
    {
        Context.AddError(FText::FromString(TEXT("스킬 모션 미지정")));
        ThisResult = EDataValidationResult::Invalid;
    }

    return CombineDataValidationResults(SuperResult, ThisResult);
}

#undef LOCTEXT_NAMESPACE
#endif

