#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

TArray<FTileIndex> FSkillMotionLayer::FilterTileIndexes(const FTileIndex& SelfIndex, const TArray<FTileIndex>& TargetTileIndexes) const
{
    TArray<FTileIndex> FilteredTileIndexes;
    if (EnumHasAllFlags(StaticCast<ETargetIndexFilter>(mTargetIndexFilter), ETargetIndexFilter::IncludeSelfIndex) == true)
    {
        FilteredTileIndexes.Add(SelfIndex);
    }
    if (EnumHasAllFlags(StaticCast<ETargetIndexFilter>(mTargetIndexFilter), ETargetIndexFilter::IncludeTargetIndexes) == true)
    {
        FilteredTileIndexes.Append(TargetTileIndexes);
    }
    return FilteredTileIndexes;
}

TArray<IBoardCombatTarget*> FSkillMotionLayer::FilterCombatTargets(const UTileMapModel* MapModel, const IBoardCombatTarget* SelfInstigator, const TArray<FTileIndex>& FilteredTileIndexes) const
{
    TArray<IBoardCombatTarget*> FilteredCombatTargets;
    for (const FTileIndex& FilteredTileIndex : FilteredTileIndexes)
    {
        TArray<UBoardActorModel*> BoardActors = MapModel->GetActorsOnTile(FilteredTileIndex);
        for (UBoardActorModel*& BoardActor : BoardActors)
        {
            IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(BoardActor);
            if (CombatTarget != nullptr)
            {
                const ETeamAttitudeFilter CombatTargetAttitude = StaticCast<ETeamAttitudeFilter>(1 << SelfInstigator->GetTeamAttitudeTowards(*BoardActor));
                if (EnumHasAllFlags(StaticCast<ETeamAttitudeFilter>(mTeamAttitudeFilter), CombatTargetAttitude) == true)
                {
                    FilteredCombatTargets.Add(CombatTarget);
                }
            }
        }
    }
    return FilteredCombatTargets;
}
