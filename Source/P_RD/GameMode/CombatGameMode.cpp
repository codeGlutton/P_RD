#include "GameMode/CombatGameMode.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Pawn/Unit.h"

#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGSkillBuildAction.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

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
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	for (const TObjectPtr<AUnit>& Unit : CombatSubsystem->GetUnits())
	{
		Unit->OnBeginRoom();
	}
	CombatSubsystem->BeginCombat();
}

bool ACombatGameMode::SelectSkill(int32 SkillIndex)
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	TSharedPtr<FSRPGSkillSelectCommand> SkillSelectCommand = MakeShared<FSRPGSkillSelectCommand>();
	SkillSelectCommand->mSkillIndex = SkillIndex;
	SkillSelectCommand->OnChangeSkillBuildPhase.AddUObject(this, &ACombatGameMode::OnChangeSkillBuildPhase);

	return CombatSubsystem->SummitCommand(SkillSelectCommand) == ESRPGActionCommandResult::Handle;
}

bool ACombatGameMode::ResolveWorldTouchEvent()
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	TSharedPtr<FSRPGWorldTraceCommand> WorldTraceActionCommand = MakeShared<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand->mIsLongPress = false;

	return CombatSubsystem->SummitCommand(WorldTraceActionCommand) == ESRPGActionCommandResult::Handle;
}

bool ACombatGameMode::ResolveWorldLongPressEvent()
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	TSharedPtr<FSRPGWorldTraceCommand> WorldTraceActionCommand = MakeShared<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand->mIsLongPress = true;

	return CombatSubsystem->SummitCommand(WorldTraceActionCommand) == ESRPGActionCommandResult::Handle;
}

void ACombatGameMode::OnChangeSkillBuildPhase(const FSRPGSkillBuildAction& Action, ESRPGSkillBuildPhase Phase)
{

}
