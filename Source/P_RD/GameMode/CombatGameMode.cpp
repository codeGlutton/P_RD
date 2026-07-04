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

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGDiceRollAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Dice/DicePoolModel.h"
#include "Dice/DiceModel.h"

#include "AttributeSet/UnitAttributeSet.h"

#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/SkillData/StaticSkillData.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

namespace
{
	FLinearColor GetRarityColor(ERarityType RarityType)
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

	ECombatSkillSelectShapeUI GetCombatSkillSelectShape(EAimPattern Pattern)
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

	ECombatSkillHitShapeUI GetCombatSkillHitShape(EEffectPattern Pattern)
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

ACombatGameMode::ACombatGameMode()
{
	mCombatUIModel = CreateDefaultSubobject<UCombatUIModel>(TEXT("CombatUIModel"));
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
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	/*
	 * === UI <-> 게임플레이 경계 배선 ===
	 *
	 * 스킬 버튼/터치를 실제 게임 로직에 잇는 곳이다. 원래는 "보내는 쪽만 있고 받는 쪽이 없거나,
	 * 받는 쪽만 있고 보내는 쪽이 없던" 반쪽 지점이 있었고, 그 끊긴 선을 여기서 잇는다.
	 * 방향은 두 갈래다:
	 *   1) 게임 -> UI (push): 모델 이벤트를 구독해 스냅샷을 UI로 밀어넣는다(레일/주사위/턴 등).
	 *   2) UI -> 게임 (route): 위젯 탭이 쏘는 Request*를 게임플레이 진입점으로 넘긴다.
	 *
	 * 이번에 새로 이은 4가닥(없으면 무슨 일이 안 됐는지 함께 표기):
	 *   - OnEndAnyTurnActionUI  -> NotifyActionResolved   : 시전 끝나도 선택 강조가 안 풀리던 것
	 *   - OnChangeSkillUI       -> PushSkillUIData         : 전투 중 스킬 교체가 레일에 안 보이던 것
	 *   - OnCombatWorldTouch    -> HandleCombatWorldTouch  : 타일 탭이 게임에 도달 못 해 조준이 안 되던 것
	 *   - LongPressSkill        -> PushSkillDetailUIData   : 스킬 상세가 "연결 대기중"만 뜨던 것(HandleCombatCommand)
	 */

	/* 전투 모델 대리자 연결 */

	CombatModel->OnRegisterUnitUI.AddUObject(this, &ACombatGameMode::OnRegisterUnit);
	CombatModel->OnUnregisterUnitUI.AddUObject(this, &ACombatGameMode::OnUnregisterUnit);

	CombatModel->OnShowDicePanelAnyTurnUI.AddWeakLambda(this, [this](const USRPGTurnContext* TurnContext) {
		// mCombatUIModel->NotifyDiceRollPresentationRequested(TurnContext);
		});

	CombatModel->OnBeginCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier) {
		PushPlayerMetaUIData();
		// OnBeginCombatUI.Broadcast(Barrier); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result) {
		PushPlayerMetaUIData();
		// OnEndCombatUI.Broadcast(Barrier, Result); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		PushTurnUIData();
		PushUnitUIData();
		PushDiceUIData();
		PushSelectedDiceUIData();
		PushSkillUIData();
		PushEquipmentUIData();
		// OnBeginAnyTurnUI.Broadcast(Barrier, TurnContext); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnEndAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		// OnEndAnyTurnUI.Broadcast(Barrier, TurnContext, Result); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnBeginAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		// OnBeginAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnEndAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result) {
		// 액션(스킬/이동 빌드 등) 종료 시 UI 선택 강조를 해제한다 — HUD 수신자(HandleCombatActionResolved)는
		// 이미 대기 중이었고 발신자만 없었다(과거 어댑터 잔재).
		mCombatUIModel->NotifyActionResolved();
		// OnEndAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action, Result); 연출은 연결고리가 아직 없음
		});

	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	/* 주사위 대리자 연결 */

	UDicePoolModel* DicePoolModel = PlayerUnitModel->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 nullptr"));

	DicePoolModel->OnRollAllDicesUI.AddWeakLambda(this, [this](const TArray<TObjectPtr<UDiceModel>>& Dices) {
		PushDiceUIData();
		});
	DicePoolModel->OnUseDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushDiceUIData();
		});
	DicePoolModel->OnResetAllDiceUI.AddWeakLambda(this, [this](const TArray<TObjectPtr<UDiceModel>>& Dices) {
		PushDiceUIData();
		});

	DicePoolModel->OnSelectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushDiceUIData();   // 슬롯별 mIsSelected 강조도 함께 갱신(선택 토글은 이 이벤트로만 온다)
		PushSelectedDiceUIData();
		});
	DicePoolModel->OnUnselectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushDiceUIData();
		PushSelectedDiceUIData();
		});

	/* 스킬 대리자 연결 */

	USkillComponentModel* SkillComponentModel = PlayerUnitModel->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("스킬 컴포넌트 nullptr"));

	SkillComponentModel->OnChangeSkillUI.AddWeakLambda(this, [this](int32 SkillIndex, const UStaticSkillData* PreSkillData, const UStaticSkillData* NewSkillData) {
		PushSkillUIData();   // 전투 중 스킬 교체가 다음 턴 시작까지 레일에 안 보이던 공백을 메운다
		});

	/* UI 조작 의도 라우팅 — 위젯 탭이 쏘는 Request*(OnCombatCommand)를 게임플레이 진입점에 연결 */

	mCombatUIModel->OnCombatCommand.AddUniqueDynamic(this, &ACombatGameMode::HandleCombatCommand);
	mCombatUIModel->OnApplyDiceResults.AddUniqueDynamic(this, &ACombatGameMode::HandleApplyDiceResults);
	// 월드 터치(조준 타일 선택/한 단계 취소)가 이 구독 없이는 게임플레이에 도달하지 못했다 —
	// 스킬 조준->프리뷰->시전 확정 입력 체인의 마지막 공백.
	mCombatUIModel->OnCombatWorldTouch.AddUniqueDynamic(this, &ACombatGameMode::HandleCombatWorldTouch);

	CombatModel->InitCombat(StaticRoomData, GetPlayerUnitModel());
}

