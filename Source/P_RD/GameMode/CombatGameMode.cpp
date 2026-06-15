#include "GameMode/CombatGameMode.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Pawn/Unit.h"
#include "UI/RDUserWidget.h"

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

	/*
	 * TopMenuBar는 RoomGameModeBase가 공용 월드 위젯으로 연다.
	 * 전투 타일맵 HUD는 전투방에만 필요한 화면이므로 CombatGameMode의 HUDClass로 분리해서 여기서 연다.
	 * 이렇게 해야 MAP/SET/DICE/SKILL 탑바 흐름과 전투 전용 액션 패널이 같은 WorldWidget 슬롯을 덮어쓰지 않는다.
	 */
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (URDUserWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<URDUserWidget>())
		{
			CombatHUD->OpenUI();
		}
	}

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	for (const TObjectPtr<AUnit>& Unit : CombatSubsystem->GetUnits())
	{
		Unit->OnBeginPlayRoom();
	}
	CombatSubsystem->BeginCombat();
}
