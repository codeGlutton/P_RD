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

namespace
{
	FLinearColor GetCombatRarityColor(ERarityType RarityType)
	{
		switch (RarityType)
		{
		case ERarityType::Rare:
			return FLinearColor(0.55f, 0.72f, 1.0f, 1.0f);
		case ERarityType::Epic:
			return FLinearColor(0.82f, 0.58f, 1.0f, 1.0f);
		case ERarityType::Common:
		default:
			return FLinearColor(0.86f, 0.98f, 0.94f, 1.0f);
		}
	}

	ECombatSkillSelectShapeUI ToCombatSkillSelectShape(EAimPattern Pattern)
	{
		switch (Pattern)
		{
		case EAimPattern::Single:
			return ECombatSkillSelectShapeUI::Single;
		case EAimPattern::Cross:
			return ECombatSkillSelectShapeUI::Cross;
		case EAimPattern::Star:
			return ECombatSkillSelectShapeUI::Diagonal;
		case EAimPattern::Square:
			return ECombatSkillSelectShapeUI::Square;
		default:
			return ECombatSkillSelectShapeUI::None;
		}
	}

	ECombatSkillHitShapeUI ToCombatSkillHitShape(EEffectPattern Pattern)
	{
		switch (Pattern)
		{
		case EEffectPattern::Single:
			return ECombatSkillHitShapeUI::Single;
		case EEffectPattern::Cross:
		case EEffectPattern::Star:
			return ECombatSkillHitShapeUI::Cross;
		case EEffectPattern::Square:
			return ECombatSkillHitShapeUI::Circle;
		case EEffectPattern::Beam:
			return ECombatSkillHitShapeUI::Single;
		default:
			return ECombatSkillHitShapeUI::None;
		}
	}

	ECombatBuildPhaseUI ToCombatBuildPhaseUI(ESRPGSkillBuildPhase Phase)
	{
		switch (Phase)
		{
		case ESRPGSkillBuildPhase::AimSelection:
			return ECombatBuildPhaseUI::AimSelection;
		case ESRPGSkillBuildPhase::Preview:
			return ECombatBuildPhaseUI::Preview;
		case ESRPGSkillBuildPhase::Build:
		case ESRPGSkillBuildPhase::None:
		default:
			return ECombatBuildPhaseUI::None;
		}
	}

	ECombatBuildPhaseUI ToCombatBuildPhaseUI(ESRPGMoveBuildPhase Phase)
	{
		switch (Phase)
		{
		case ESRPGMoveBuildPhase::DestSelection:
			return ECombatBuildPhaseUI::AimSelection;
		case ESRPGMoveBuildPhase::Preview:
			return ECombatBuildPhaseUI::Preview;
		case ESRPGMoveBuildPhase::Build:
		case ESRPGMoveBuildPhase::None:
		default:
			return ECombatBuildPhaseUI::None;
		}
	}

