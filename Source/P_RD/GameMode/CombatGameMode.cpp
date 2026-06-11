#include "GameMode/CombatGameMode.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Camera/CameraActor.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "Setting/RDWorldSettings.h"

#include "PCGStage/Room.h"

#include "Pawn/Unit.h"

void ACombatGameMode::InitializeRoom()
{
	Super::InitializeRoom();

	const FRoom& CurRoom = GetRunPersistData()->GetCurrentRoom();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	const UStaticCombatRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticCombatRoomSpawnData>(CurRoom.mStaticRoomSpawnDataId);
	checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	CombatSubsystem->InitCombat(StaticRoomData, GetPlayerUnit());
}

void ACombatGameMode::BeginRoom()
{
	Super::BeginRoom();

	ApplyMainCameraPoint();

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	for (const TObjectPtr<AUnit>& Unit : CombatSubsystem->GetUnits())
	{
		Unit->OnBeginPlayRoom();
	}
	CombatSubsystem->BeginCombat();
}

void ACombatGameMode::ApplyMainCameraPoint() const
{
	UWorld* World = GetWorld();
	checkf(World != nullptr, TEXT("월드 nullptr"));

	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(World->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RDWorldSettings nullptr"));

	AActor* MainCameraPoint = WorldSettings->GetMainCameraPoint();
	checkf(MainCameraPoint != nullptr, TEXT("MainCameraPoint nullptr"));

	APlayerController* PlayerController = World->GetFirstPlayerController();
	checkf(PlayerController != nullptr, TEXT("플레이어 컨트롤러 nullptr"));

	const FTransform CameraTransform = MainCameraPoint->GetActorTransform();
	ACameraActor* CombatCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraTransform);
	checkf(CombatCamera != nullptr, TEXT("전투 카메라 nullptr"));

	PlayerController->SetViewTarget(CombatCamera);
	PlayerController->SetControlRotation(CameraTransform.GetRotation().Rotator());
}
