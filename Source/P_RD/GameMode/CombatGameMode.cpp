#include "GameMode/CombatGameMode.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

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

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	for (const TObjectPtr<AUnit>& Unit : CombatSubsystem->GetUnits())
	{
		Unit->OnBeginPlayRoom();
	}
	CombatSubsystem->BeginCombat();
}
