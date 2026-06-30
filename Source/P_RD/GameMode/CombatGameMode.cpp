#include "GameMode/CombatGameMode.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Simulation/Factory/ObjectModelFactory.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "UI/CombatTileMapHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Combat/CombatUIAdapter.h"

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGDiceRollAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Dice/DicePoolModel.h"

#include "AttributeSet/UnitAttributeSet.h"

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
	CombatModel->InitCombat(StaticRoomData, GetPlayerUnitModel());

	/* 전투 모델 대리자 연결 */

	CombatModel->OnBeginCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier) {
		OnRefreshAllUI.Broadcast();
		OnBeginCombatUI.Broadcast(Barrier);
		});
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result) {
		OnEndCombatUI.Broadcast(Barrier, Result);
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		OnBeginAnyTurnUI.Broadcast(Barrier, TurnContext);
		});
	CombatModel->OnEndAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		OnEndAnyTurnUI.Broadcast(Barrier, TurnContext, Result);
		});
	CombatModel->OnBeginAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		OnBeginAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action);
		});
	CombatModel->OnEndAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result) {
		OnEndAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action, Result);
		});

	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	/* 주사위 대리자 연결 */

	UDicePoolModel* DicePoolModel = PlayerUnitModel->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 nullptr"));

	DicePoolModel->OnRollAllDicesUI.AddWeakLambda(this, [this](const TArray<TObjectPtr<UDiceModel>>& Dices) {
		OnRefreshDiceUI.Broadcast();
		});
	DicePoolModel->OnUseDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		OnRefreshDiceUI.Broadcast();
		});
	DicePoolModel->OnResetAllDiceUI.AddWeakLambda(this, [this](const TArray<TObjectPtr<UDiceModel>>& Dices) {
		OnRefreshDiceUI.Broadcast();
		});

	DicePoolModel->OnSelectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		OnRefreshSelectedDiceUI.Broadcast();
		});
	DicePoolModel->OnUnselectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		OnRefreshSelectedDiceUI.Broadcast();
		});
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
	if (UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel())
	{
		mCombatUIAdapter->SetDicePool(PlayerUnit->GetDicePoolModel());
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
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnChangeSkillBuildPhase.AddWeakLambda(this, [this](const USRPGSkillBuildAction* Action, ESRPGSkillBuildPhase Phase) {
		OnRefreshSkillBuildPhase.Broadcast(Phase);
		});

	return CommandRouterModel->SummitCommand(SkillSelectCommand);
}

bool ACombatGameMode::SelectDice(int32 DiceIndex)
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> DiceSelectCommand;
	DiceSelectCommand.InitializeAs<FSRPGDiceSelectCommand>();
	DiceSelectCommand.GetMutable<FSRPGDiceSelectCommand>().mDiceIndex = DiceIndex;

	return CommandRouterModel->SummitCommand(DiceSelectCommand);
}

bool ACombatGameMode::RollDices()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> DiceSelectCommand;
	DiceSelectCommand.InitializeAs<FSRPGDiceRollCommand>();

	return CommandRouterModel->SummitCommand(DiceSelectCommand);
}

bool ACombatGameMode::SelectMove()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> MoveSelectCommand;
	MoveSelectCommand.InitializeAs<FSRPGMoveSelectCommand>();
	MoveSelectCommand.GetMutable<FSRPGMoveSelectCommand>().OnChangeMoveBuildPhase.AddWeakLambda(this, [this](const USRPGMoveBuildAction* Action, ESRPGMoveBuildPhase Phase) {
		OnRefreshMoveBuildPhase.Broadcast(Phase);
		});

	return CommandRouterModel->SummitCommand(MoveSelectCommand);
}

bool ACombatGameMode::EndTurn()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> DiceSelectCommand;
	DiceSelectCommand.InitializeAs<FSRPGTurnEndCommand>();

	return CommandRouterModel->SummitCommand(DiceSelectCommand);
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
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().OnShowTargetDetailPanelUI.AddWeakLambda(this, [this](IBoardSelectionTarget* Target) {
		OnShowTargetDetailPanelUI.Broadcast(Target);
		});

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

FEquippedEntry* ACombatGameMode::GetEquipmentDetail(int32 EquipmentIndex)
{
	return nullptr;
}

