#include "Tutorial/StaticTutorialRoomSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UStaticTutorialRoomSpawnData::UStaticTutorialRoomSpawnData()
{
	mStageLevel = EStageLevelType::None;
	mUseRandomSpawnSetting = false;
	MoveDestination = FTileIndex::Invalid;
}

FTileIndex UStaticTutorialRoomSpawnData::GetTargetTile() const
{
	return mEnemyUnitPlacementDatas.IsValidIndex(TargetEnemyIndex)
		? mEnemyUnitPlacementDatas[TargetEnemyIndex].mTransform.mIndex : FTileIndex::Invalid;
}

bool UStaticTutorialRoomSpawnData::ValidateLayout(TArray<FText>& Errors) const
{
	auto Error = [&Errors](const FString& Text) { Errors.Add(FText::FromString(Text)); };
	if (mStageLevel != EStageLevelType::None) Error(TEXT("Tutorial StageLevel must be None; otherwise it enters normal random rooms."));
	if (mUseRandomSpawnSetting || mDefaultSpawnSettingName.IsNone()) Error(TEXT("Tutorial requires a named, fixed background spawn setting."));
	if (mPlayerTransforms.Num() != 3) Error(TEXT("Tutorial needs three party placements."));
	if (mEnemyUnitPlacementDatas.Num() < 2) Error(TEXT("Tutorial needs at least two enemies so one attack cannot skip End Turn."));
	if (GetTargetTile() == FTileIndex::Invalid) Error(TEXT("Tutorial TargetEnemyIndex must select an authored enemy."));
	TSet<FTileIndex> Occupied;
	auto Claim = [&Occupied, &Error](const FTileIndex& Tile)
	{
		if (Occupied.Contains(Tile)) Error(FString::Printf(TEXT("Tutorial overlapping occupancy at (%d,%d)."), Tile.mX, Tile.mY));
		Occupied.Add(Tile);
	};
	for (const auto& Player : mPlayerTransforms) Claim(Player.mIndex);
	for (const auto& Enemy : mEnemyUnitPlacementDatas) Claim(Enemy.mTransform.mIndex);
	for (const auto& Placement : mObstaclePlacementDatas)
	{
		Claim(Placement.mTransform.mIndex);
		const auto* Obstacle = Placement.mSpawnData.LoadSynchronous();
		if (!Obstacle) { Error(TEXT("Tutorial obstacle asset cannot be loaded.")); continue; }
		for (const FTileIndex& Relative : Obstacle->mRequiredEmptyTiles)
			Claim(LocalToTileMapTransform(FTileTransform(Relative), Placement.mTransform).mIndex);
	}
	if (MoveDestination == FTileIndex::Invalid || Occupied.Contains(MoveDestination)) Error(TEXT("Tutorial move destination must be an empty tile."));
	return Errors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UStaticTutorialRoomSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	const auto Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateLayout(Errors);
	for (const auto& Error : Errors) Context.AddError(Error);
	return Errors.IsEmpty() ? Parent : EDataValidationResult::Invalid;
}
#endif
