#include "GameMode/CombatGameMode.h"
#include "Simulation/Factory/ObjectModelFactory.h"

#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Pawn/UnitModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/CombatTileMapHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Combat/CombatUIAdapter.h"
#include "Pawn/Player/PlayerUnit.h"
#include "Dice/DicePoolModel.h"

#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGSkillBuildAction.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

void ACombatGameMode::InitializeRoom()
{
	Super::InitializeRoom();

	const FRoom& CurRoom = GetRunPersistData()->GetCurrentRoom();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticCombatRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticCombatRoomSpawnData>(CurRoom.mStaticRoomSpawnDataId);
	checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	CombatModel->InitCombat(StaticRoomData, GetPlayerUnit());
}

void ACombatGameMode::BeginRoom()
{
	Super::BeginRoom();

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 nullptr"));
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 시스템 모델 nullptr"));

	// --- 전투 UI 배선(정석/MVVM): CombatUIModel 1개를 전투 수명에 두고, HUD는 그걸 읽고(bind),
	//     어댑터가 게임플레이를 읽어 Set*로 push + HUD의 Request 입력을 구독한다. ---
	UCombatUIModel* CombatUIModel = CombatSubsystem->GetCombatUIModel();
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UCombatTileMapHUDWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<UCombatTileMapHUDWidget>())
		{
			CombatHUD->BindCombatUIModel(CombatUIModel);   // HUD가 모델을 읽고 구독
			CombatHUD->OpenUI();                            // InitHUD로 생성만 된 HUD를 화면에 올림
		}
	}
	// 임시 비GAS 어댑터: 전투 상태 → 모델 push, 모델의 Request → 게임플레이 처리.
	mCombatUIAdapter = NewObject<UCombatUIAdapter>(this);
	if (UPlayerUnitModel* PlayerUnit = GetPlayerUnit())
	{
		// 런 다이스 목록(mDiceIds)으로 플레이어 주사위 풀을 구성한다. 비면 HUD 주사위 수가 0.
		if (UDicePoolModel* DicePool = PlayerUnit->GetDicePool())
		{
			DicePool->BuildFromDiceIds(GetRunPersistData()->GetDiceIds());
		}
		mCombatUIAdapter->SetDicePool(PlayerUnit->GetDicePool());
	}
	mCombatUIAdapter->BindUIModel(CombatUIModel);
	mCombatUIAdapter->Build(CombatSubsystem, GetRunPersistData());
	mCombatUIAdapter->PushAll();

	CombatModel->BeginCombat();
}

bool ACombatGameMode::SelectSkill(int32 SkillIndex)
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> SkillSelectCommand;
	SkillSelectCommand.InitializeAs<FSRPGSkillSelectCommand>();
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().mSkillIndex = SkillIndex;
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnChangeSkillBuildPhase.AddUObject(this, &ACombatGameMode::OnChangeSkillBuildPhase);

	return CommandRouterModel->SummitCommand(SkillSelectCommand);
}

bool ACombatGameMode::ResolveWorldTouchEvent()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> WorldTraceActionCommand;
	WorldTraceActionCommand.InitializeAs<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().mIsLongPress = false;

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

bool ACombatGameMode::ResolveWorldLongPressEvent()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> WorldTraceActionCommand;
	WorldTraceActionCommand.InitializeAs<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().mIsLongPress = true;

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

void ACombatGameMode::OnChangeSkillBuildPhase(const USRPGSkillBuildAction* Action, ESRPGSkillBuildPhase Phase)
{
}
