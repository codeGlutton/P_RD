#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Spawn.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMapModel.h"

bool FDynamicPlacementData::IsUnit() const
{
	if (mSpawnData.IsNull() == true)
	{
		return false;
	}

	const UStaticObstacleSpawnData* LoadedData = mSpawnData.Get();
	if (LoadedData == nullptr)
	{
		LoadedData = mSpawnData.LoadSynchronous();
	}

	return Cast<UStaticUnitSpawnData>(LoadedData) != nullptr;
}

FEnemyUnitPlacementData FDynamicPlacementData::MakeEnemyUnitPlacementData(const FTileTransform& TargetTileTransform) const
{
	FEnemyUnitPlacementData PlacementData;
	PlacementData.mSpawnData = Cast<UStaticEnemyUnitSpawnData>(mSpawnData.LoadSynchronous());
	PlacementData.mTransform = TargetTileTransform;
	PlacementData.mDifficulty = mDifficulty;
	PlacementData.mDefaultSpeedPoint = mDefaultSpeedPoint;

	return PlacementData;
}

FObstaclePlacementData FDynamicPlacementData::MakeObstaclePlacementData(const FTileTransform& TargetTileTransform) const
{
	FObstaclePlacementData PlacementData;
	PlacementData.mSpawnData = mSpawnData;
	PlacementData.mTransform = TargetTileTransform;

	return PlacementData;
}

void FSkillEffectLayer_Spawn::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	if (Params.mInstigator == nullptr || mSpawnDatas.IsEmpty() == true)
	{
		return;
	}

	UObject* InstigatorObject = Params.mInstigator.GetObject();
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(InstigatorObject);
	UTileMapModel* TileMapModel = CombatModel->GetTileMap();
	
	const FTileTransform& OriginTileTransform = Params.mInstigatorSnapshot->mTileTransform;
	int32 mSpawnDataIndex = 0;
	
	for (const FTileIndex& FinalTileIndex : Params.mFinalTileIndexes)
	{
		const FDynamicPlacementData& SpawnData = mSpawnDatas[mSpawnDataIndex];
		const UStaticObstacleSpawnData* ObstacleSpawnData = SpawnData.mSpawnData.LoadSynchronous();
		const TSubclassOf<UBoardActorModel> BoardActorModelClass = ObstacleSpawnData->mModelClass.LoadSynchronous();

		if (TileMapModel->CanPlace(FinalTileIndex, GetDefault<UBoardActorModel>(BoardActorModelClass)) == true)
		{
			if (SpawnData.IsUnit())
			{
				FEnemyUnitPlacementData EnemyPlacement = SpawnData.MakeEnemyUnitPlacementData(FTileTransform(FinalTileIndex, OriginTileTransform.mDirection));
				if (CombatModel != nullptr)
				{
					CombatModel->RegisterEnemyUnitModel(EnemyPlacement);
				}
			}
			else
			{
				FObstaclePlacementData ObstaclePlacement = SpawnData.MakeObstaclePlacementData(FTileTransform(FinalTileIndex, OriginTileTransform.mDirection));
				if (CombatModel != nullptr)
				{
					CombatModel->RegisterObstacleModel(ObstaclePlacement);
				}
			}
		}

		mSpawnDataIndex = (mSpawnDataIndex + 1) % mSpawnDatas.Num();
	}
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Spawn"

FText FSkillEffectLayer_Spawn::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("SpawnDescriptionFormat", "지정 위치에 보드 액터 {0}개를 차례로 스폰합니다."),
		FText::AsNumber(mSpawnDatas.Num())
	);
}

#undef LOCTEXT_NAMESPACE