/**
 * @brief UIModel의 조작 의도(OnCombatCommand)를 게임플레이 진입점으로 라우팅한다.
 *
 * @details
 * UI 경계 원칙: 위젯은 게임모드를 직접 알지 않고 UIModel의 Request*로 의도만 보낸다.
 * 그 의도를 실제 커맨드 발행 진입점(SelectSkill/SelectDice/RollDices/SelectMove/EndTurn)으로
 * 연결하는 유일한 지점이 여기다. 과거 임시 어댑터(UCombatUIAdapter)가 맡던 역할의 정식 대체.
 */
void ACombatGameMode::HandleCombatCommand(ECombatInputType Type, int32 IntPayload)
{
	switch (Type)
	{
	case ECombatInputType::SelectSkill:
		SelectSkill(IntPayload);
		break;
	case ECombatInputType::ToggleDice:
		SelectDice(IntPayload);
		break;
	case ECombatInputType::RollDice:
		RollDices();
		break;
	case ECombatInputType::Move:
		SelectMove();
		break;
	case ECombatInputType::EndTurn:
		EndTurn();
		break;
	case ECombatInputType::LongPressSkill:
		// 스킬을 길게 누르면 상세(이름/설명/사거리) 카드를 채운다 — 이 라우팅이 없어 "연결 대기중"만 떴었다.
		PushSkillDetailUIData(IntPayload);
		break;
	case ECombatInputType::Cancel:
	case ECombatInputType::LongPressUnit:
	case ECombatInputType::LongPressEquip:
		// 대응 진입점(취소/유닛·장비 상세 경로)이 아직 없다 — 각 기능 구현 시 여기서 라우팅한다.
		break;
	}
}

/**
 * @brief UI의 월드 터치 의도를 월드 트레이스 커맨드로 라우팅한다.
 *
 * @details
 * 조준 타일 선택/프리뷰 재조준/빈 타일 탭의 한 단계 취소가 전부 이 경로다.
 * ScreenPosition은 커맨드에 싣지 않는다 — Summit이 동기라 소비 측(GetTileActorUnderCursor)이
 * 트레이스하는 시점의 커서 위치가 곧 이 터치 위치다. 좌표를 커맨드로 옮기는 개편은 프레임워크 확장으로 남긴다.
 */
