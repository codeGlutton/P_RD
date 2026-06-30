#include "GameMode/CombatGameMode.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Simulation/Factory/ObjectModelFactory.h"
#include "Pawn/UnitModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "UI/CombatTileMapHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/CombatUIProjection.h"

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGDiceRollAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "Dice/DiceModel.h"
#include "Dice/DicePoolModel.h"

#include "AttributeSet/UnitAttributeSet.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

// 게임플레이 모델/enum -> 표시 DTO 변환 헬퍼는 UI/Combat/CombatUIProjection.{h,cpp}로 분리됨.

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
		OnBeginCombatUI.Broadcast(Barrier);
		});
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result) {
		OnEndCombatUI.Broadcast(Barrier, Result == ESRPGCombatResult::PlayerWin);
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		PushTurnUI();
		OnBeginAnyTurnUI.Broadcast(Barrier);
		});
	CombatModel->OnEndAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		OnEndAnyTurnUI.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		OnBeginAnyTurnActionUI.Broadcast(Barrier);
		});
	CombatModel->OnEndAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result) {
		OnEndAnyTurnActionUI.Broadcast(Barrier);
		});
	CombatModel->OnRegisterUnitUI.AddUObject(this, &ACombatGameMode::OnRegisterUnit);
	CombatModel->OnUnregisterUnitUI.AddUObject(this, &ACombatGameMode::OnUnregisterUnit);
	CombatModel->OnShowDicePanelAnyTurnUI.AddWeakLambda(this, [this](const USRPGTurnContext* TurnContext) {
		PushDiceUI();
		PushSelectedDiceUI();
		OnShowDicePanelAnyTurnUI.Broadcast();
		});

	for (TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
	{
		OnRegisterUnit(Unit);
	}
	BindPlayerDicePoolUIEvents();
	BindPlayerMetaUIEvents();
}

void ACombatGameMode::BeginRoom()
{
	Super::BeginRoom();

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 nullptr"));
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 시스템 모델 nullptr"));

	// 전투 HUD 표시 수명만 연다. HUD는 UCombatUIModel을 읽기 전용으로 구독한다.
	// (입력 의도/갱신 push 경로는 Phase 2에서 UCombatUIModel 단일 경계로 복원 예정)
	UCombatUIModel* CombatUIModel = CombatSubsystem->GetCombatUIModel();
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UCombatTileMapHUDWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<UCombatTileMapHUDWidget>())
		{
			CombatHUD->BindCombatUIModel(CombatUIModel);   // HUD가 모델을 읽고 구독
			CombatHUD->OpenUI();                            // InitHUD로 생성만 된 HUD를 화면에 올림
		}
	}

	PushAllCombatUI();
	CombatModel->BeginCombat();
}

bool ACombatGameMode::SelectSkill(int32 SkillIndex)
{
	if (UCombatUIModel* CombatUIModel = GetCombatUIModel())
	{
		const TArray<FSkillUI>& SkillUIs = CombatUIModel->GetSkillUIs();
		if (SkillUIs.IsValidIndex(SkillIndex) == false || SkillUIs[SkillIndex].mIsUsable == false)
		{
			return false;
		}
	}

	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> SkillSelectCommand;
	SkillSelectCommand.InitializeAs<FSRPGSkillSelectCommand>();
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().mSkillIndex = SkillIndex;
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnChangeSkillBuildPhase.AddWeakLambda(this, [this](const USRPGSkillBuildAction* Action, ESRPGSkillBuildPhase Phase) {
		OnRefreshSkillBuildPhase.Broadcast(CombatUIProjection::ToCombatBuildPhaseUI(Phase));
		PushSelectedDiceUI();
		PushDiceUI();
		PushTurnUI();
		if (Phase == ESRPGSkillBuildPhase::Build || Phase == ESRPGSkillBuildPhase::None)
		{
			OnCombatActionResolvedUI.Broadcast();
		}
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
		OnRefreshMoveBuildPhase.Broadcast(CombatUIProjection::ToCombatBuildPhaseUI(Phase));
		PushTurnUI();
		if (Phase == ESRPGMoveBuildPhase::Build || Phase == ESRPGMoveBuildPhase::None)
		{
			OnCombatActionResolvedUI.Broadcast();
		}
		});

	return CommandRouterModel->SummitCommand(MoveSelectCommand);
}

bool ACombatGameMode::EndTurn()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> DiceSelectCommand;
	DiceSelectCommand.InitializeAs<FSRPGTurnEndCommand>();

	const bool bHandled = CommandRouterModel->SummitCommand(DiceSelectCommand);
	UE_LOG(LogCombatGameMode, Log, TEXT("CombatGameMode: EndTurn command submitted. Handled=%s"), bHandled ? TEXT("true") : TEXT("false"));
	return bHandled;
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
		OnShowTargetDetailPanelUI.Broadcast();
		});

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

