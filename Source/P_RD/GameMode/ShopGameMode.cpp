#include "GameMode/ShopGameMode.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "Simulation/Factory/ObjectModelFactory.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Setting/RDWorldSettings.h"

void AShopGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FStage& CurStage = GetRunPersistData()->GetStage();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticStageSpawnData* StaticStageData = AssetManager->GetPrimaryAssetObject<UStaticStageSpawnData>(CurStage.mStaticStageSpawnDataId);
	checkf(StaticStageData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = StaticStageData->mShopRoomBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous(), false);
}

void AShopGameMode::InitializeRoom()
{
	Super::InitializeRoom();

	SpawnTileMap();
}

void AShopGameMode::SpawnTileMap()
{
	checkf(mTileMap == nullptr, TEXT("이미 타일 존재"));

	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RD 월드 세팅 nullptr"));

	// 스폰 세팅의 스타트포인트 트랜스폼 기준으로 타일맵 배치
	FTransform SpawnPointTransform = FTransform::Identity;
	AActor* SettingPointActor = WorldSettings->GetRoomStartPoint(GetRoomSpawnSettingName());
	if (SettingPointActor != nullptr)
	{
		SpawnPointTransform = SettingPointActor->GetActorTransform();
	}

	// 모델 팩토리가 모델과 뷰(ATileMap)를 함께 스폰
	mTileMap = GetWorldModelFactory(this)->NewModel<UTileMapModel>(SpawnPointTransform);
	checkf(mTileMap != nullptr, TEXT("타일맵 모델 생성 실패"));

	// 유닛 배치 전에 모델이 타일 저장소를 직접 빌드
	mTileMap->RebuildTiles();
}