void ACombatGameMode::HandleCombatWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	/*
	 * 활성 플레이어 턴이 아니면 월드 터치를 게임플레이로 넘기지 않는다.
	 * 스킬로 적을 잡아 마지막 턴 노드가 제거되면 현재 턴이 잠깐 사라지는데(PushTurnUIData와 동일 상태),
	 * 그때 터치를 넘기면 SetTargetTile->SimulateUntilNextAction이 활성 턴 부재로
	 * 어설션("현재 전투가 진행 중이 아님")에 걸린다. 적 턴 중 조준도 무의미하므로 함께 막는다.
	 */
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatModel == nullptr)
	{
		return;
	}
	const USRPGTurnContext* TurnContext = CombatModel->GetCurrentTurnContext();
	if (TurnContext == nullptr || TurnContext->GetOwner()->IsPlayerUnitModel() == false)
	{
		return;
	}

	if (bLongPress == true)
	{
		ResolveWorldLongPressEvent();
	}
	else
	{
		ResolveWorldTouchEvent();
	}
}

void ACombatGameMode::HandleApplyDiceResults(const TArray<int32>& RolledFaceIndices)
{
	ApplyRolledDices(RolledFaceIndices);
}

/**
 * @brief 물리 굴림 연출이 확정한 면 index들을 굴림 커맨드에 실어 발행한다.
 *
 * @details
 * 입장 주사위 연출은 실제 물리로 굴려 멈춘 윗면을 보여준다. 그 "보이는 면"을 그대로
 * 게임 결과로 기록해야 표시와 판정이 어긋나지 않는다. 결과를 실은 DiceRoll 커맨드는
 * 대기 중인 USRPGDiceRollAction이 소비하며(내부 난수 굴림 대신 주입값 기록),
 * 이로써 굴림 액션이 완료되어 이후 스킬 빌드 커맨드가 흐를 수 있다.
 */
bool ACombatGameMode::ApplyRolledDices(const TArray<int32>& RolledFaceIndices)
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> DiceRollCommand;
	DiceRollCommand.InitializeAs<FSRPGDiceRollCommand>();
	DiceRollCommand.GetMutable<FSRPGDiceRollCommand>().mRolledFaceIndices = RolledFaceIndices;

	return CommandRouterModel->SummitCommand(DiceRollCommand);
}

void ACombatGameMode::BeginRoom()
{
	Super::BeginRoom();

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 nullptr"));
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 시스템 모델 nullptr"));

	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UCombatTileMapHUDWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<UCombatTileMapHUDWidget>())
		{
			// HUD가 게임모드 소유 뷰모델을 구독해야 Push*UIData가 화면에 반영된다(미바인딩이면 push 전체가 표시로 이어지지 않음).
			CombatHUD->BindCombatUIModel(mCombatUIModel);
			CombatHUD->OpenUI();                            // InitHUD로 생성만 된 HUD를 화면에 올림
		}
	}

	CombatModel->BeginCombat();
}

UCombatUIModel* ACombatGameMode::GetCombatUIModel() const
{
	return mCombatUIModel;
}

bool ACombatGameMode::SelectSkill(int32 SkillIndex)
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> SkillSelectCommand;
	SkillSelectCommand.InitializeAs<FSRPGSkillSelectCommand>();
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().mSkillIndex = SkillIndex;
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnChangeSkillBuildPhase.AddWeakLambda(this, [this](const USRPGSkillBuildAction* Action, ESRPGSkillBuildPhase Phase) {
		PushSkillBuildUIData(Phase);
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
		PushMoveBuildUIData(Phase);
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
		// mCombatUIModel->NotifyTargetDetailPanelRequested(Target);
		});

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

void ACombatGameMode::OnRegisterUnit(UUnitModel* Unit)
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Unit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	// 각 속성이 변경될 때마다 OnRefreshUnitUI를 브로드캐스트하도록 바인딩
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMovementAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});

	// 상태 이상 태그 변경 시에도 UI 갱신 바인딩
	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, EGameplayTagEventType::NewOrRemoved).AddWeakLambda(this, [this](const FGameplayTag Tag, int32 Count) {
		PushUnitUIData();
		});

	PushTurnUIData();
	PushUnitUIData();
}

void ACombatGameMode::OnUnregisterUnit(UUnitModel* Unit)
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Unit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).RemoveAll(this);
	// 해제는 구독(OnRegisterUnit)과 같은 속성 쌍이어야 한다 — 다른 속성을 지우면 no-op이라 바인딩이 잔존한다.
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMovementAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).RemoveAll(this);

	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);

	PushTurnUIData();
	PushUnitUIData();
}

