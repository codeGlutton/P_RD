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

	/* 1. 모션 및 이펙트 레이어 순회 */
	const int32 PhaseCount = mSkillPhaseLayers.Num();
	for (int32 PhaseIndex = 0; PhaseIndex < PhaseCount; ++PhaseIndex)
	{
		const FSkillPhaseLayer& MotionLayer = mSkillPhaseLayers[PhaseIndex];

        if (PhaseCount > 1)
        {
            FText MotionHeader = FText::Format(
                LOCTEXT("MotionHeaderFormat", "■ {0}타 모션:"),
                FText::AsNumber(PhaseIndex + 1)
            );
            DescriptionLines.Add(MotionHeader);
        }

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