bool ACombatGameMode::ShowSkillDetail(int32 SkillIndex)
{
	if (UCombatUIModel* CombatUIModel = GetCombatUIModel())
	{
		const TArray<FSkillUI>& SkillUIs = CombatUIModel->GetSkillUIs();
		if (SkillUIs.IsValidIndex(SkillIndex) == false || SkillUIs[SkillIndex].mIsUsable == false)
		{
			return false;
		}
	}

	OnShowSkillDetailPanelUI.Broadcast(SkillIndex);
	return true;
}

bool ACombatGameMode::ShowEquipmentDetail(int32 SlotIndex)
{
	OnShowEquipmentDetailPanelUI.Broadcast(SlotIndex);
	return GetEquipmentDetail(static_cast<EEquipmentType>(SlotIndex)) != nullptr;
}

const FEquippedEntry* ACombatGameMode::GetEquipmentDetail(EEquipmentType EquipmentType)
{
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (PlayerUnit == nullptr)
	{
		return nullptr;
	}

	UEquipmentComponentModel* EquipmentComponentModel = PlayerUnit->GetEquipmentComponentModel();
	if (EquipmentComponentModel == nullptr)
	{
		return nullptr;
	}

	return EquipmentComponentModel->GetEquipped(EquipmentType);
}

void ACombatGameMode::PushAllCombatUI()
{
	PushUnitUI();
	PushDiceUI();
	PushSelectedDiceUI();
	PushTurnUI();
	PushSkillUI();
	PushEquipmentUI();
	PushPlayerMetaUI();

	OnRefreshAllUI.Broadcast();
}

void ACombatGameMode::OnRegisterUnit(UUnitModel* Unit)
{
	if (Unit == nullptr)
	{
		return;
	}

	UAttributeSetComponentModel* AttributeComponentModel = Unit->GetAttributeComponentModel();
	if (AttributeComponentModel != nullptr)
	{
		const TArray<FTacticalAttribute> AttributesToRefresh = {
			UUnitAttributeSet::GetHPAttribute(),
			UUnitAttributeSet::GetMaxHPAttribute(),
			UUnitAttributeSet::GetAttackPointAttribute(),
			UUnitAttributeSet::GetDefensePointAttribute(),
			UUnitAttributeSet::GetMovementPointAttribute()
		};

		for (const FTacticalAttribute& Attribute : AttributesToRefresh)
		{
			AttributeComponentModel->GetTacticalAttributeValueChangeDelegate(Attribute).RemoveAll(this);
			AttributeComponentModel->GetTacticalAttributeValueChangeDelegate(Attribute).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& ChangeData) {
				PushUnitUI();
				});
		}
	}

	PushUnitUI();
}

void ACombatGameMode::OnUnregisterUnit(UUnitModel* Unit)
{
	if (Unit == nullptr)
	{
		return;
	}

	UAttributeSetComponentModel* AttributeComponentModel = Unit->GetAttributeComponentModel();
	if (AttributeComponentModel != nullptr)
	{
		const TArray<FTacticalAttribute> AttributesToRefresh = {
			UUnitAttributeSet::GetHPAttribute(),
			UUnitAttributeSet::GetMaxHPAttribute(),
			UUnitAttributeSet::GetAttackPointAttribute(),
			UUnitAttributeSet::GetDefensePointAttribute(),
			UUnitAttributeSet::GetMovementPointAttribute()
		};

		for (const FTacticalAttribute& Attribute : AttributesToRefresh)
		{
			AttributeComponentModel->GetTacticalAttributeValueChangeDelegate(Attribute).RemoveAll(this);
		}
	}

	PushUnitUI();
}

void ACombatGameMode::BindPlayerDicePoolUIEvents()
{
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (PlayerUnit == nullptr)
	{
		return;
	}

	UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
	if (DicePoolModel == nullptr)
	{
		return;
	}

	DicePoolModel->OnRollAllDicesUI.RemoveAll(this);
	DicePoolModel->OnRollAllDicesUI.AddWeakLambda(this, [this](const TArray<TObjectPtr<UDiceModel>>& Dices) {
		PushDiceUI();
		PushSelectedDiceUI();
		});

	DicePoolModel->OnUseDiceUI.RemoveAll(this);
	DicePoolModel->OnUseDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushDiceUI();
		PushSelectedDiceUI();
		});

	DicePoolModel->OnResetAllDiceUI.RemoveAll(this);
	DicePoolModel->OnResetAllDiceUI.AddWeakLambda(this, [this](const TArray<TObjectPtr<UDiceModel>>& Dices) {
		PushDiceUI();
		PushSelectedDiceUI();
		});

	DicePoolModel->OnSelectedDiceUI.RemoveAll(this);
	DicePoolModel->OnSelectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushSelectedDiceUI();
		});

	DicePoolModel->OnUnselectedDiceUI.RemoveAll(this);
	DicePoolModel->OnUnselectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushSelectedDiceUI();
		});
}