void ACombatGameMode::PushTurnUIData() const
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	const TArray<TObjectPtr<UUnitModel>>& UnitModels = CombatModel->GetUnits();

	// 마지막 턴 노드가 제거된 직후(OnUnregisterUnit 경로)에는 현재 턴이 없을 수 있다 — 그때는 push를 생략한다.
	USRPGTurnContext* TurnContext = CombatModel->GetCurrentTurnContext();
	if (TurnContext == nullptr)
	{
		return;
	}

	FTurnUI TurnUI;
	TurnUI.mCurrentUnitId = TurnContext->GetOwner()->GetModelId();
	// 페이즈의 단일 소유자는 SetBuildPhase(빌드 이벤트) — 턴 스냅샷 push가 진행 중 페이즈를 덮지 않게 보존한다.
	TurnUI.mPhase = mCombatUIModel->GetTurnUI().mPhase;
	TurnUI.mRound = 0; // TODO : 라운드 수
	for (const TObjectPtr<UUnitModel>& UnitModel : UnitModels)
	{
		TurnUI.mTurnOrderUnitIds.Add(UnitModel->GetModelId());
	}
	mCombatUIModel->SetTurnUI(TurnUI);
}

void ACombatGameMode::PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const
{
	switch (Phase)
	{
	case ESRPGSkillBuildPhase::None:
	case ESRPGSkillBuildPhase::Build:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::None);
		break;
	case ESRPGSkillBuildPhase::AimSelection:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::AimSelection);
		break;
	case ESRPGSkillBuildPhase::Preview:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::Preview);
		break;
	}
}

void ACombatGameMode::PushMoveBuildUIData(ESRPGMoveBuildPhase Phase) const
{
	switch (Phase)
	{
	case ESRPGMoveBuildPhase::None:
	case ESRPGMoveBuildPhase::Build:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::None);
		break;
	case ESRPGMoveBuildPhase::DestSelection:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::AimSelection);
		break;
	case ESRPGMoveBuildPhase::Preview:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::Preview);
		break;
	}
}

void ACombatGameMode::PushUnitUIData() const
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	const TArray<TObjectPtr<UUnitModel>>& UnitModels = CombatModel->GetUnits();
	const int32 UnitModelNum = UnitModels.Num();

	TArray<FUnitUI> UnitUIDatas;
	UnitUIDatas.Init(FUnitUI(), UnitModelNum);

	for (int32 i = 0; i < UnitModelNum; ++i)
	{
		const TObjectPtr<UUnitModel>& UnitModel = UnitModels[i];
		FUnitUI& UnitUIData = UnitUIDatas[i];

		UAttributeSetComponentModel* AttributeSetComponentModel = UnitModel->GetAttributeComponentModel();
		checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

		UnitUIData.mIsPlayer = UnitModel->IsPlayerUnitModel();
		UnitUIData.mUnitId = UnitModel->GetModelId();
		UnitUIData.mTile = UnitModel->GetTileTransform().mIndex;
		UnitUIData.mHP = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute());
		UnitUIData.mMaxHP = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute());
		UnitUIData.mDefensePoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefenseAttribute());
		UnitUIData.mMaxMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMovementAttribute());

		UnitUIData.mStatusTags = AttributeSetComponentModel->GetOwnedGameplayTags(); // 모든 소유 태그가 아닌 고의적으로 넣은 태그만 해당

		// 죽는 유닛 등 뷰가 이미 없는 경로에서도 push가 돌 수 있어 null 가드한다.
		// mWorldLocation은 머리 위 HP바 스크린 투영(UnitBars)에 실사용된다.
		if (const AActor* UnitViewActor = UnitModel->GetView<AActor>())
		{
			UnitUIData.mWorldLocation = UnitViewActor->GetActorLocation();
		}
	}

	mCombatUIModel->SetUnitUIs(UnitUIDatas);
}

