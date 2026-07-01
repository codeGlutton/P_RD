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

/*
 * #216 / #217 경계 메모
 *
 * 이 파일은 전투 HUD가 실제 게임플레이와 연결되기 위해 필요한 "중간 관제탑"이다.
 * 위젯은 이 GameMode의 public 함수만 호출하고, GameMode는 커맨드 라우터/전투 모델/플레이어 모델을 다룬다.
 *
 * - #216: 이 경계를 사용해서 전투 HUD가 실제로 동작하게 만드는 PR.
 * - #217: 이 파일의 커맨드 발행, 이벤트 바인딩, Push*UI 오케스트레이션을 PM이 검토/이관할 수 있게 떼어 둔 참고 PR.
 *
 * 게임플레이 모델/enum -> 표시 DTO 변환 본문은 UI/Combat/CombatUIProjection.{h,cpp}로 분리했다.
 * GameMode에는 "언제 Push할지"만 남기고, "어떻게 DTO로 바꿀지"는 무상태 Projection 함수로 보낸 구조다.
 */

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

	/*
	 * PM 리뷰 포인트:
	 * 여기부터는 전투 모델 이벤트를 UI용 신호로 중계하는 GameMode 배선이다.
	 * HUD가 CombatModel을 직접 구독하지 않게 하려는 구조지만,
	 * 어떤 게임플레이 이벤트에서 어떤 Push*UI를 호출할지는 게임플레이 흐름과 강하게 묶인다.
	 */
	CombatModel->OnBeginCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier) {
		OnBeginCombatUI.Broadcast(Barrier);
		});
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result) {
		OnEndCombatUI.Broadcast(Barrier, Result == ESRPGCombatResult::PlayerWin);
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		// 턴 시작 시에는 PM안대로 전체 스냅샷을 한 번 다시 채운다.
		PushAllCombatUI();
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
		// 턴 시작 주사위 패널은 UI 내부에서 열지만, 열기 직전 전체 스냅샷은 GameMode가 최신화한다.
		PushAllCombatUI();
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

	// HUD가 대리자를 구독한 뒤 첫 스냅샷을 채운다. 순서가 바뀌면 첫 화면이 비어 보일 수 있다.
	PushAllCombatUI();

	// 전투 시작 자체는 게임플레이 라이프사이클이다. #217에서 PM 검토 대상으로 표시한 이유가 이 줄이다.
	CombatModel->BeginCombat();
}

bool ACombatGameMode::SelectSkill(int32 SkillIndex)
{
	/*
	 * UI 입력 가드:
	 * 빈 슬롯/로드 실패 슬롯은 커맨드 라우터까지 보내지 않는다.
	 * 실제 스킬 실행 가능 여부의 최종 판정은 게임플레이 커맨드 쪽에 남고,
	 * 여기서는 UI가 보여주는 슬롯 범위와 usable 상태만 막는다.
	 */
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
		// 게임플레이 phase를 UI enum으로 낮춰서 전달한다. HUD는 ESRPGSkillBuildPhase를 몰라도 된다.
		OnRefreshSkillBuildPhase.Broadcast(CombatUIProjection::ToCombatBuildPhaseUI(Phase));
		});

	const bool bHandled = CommandRouterModel->SummitCommand(SkillSelectCommand);
	if (bHandled == true)
	{
		OnRefreshSelectedSkillUI.Broadcast();
	}
	return bHandled;
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
		// MOVE는 스킬과 별도 행동이다. UI에는 공용 ECombatBuildPhaseUI만 내려서 MOVE/CANCEL 표시를 결정한다.
		OnRefreshMoveBuildPhase.Broadcast(CombatUIProjection::ToCombatBuildPhaseUI(Phase));
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
		OnShowTargetDetailPanelUI.Broadcast(Target);
		});

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

const FSkillEntry* ACombatGameMode::GetSkillDetail(int32 SkillIndex)
{
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (PlayerUnit == nullptr)
	{
		return nullptr;
	}

	USkillComponentModel* SkillComponentModel = PlayerUnit->GetSkillComponentModel();
	if (SkillComponentModel == nullptr)
	{
		return nullptr;
	}
	if (SkillIndex < 0 || SkillIndex >= SkillComponentModel->GetSkillNum())
	{
		return nullptr;
	}

	const FSkillEntry* SkillEntry = SkillComponentModel->GetSkill(SkillIndex);
	return SkillEntry != nullptr && SkillEntry->IsValid() ? SkillEntry : nullptr;
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
	/*
	 * 초기/전체 갱신 스냅샷.
	 * Push*UI는 모델을 읽어 DTO를 저장하고, 개별 갱신 신호가 남아 있는 도메인은 즉시 redraw를 요청한다.
	 * 턴 시작/초기화처럼 넓은 갱신이 필요한 시점은 마지막 OnRefreshAllUI로 한 번 더 묶는다.
	 */
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
		// 유닛 HUD가 다시 그려져야 하는 최소 전투 속성만 구독한다.
		// 상태이상 태그 구독은 후속으로 넣어도 같은 PushUnitUI 경로를 사용하면 된다.
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
		// 굴림 결과와 선택 합계를 함께 갱신해야 패널/보유주사위 표시가 어긋나지 않는다.
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
			// PM API에는 PlayerMeta 전용 refresh가 없으므로, 메타 변화는 넓은 갱신 신호로 소비하게 한다.
			OnRefreshAllUI.Broadcast();
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

	// 실제 변환은 CombatUIProjection이 담당한다. GameMode는 저장과 신호만 담당한다.
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

	// 주사위 원본 모델을 읽어 FDiceSlotUI 배열로 만든 뒤, Dice 영역만 redraw하게 알린다.
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

	// 선택 목록과 합계를 같은 타이밍에 저장한다. 합계만 늦게 읽으면 스킬 빌드 표시가 틀어진다.
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

	// 턴 주인 id 중심의 얇은 DTO다. UI가 TurnContext를 직접 들고 있지 않게 한다.
	CombatUIModel->SetTurnUI(CombatUIProjection::BuildTurnUI(CombatModel));
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

	// SkillComponentModel -> FSkillUI 변환. 빈 슬롯도 unusable DTO로 남겨 UI 인덱스와 실제 스킬 인덱스를 맞춘다.
	CombatUIModel->SetSkillUIs(CombatUIProjection::BuildSkillUIs(SkillComponentModel));
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

	// 장비 DTO는 TopMenuBar/상세 헬퍼가 소비한다. Combat HUD용 장비 refresh 신호는 PM API에서 제외했다.
	CombatUIModel->SetEquipmentUIs(CombatUIProjection::BuildEquipmentUIs(EquipmentComponentModel));
}

void ACombatGameMode::PushPlayerMetaUI()
{
	UCombatUIModel* CombatUIModel = GetCombatUIModel();
	UPlayerUnitModel* PlayerUnit = GetPlayerUnitModel();
	if (CombatUIModel == nullptr || PlayerUnit == nullptr)
	{
		return;
	}

	// Gold/Level/Exp 같은 전투 중 요약값. 별도 PlayerMeta refresh 없이 전체 갱신 시점에 다시 읽는다.
	CombatUIModel->SetPlayerMeta(CombatUIProjection::BuildPlayerMetaUI(PlayerUnit));
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