void ACombatGameMode::BindPlayerMetaUIEvents()
{
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (PlayerUnit == nullptr)
	{
		return;
	}

	UAttributeSetComponentModel* AttributeComponentModel = PlayerUnit->GetAttributeComponentModel();
	if (AttributeComponentModel == nullptr)
	{
		return;
	}

	const TArray<FTacticalAttribute> AttributesToRefresh = {
		UPlayerUnitAttributeSet::GetMoneyAttribute(),
		UPlayerUnitAttributeSet::GetExpAttribute(),
		UPlayerUnitAttributeSet::GetMaxExpAttribute()
	};

	for (const FTacticalAttribute& Attribute : AttributesToRefresh)
	{
		AttributeComponentModel->GetTacticalAttributeValueChangeDelegate(Attribute).RemoveAll(this);
		AttributeComponentModel->GetTacticalAttributeValueChangeDelegate(Attribute).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& ChangeData) {
			PushPlayerMetaUI();
			});
	}
}

void ACombatGameMode::PushUnitUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatUIModel == nullptr || CombatModel == nullptr)
	{
		return;
	}

	CombatUIModel->SetUnitUIs(CombatUIProjection::BuildUnitUIs(CombatModel));
	OnRefreshUnitUI.Broadcast();
}

void ACombatGameMode::PushDiceUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (CombatUIModel == nullptr || PlayerUnit == nullptr)
	{
		return;
	}

	UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
	if (DicePoolModel == nullptr)
	{
		return;
	}

	CombatUIModel->SetDiceUIs(CombatUIProjection::BuildDiceUIs(DicePoolModel));
	OnRefreshDiceUI.Broadcast();
}

void ACombatGameMode::PushSelectedDiceUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (CombatUIModel == nullptr || PlayerUnit == nullptr)
	{
		return;
	}

	UDicePoolModel* DicePoolModel = PlayerUnit->GetDicePoolModel();
	if (DicePoolModel == nullptr)
	{
		return;
	}

	CombatUIModel->SetSelectedDice(CombatUIProjection::BuildSelectedDiceIndices(DicePoolModel), DicePoolModel->GetSelectedDiceSum());
	OnRefreshSelectedDiceUI.Broadcast();
}

void ACombatGameMode::PushTurnUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatUIModel == nullptr || CombatModel == nullptr)
	{
		return;
	}

	CombatUIModel->SetTurnUI(CombatUIProjection::BuildTurnUI(CombatModel));
	OnRefreshTurnUI.Broadcast();
}

void ACombatGameMode::PushSkillUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (CombatUIModel == nullptr || PlayerUnit == nullptr)
	{
		return;
	}

	USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
	if (SkillComponentModel == nullptr)
	{
		UE_LOG(LogCombatGameMode, Warning, TEXT("CombatGameMode: PushSkillUI skipped because SkillComponentModel is null."));
		return;
	}

	CombatUIModel->SetSkillUIs(CombatUIProjection::BuildSkillUIs(SkillComponentModel));
	OnRefreshSkillUI.Broadcast();
}

void ACombatGameMode::PushEquipmentUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (CombatUIModel == nullptr || PlayerUnit == nullptr)
	{
		return;
	}

	UEquipmentComponentModel* EquipmentComponentModel = PlayerUnit->GetEquipmentComponentModel();
	if (EquipmentComponentModel == nullptr)
	{
		return;
	}

	CombatUIModel->SetEquipmentUIs(CombatUIProjection::BuildEquipmentUIs(EquipmentComponentModel));
	OnRefreshEquipmentUI.Broadcast();
}

void ACombatGameMode::PushPlayerMetaUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (CombatUIModel == nullptr || PlayerUnit == nullptr)
	{
		return;
	}

	CombatUIModel->SetPlayerMeta(CombatUIProjection::BuildPlayerMetaUI(PlayerUnit));
	OnRefreshPlayerMetaUI.Broadcast();
}

bool ACombatGameMode::GetEquipmentUIs(TArray<FEquipmentUI>& OutEquipmentUIs) const
{
	OutEquipmentUIs.Reset();

	const UCombatUIModel* CombatUIModel = GetCombatUIModel();
	if (CombatUIModel == nullptr)
	{
		return false;
	}

	OutEquipmentUIs = CombatUIModel->GetEquipmentUIs();
	return true;
}

UCombatUIModel* ACombatGameMode::GetCombatUIModel() const
{
	if (UWorld* World = GetWorld())
	{
		if (USRPGCombatSubsystem* CombatSubsystem = World->GetSubsystem<USRPGCombatSubsystem>())
		{
			return CombatSubsystem->GetCombatUIModel();
		}
	}

	return nullptr;
}