void ACombatGameMode::PushDiceUIData() const
{
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	UDicePoolModel* DicePoolModel = PlayerUnitModel->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 nullptr"));

	const TArray<TObjectPtr<UDiceModel>>& DiceModels = DicePoolModel->GetDices();
	const int32 DiceModelsNum = DiceModels.Num();

	TArray<FDiceSlotUI> DiceSlotUIDatas;
	DiceSlotUIDatas.Init(FDiceSlotUI(), DiceModelsNum);

	for (int32 i = 0; i < DiceModelsNum; ++i)
	{
		const TObjectPtr<UDiceModel>& DiceModel = DiceModels[i];
		FDiceSlotUI& DiceSlotUIData = DiceSlotUIDatas[i];

		// UObject 기본 GetPrimaryAssetId()는 무효 id를 반환한다 — 주사위의 원본 정적 데이터 id를 써야
		// 위젯의 희귀도 색(ResolveDiceRarity)/프리뷰 매칭이 동작한다.
		DiceSlotUIData.mDiceId = DiceModel->GetSourceDiceId();
		DiceSlotUIData.mResultValue = DiceModel->IsRolled() ? DiceModel->GetCurrentValue() : 0;
		DiceSlotUIData.mRolledFaceIndex = DiceModel->GetRolledFaceIndex();
		DiceSlotUIData.mIsRolled = DiceModel->IsRolled();
		DiceSlotUIData.mIsSelected = DicePoolModel->IsSelectedDice(i);
		DiceSlotUIData.mIsUsed = DiceModel->IsUsed();
		DiceSlotUIData.mFaceCount = DiceModel->GetFaceCount();
		DiceSlotUIData.mFaceValues = DiceModel->GetFaceValues();
		DiceSlotUIData.mFaceTextures = DiceModel->GetFaceTextures();

		DiceSlotUIData.mRarityText = FText::FromString(EnumToString(DiceModel->GetRarity()));
		DiceSlotUIData.mRarityColor = GetRarityColor(DiceModel->GetRarity());
	}

	mCombatUIModel->SetDiceUIs(DiceSlotUIDatas);
}

void ACombatGameMode::PushSelectedDiceUIData() const
{
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	UDicePoolModel* DicePoolModel = PlayerUnitModel->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 nullptr"));

	mCombatUIModel->SetSelectedDice(DicePoolModel->GetSelectedDices(), DicePoolModel->GetSelectedDiceSum());
}

void ACombatGameMode::PushSkillUIData() const
{
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	USkillComponentModel* SkillComponentModel = PlayerUnitModel->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("스킬 컴포넌트 nullptr"));

	const TArray<FSkillEntry>& SkillEntries = SkillComponentModel->GetSkills();
	const int32 SkillEntriesNum = SkillEntries.Num();

	TArray<FSkillUI> SkillUIDatas;
	SkillUIDatas.Init(FSkillUI(), SkillEntriesNum);

	for (int32 i = 0; i < SkillEntriesNum; ++i)
	{
		const FSkillEntry& SkillEntry = SkillEntries[i];
		FSkillUI& SkillUIData = SkillUIDatas[i];

		SkillUIData.mSkillIndex = i;   // UI가 스킬 선택 의도(SelectSkill)에 되돌려 보내는 왕복 식별자
		SkillUIData.mIsUsable = false;

		UStaticSkillData* StaticSkillData = (SkillEntry.IsValid() == true) ? SkillEntry.mData.Get() : nullptr;
		if (StaticSkillData != nullptr)
		{
			SkillUIData.mName = StaticSkillData->mName;
			SkillUIData.mIcon = StaticSkillData->mIcon.LoadSynchronous();
			SkillUIData.mDiceCost = StaticSkillData->mRequiredDiceCount;
			SkillUIData.mIsUsable = true;
			SkillUIData.mTargeting.mSelectShape = GetCombatSkillSelectShape(StaticSkillData->mAimPattern);
			SkillUIData.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRangeDefaultValue);
			SkillUIData.mTargeting.mSelectRangeRatio = StaticSkillData->mAimRangeRatio;
			SkillUIData.mTargeting.mHitShape = GetCombatSkillHitShape(StaticSkillData->mEffectPattern);
			SkillUIData.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectAreaDefaultValue);
			SkillUIData.mTargeting.mHitRangeRatio = StaticSkillData->mEffectAreaRatio;
			SkillUIData.mTargeting.mIsIndirect = StaticSkillData->mIsIndirect;
			SkillUIData.mTargeting.mIsPenetration = StaticSkillData->mIsPenetration;
		}
	}

	mCombatUIModel->SetSkillUIs(SkillUIDatas);
}

/**
 * @brief 롱프레스한 스킬의 상세 DTO를 UIModel로 push한다.
 *
 * @details
 * UI는 RequestLongPressSkill 직후 GetSkillDetail()을 동기로 읽는 계약이다.
 * Request -> OnCombatCommand -> 이 함수까지 전 구간이 동기 브로드캐스트라 UI의 읽기보다 항상 먼저 실행된다.
 * 데이터가 없는 슬롯이면 빈 상세(mSkillIndex만 유효)를 push해 UI가 폴백 문구를 띄우게 한다.
 */
