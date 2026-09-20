#include "DataAsset/SkillData/StaticSkillData.h"
#include "Engine/Texture2D.h"

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

#define LOCTEXT_NAMESPACE "StaticSkillData"

namespace
{
	FText GetTargetIndexFilterText(int32 TargetIndexFilter)
	{
		const ETargetIndexFilter Filter = StaticCast<ETargetIndexFilter>(TargetIndexFilter);
		const bool IsIncludeSelf = EnumHasAllFlags(Filter, ETargetIndexFilter::IncludeSelfIndex);
		const bool IsIncludeTarget = EnumHasAllFlags(Filter, ETargetIndexFilter::IncludeTargetIndexes);

		if (IsIncludeSelf == true && IsIncludeTarget == true)
		{
			return LOCTEXT("TargetIndexFilter_SelfAndTarget", "시전자 및 타겟 타일들");
		}
		if (IsIncludeSelf == true)
		{
			return LOCTEXT("TargetIndexFilter_Self", "시전자 타일");
		}
		if (IsIncludeTarget == true)
		{
			return LOCTEXT("TargetIndexFilter_Target", "타겟 타일들");
		}
		return LOCTEXT("TargetIndexFilter_None", "없음");
	}

	FText GetTeamAttitudeFilterText(int32 TeamAttitudeFilter)
	{
		const ETeamAttitudeFilter Filter = StaticCast<ETeamAttitudeFilter>(TeamAttitudeFilter);
		TArray<FText> TeamTexts;

		if (EnumHasAllFlags(Filter, ETeamAttitudeFilter::Hostile) == true)
		{
			TeamTexts.Add(LOCTEXT("Team_Hostile", "적"));
		}
		if (EnumHasAllFlags(Filter, ETeamAttitudeFilter::Friendly) == true)
		{
			TeamTexts.Add(LOCTEXT("Team_Friendly", "아군"));
		}
		if (EnumHasAllFlags(Filter, ETeamAttitudeFilter::Neutral) == true)
		{
			TeamTexts.Add(LOCTEXT("Team_Neutral", "중립"));
		}

		if (TeamTexts.IsEmpty() == true)
		{
			return LOCTEXT("Team_None", "없음");
		}

		return FText::Join(FText::FromString(TEXT("/")), TeamTexts);
	}
}

FText UStaticSkillData::MakeDescription() const
{
	TArray<FText> DescriptionLines;

	/* 1. 모션 및 이펙트 레이어 순회 */
	const int32 PhaseCount = mSkillPhaseLayers.Num();
	for (int32 PhaseIndex = 0; PhaseIndex < PhaseCount; ++PhaseIndex)
	{
		const FSkillPhaseLayer& MotionLayer = mSkillPhaseLayers[PhaseIndex];

		const FText TargetFilterText = GetTargetIndexFilterText(MotionLayer.mTargetIndexFilter);
		const FText TeamFilterText = GetTeamAttitudeFilterText(MotionLayer.mTeamAttitudeFilter);

		FText MotionHeader = FText::Format(
			LOCTEXT("MotionHeaderFormat", "■ {0}타 모션 ({1}, {2}):"),
			FText::AsNumber(PhaseIndex + 1),
			TargetFilterText,
			TeamFilterText
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

#if WITH_EDITOR
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

    // 밀치기/당기기 배치 순서 검증
    // 밀당이 중간에 끼면 시전자와 피격자의 모션이 어긋나서 어색할 수 있으므로 의도가 맞는 지 한 번 더 확인해보라는 의미에서 경고
    const int32 LastPhaseIndex = mSkillPhaseLayers.Num() - 1;
    for (int32 PhaseIndex = 0; PhaseIndex <= LastPhaseIndex; ++PhaseIndex)
    {
        const TArray<TInstancedStruct<FSkillEffectLayer>>& Layers = mSkillPhaseLayers[PhaseIndex].mSkillEffectLayers;
        bool HasPassedForcedMovement = false;
        for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); ++LayerIndex)
        {
            if (Layers[LayerIndex].IsValid() == false)
            {
                continue;
            }

            // 마지막 페이즈가 아닌 곳의 밀당
            const bool HasForced = Layers[LayerIndex].Get().HasForcedMovement();
            if (HasForced == true && PhaseIndex != LastPhaseIndex)
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT("밀치기/당기기를 마지막 페이즈 아닌 곳에 배치 (%d타)"), PhaseIndex + 1)));
            }

            // 같은 페이즈에서 밀당 뒤에 오는 다른 이펙트
            if (HasForced == true)
            {
                HasPassedForcedMovement = true;
            }
            else if (HasPassedForcedMovement == true)
            {
                Context.AddWarning(FText::FromString(FString::Printf(TEXT("밀치기/당기기 뒤에 다른 이펙트 배치 (%d타 %d번째)"), PhaseIndex + 1, LayerIndex + 1)));
            }
        }
    }

    return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif

#undef LOCTEXT_NAMESPACE


void UStaticSkillData::PostLoad()
{
	Super::PostLoad();
	// Legacy skills without authored artwork use a neutral skill symbol in every UI.
	if (mIcon.IsNull())
		mIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Concept02/T_nav_skill_icon.T_nav_skill_icon")));
}