	FText GetEquipmentSlotFallbackName(EEquipmentType Slot)
	{
		switch (Slot)
		{
		case EEquipmentType::Weapon:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotWeapon", "WEAPON");
		case EEquipmentType::Gloves:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotGloves", "GLOVES");
		case EEquipmentType::Boots:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotBoots", "BOOTS");
		default:
			return NSLOCTEXT("CombatGameMode", "EquipmentSlotEmpty", "EMPTY");
		}
	}
}

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
		OnRefreshSkillBuildPhase.Broadcast(ToCombatBuildPhaseUI(Phase));
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
		OnRefreshMoveBuildPhase.Broadcast(ToCombatBuildPhaseUI(Phase));
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

	TArray<FUnitUI> UnitUIs;
	for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
	{
		if (Unit == nullptr)
		{
			continue;
		}

		FUnitUI UnitUI;
		UnitUI.mUnitId = Unit->GetModelId();
		UnitUI.mIsPlayer = Unit->IsPlayerUnitModel();
		UnitUI.mTile = Unit->GetTileTransform().mIndex;

		if (AActor* UnitActor = Unit->GetView<AActor>())
		{
			UnitUI.mWorldLocation = UnitActor->GetActorLocation();
		}

		if (UAttributeSetComponentModel* AttributeComponentModel = Unit->GetAttributeComponentModel())
		{
			UnitUI.mHP = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute());
			UnitUI.mMaxHP = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute());
			// [216 통합] 215가 DamagePoint 속성을 제거 → mDamagePoint는 후속 매핑 전까지 기본값(0). TODO: AttackPoint/AttackFactor 중 정책 확정 후 연결.
			UnitUI.mDefensePoint = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefensePointAttribute());
			UnitUI.mMovementPoint = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMovementPointAttribute());
			UnitUI.mMaxMovementPoint = UnitUI.mMovementPoint;
			UnitUI.mSkillPoint = AttributeComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetAttackPointAttribute());
			UnitUI.mStatusTags = AttributeComponentModel->GetOwnedGameplayTags();
		}

		UnitUIs.Add(UnitUI);
	}

	CombatUIModel->SetUnitUIs(UnitUIs);
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

	TArray<FDiceSlotUI> DiceUIs;
	const TArray<TObjectPtr<UDiceModel>>& Dices = DicePoolModel->GetDices();
	DiceUIs.Reserve(Dices.Num());

	for (int32 DiceIndex = 0; DiceIndex < Dices.Num(); ++DiceIndex)
	{
		const UDiceModel* Dice = Dices[DiceIndex];
		if (Dice == nullptr)
		{
			continue;
		}

		FDiceSlotUI DiceUI;
		DiceUI.mDiceId = Dice->GetSourceDiceId();
		DiceUI.mResultValue = Dice->IsRolled() ? Dice->GetCurrentValue() : 0;
		DiceUI.mRolledFaceIndex = Dice->GetRolledFaceIndex();
		DiceUI.mIsRolled = Dice->IsRolled();
		DiceUI.mIsSelected = DicePoolModel->IsSelectedDice(DiceIndex);
		DiceUI.mIsUsed = Dice->IsUsed();
		DiceUI.mFaceCount = Dice->GetFaceCount();
		DiceUI.mFaceValues = Dice->GetFaceValues();
		DiceUI.mFaceTextures = Dice->GetFaceTextures();

		DiceUIs.Add(DiceUI);
	}

	CombatUIModel->SetDiceUIs(DiceUIs);
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

	TArray<int32> SelectedDiceIndices;
	const TArray<TObjectPtr<UDiceModel>>& Dices = DicePoolModel->GetDices();
	for (int32 DiceIndex = 0; DiceIndex < Dices.Num(); ++DiceIndex)
	{
		if (DicePoolModel->IsSelectedDice(DiceIndex))
		{
			SelectedDiceIndices.Add(DiceIndex);
		}
	}

	CombatUIModel->SetSelectedDice(SelectedDiceIndices, DicePoolModel->GetSelectedDiceSum());
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

	FTurnUI TurnUI;
	if (USRPGTurnContext* CurrentTurnContext = CombatModel->GetCurrentTurnContext())
	{
		if (UUnitModel* TurnOwner = CurrentTurnContext->GetOwner())
		{
			TurnUI.mCurrentUnitId = TurnOwner->GetModelId();
		}
	}

	CombatUIModel->SetTurnUI(TurnUI);
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

	TArray<FSkillUI> SkillUIs;
	const int32 SkillCount = SkillComponentModel->GetSkillNum();
	SkillUIs.Reserve(SkillCount);

	int32 LoadedSkillCount = 0;
	for (int32 SkillIndex = 0; SkillIndex < SkillCount; ++SkillIndex)
	{
		FSkillUI SkillUI;
		SkillUI.mSkillIndex = SkillIndex;
		SkillUI.mIsUsable = false;

		// [216 통합] 215의 새 스킬 API. FSkillEntry.mData는 이미 로드된 UStaticSkillData.
		FSkillEntry* SkillEntry = SkillComponentModel->GetSkill(SkillIndex);
		UStaticSkillData* StaticSkillData = (SkillEntry != nullptr) ? SkillEntry->mData : nullptr;
		if (StaticSkillData == nullptr)
		{
			UE_LOG(LogCombatGameMode, Warning, TEXT("CombatGameMode: PushSkillUI skill slot %d has no StaticSkillData."), SkillIndex);
			SkillUIs.Add(SkillUI);
			continue;
		}

		++LoadedSkillCount;
		SkillUI.mName = StaticSkillData->mName;
		SkillUI.mDescription = StaticSkillData->mDescription;
		SkillUI.mIcon = StaticSkillData->mIcon.LoadSynchronous();
		SkillUI.mDiceCost = StaticSkillData->mRequiredDiceCount;
		SkillUI.mIsUsable = true;
		SkillUI.mTargeting.mSelectShape = ToCombatSkillSelectShape(StaticSkillData->mAimPattern);
		SkillUI.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRangeDefaultValue);
		SkillUI.mTargeting.mSelectRangeRatio = StaticSkillData->mAimRangeRatio;
		SkillUI.mTargeting.mHitShape = ToCombatSkillHitShape(StaticSkillData->mEffectPattern);
		SkillUI.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectAreaDefaultValue);
		SkillUI.mTargeting.mHitRangeRatio = StaticSkillData->mEffectAreaRatio;
		SkillUI.mTargeting.mIsIndirect = StaticSkillData->mIsIndirect;
		SkillUI.mTargeting.mIsPenetration = StaticSkillData->mIsPenetration;
		SkillUIs.Add(SkillUI);
	}

	CombatUIModel->SetSkillUIs(SkillUIs);
	UE_LOG(LogCombatGameMode, Log, TEXT("CombatGameMode: PushSkillUI pushed %d/%d usable skills."), LoadedSkillCount, SkillCount);
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

	TArray<FEquipmentUI> EquipmentUIs;
	const int32 SlotCount = StaticCast<int32>(EEquipmentType::Count);
	EquipmentUIs.Reserve(SlotCount);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const EEquipmentType Slot = StaticCast<EEquipmentType>(SlotIndex);
		const FEquippedEntry* EquippedEntry = EquipmentComponentModel->GetEquipped(Slot);
		const UStaticEquipmentData* StaticEquipmentData = EquippedEntry != nullptr ? EquippedEntry->mData.Get() : nullptr;

		FEquipmentUI EquipmentUI;
		EquipmentUI.mSlotIndex = SlotIndex;
		EquipmentUI.mName = GetEquipmentSlotFallbackName(Slot);

		if (StaticEquipmentData != nullptr)
		{
			EquipmentUI.mItemId = StaticEquipmentData->GetPrimaryAssetId();
			EquipmentUI.mName = StaticEquipmentData->mName.IsEmpty() ? EquipmentUI.mName : StaticEquipmentData->mName;
			EquipmentUI.mIcon = StaticEquipmentData->mIcon.LoadSynchronous();
			EquipmentUI.mIsEquipped = true;
			EquipmentUI.mRarityColor = GetCombatRarityColor(StaticEquipmentData->mRarityType);
		}

		EquipmentUIs.Add(EquipmentUI);
	}

	CombatUIModel->SetEquipmentUIs(EquipmentUIs);
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

	FPlayerMetaUI MetaUI;
	MetaUI.mLevel = PlayerUnit->GetPlayerLevel();

	if (UAttributeSetComponentModel* AttributeComponentModel = PlayerUnit->GetAttributeComponentModel())
	{
		MetaUI.mGold = FMath::RoundToInt(AttributeComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMoneyAttribute()));
		MetaUI.mExp = AttributeComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
		MetaUI.mMaxExp = AttributeComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxExpAttribute());
	}

	CombatUIModel->SetPlayerMeta(MetaUI);
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