void ACombatGameMode::PushSkillDetailUIData(int32 SkillIndex) const
{
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	USkillComponentModel* SkillComponentModel = PlayerUnitModel->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("스킬 컴포넌트 nullptr"));

	FSkillDetailUI SkillDetailUIData;
	SkillDetailUIData.mSkillIndex = SkillIndex;

	const FSkillEntry* SkillEntry = SkillComponentModel->GetSkill(SkillIndex);
	UStaticSkillData* StaticSkillData = (SkillEntry != nullptr && SkillEntry->IsValid() == true) ? SkillEntry->mData.Get() : nullptr;
	if (StaticSkillData != nullptr)
	{
		SkillDetailUIData.mName = StaticSkillData->mName;
		SkillDetailUIData.mDescription = StaticSkillData->mDescription;
		SkillDetailUIData.mIcon = StaticSkillData->mIcon.LoadSynchronous();
		SkillDetailUIData.mDiceCost = StaticSkillData->mRequiredDiceCount;
		SkillDetailUIData.mTargeting.mSelectShape = GetCombatSkillSelectShape(StaticSkillData->mAimPattern);
		SkillDetailUIData.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRangeDefaultValue);
		SkillDetailUIData.mTargeting.mSelectRangeRatio = StaticSkillData->mAimRangeRatio;
		SkillDetailUIData.mTargeting.mHitShape = GetCombatSkillHitShape(StaticSkillData->mEffectPattern);
		SkillDetailUIData.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectAreaDefaultValue);
		SkillDetailUIData.mTargeting.mHitRangeRatio = StaticSkillData->mEffectAreaRatio;
		SkillDetailUIData.mTargeting.mIsIndirect = StaticSkillData->mIsIndirect;
		SkillDetailUIData.mTargeting.mIsPenetration = StaticSkillData->mIsPenetration;
	}

	mCombatUIModel->SetSkillDetail(SkillDetailUIData);
}

void ACombatGameMode::PushEquipmentUIData() const
{
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	UEquipmentComponentModel* EquipmentComponentModel = PlayerUnitModel->GetEquipmentComponentModel();
	checkf(EquipmentComponentModel != nullptr, TEXT("장비 컴포넌트 nullptr"));

	const int32 EquipSlotNum = StaticCast<int32>(EEquipmentType::Count);

	TArray<FEquipmentUI> EquipmentUIDatas;
	EquipmentUIDatas.Init(FEquipmentUI(), EquipSlotNum);

	for (int32 i = 0; i < EquipSlotNum; ++i)
	{
		const EEquipmentType EquipType = StaticCast<EEquipmentType>(i);

		const FEquippedEntry* EquippedEntry = EquipmentComponentModel->GetEquipped(EquipType);
		FEquipmentUI& EquipmentUIData = EquipmentUIDatas[i];

		EquipmentUIData.mSlotIndex = i;
		EquipmentUIData.mName = GetEquipmentSlotFallbackName(EquipType);
		EquipmentUIData.mIsEquipped = false;

		const UStaticEquipmentData* StaticEquipmentData = EquippedEntry != nullptr ? EquippedEntry->mData.Get() : nullptr;
		if (StaticEquipmentData != nullptr)
		{
			EquipmentUIData.mItemId = StaticEquipmentData->GetPrimaryAssetId();
			EquipmentUIData.mName = StaticEquipmentData->mName.IsEmpty() == true ? EquipmentUIData.mName : StaticEquipmentData->mName;
			EquipmentUIData.mIcon = StaticEquipmentData->mIcon.LoadSynchronous();
			EquipmentUIData.mIsEquipped = true;
			EquipmentUIData.mRarityColor = GetRarityColor(StaticEquipmentData->mRarityType);
		}
	}

	mCombatUIModel->SetEquipmentUIs(EquipmentUIDatas);
}

void ACombatGameMode::PushPlayerMetaUIData() const
{
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnitModel->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	FPlayerMetaUI PlayerMetaUIData;
	PlayerMetaUIData.mLevel = PlayerUnitModel->GetPlayerLevel();
	PlayerMetaUIData.mGold = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMoneyAttribute());
	PlayerMetaUIData.mExp = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
	PlayerMetaUIData.mMaxExp = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxExpAttribute());

	mCombatUIModel->SetPlayerMeta(PlayerMetaUIData);
}

