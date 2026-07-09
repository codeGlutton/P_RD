#include "GameMode/CombatGameMode.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Pawn/Player/PlayerUnitModel.h"

#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatUIModel.h"

#include "Actor/ActorView.h"

#include "SRPGFramework/SRPGCommand.h"

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGDiceRollAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Dice/DicePoolModel.h"
#include "Dice/DiceModel.h"

#include "TAS/Passive/TacticalPassive.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "Simulation/Logger/EventLogger.h"

#include "Actor/BoardActor/BoardSelectionTarget.h"
#include "Actor/TileMap/TileMapModel.h"

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

	void ConvertFloatingLogUITypes(const FSRPGTileEffectEventLog& TileLog, OUT EFloatingLogIconType& IconType, OUT EFloatingLogColorType& ColorType)
	{
		IconType = EFloatingLogIconType::Move;
		ColorType = EFloatingLogColorType::Move;
		if (TileLog.mOccupancyState != ESRPGTileOccupancyState::Move)
		{
			// 스폰이나 죽음
			ColorType = EFloatingLogColorType::Warning;
		}
	}

	void ConvertFloatingLogUITypes(const FSRPGTagEffectEventLog& TagLog, OUT EFloatingLogIconType& IconType, OUT EFloatingLogColorType& ColorType)
	{
		if (TagLog.mEffectTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility))
		{
			IconType = EFloatingLogIconType::Agility;
			ColorType = EFloatingLogColorType::Buff;
		}
		else if (TagLog.mEffectTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification))
		{
			IconType = EFloatingLogIconType::Fortification;
			ColorType = EFloatingLogColorType::Buff;
		}
		else if (TagLog.mEffectTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability))
		{
			IconType = EFloatingLogIconType::Vulnerability;
			ColorType = EFloatingLogColorType::Debuff;
		}
		else if (TagLog.mEffectTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness))
		{
			IconType = EFloatingLogIconType::Weakness;
			ColorType = EFloatingLogColorType::Debuff;
		}
	}

	void ConvertFloatingLogUITypes(const FSRPGAttributeEffectEventLog& AttrLog, OUT EFloatingLogIconType& IconType, OUT EFloatingLogColorType& ColorType)
	{
		if (AttrLog.mEffectAttribute == UUnitAttributeSet::GetHPAttribute())
		{
			IconType = EFloatingLogIconType::HP;
			ColorType = AttrLog.mMagnitude > 0.f ? EFloatingLogColorType::Heal : EFloatingLogColorType::Damage;
		}
		else if (AttrLog.mEffectAttribute == UUnitAttributeSet::GetMovementAttribute())
		{
			IconType = EFloatingLogIconType::GetMove;
			ColorType = EFloatingLogColorType::PointUp;
		}
		else if (AttrLog.mEffectAttribute == UUnitAttributeSet::GetDefenseAttribute())
		{
			IconType = EFloatingLogIconType::GetDefense;
			ColorType = EFloatingLogColorType::PointUp;
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

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	/*
	 * UI와 전투 로직을 여기서 연결한다.
	 * - 게임 상태가 바뀌면 UIModel에 새 값을 넣는다.
	 * - UI 버튼/터치 입력은 전투 명령으로 보낸다.
	 */

	/* 전투 모델 대리자 연결 */

	CombatModel->OnRegisterUnitUI.AddUObject(this, &ACombatGameMode::OnRegisterUnit);
	CombatModel->OnUnregisterUnitUI.AddUObject(this, &ACombatGameMode::OnUnregisterUnit);

	CombatModel->OnShowDicePanelAnyTurnUI.AddWeakLambda(this, [this](const USRPGTurnContext* TurnContext) {
		// 턴 시작 주사위 준비(DicePrepare) 시점 — 굴림 오버레이를 열라고 UI에 통지한다.
		mCombatUIModel->NotifyDiceRollRequested();
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
	// 라운드 시작 연출: 데이터(mRound) 먼저 갱신 후 배리어를 HUD로 중계한다(순서 보장 위해 게임모드가 재방송).
	// 프레임워크가 OnBeginAnyRoundUI를 방송하기 전까지 이 람다는 호출되지 않는다(휴면). 방송 배선은 SRPGCombatModel TODO 참고.
	CombatModel->OnBeginAnyRoundUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, int32 RoundCount) {
		PushTurnUIData();   // 배너 숫자용 mRound 갱신
		OnBeginAnyRoundUI.Broadcast(Barrier, RoundCount);
		});
	CombatModel->OnEndAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		// OnEndAnyTurnUI.Broadcast(Barrier, TurnContext, Result); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnBeginAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		mCombatUIModel->NotifyCombatFloatingLogsCleared();
		// OnBeginAnyTurnActionUI.Broadcast(Barrier, TurnContext, Action); 연출은 연결고리가 아직 없음
		});
	CombatModel->OnEndAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result) {
		// 액션이 끝나면 UI의 스킬/주사위 선택 표시를 지운다.
		mCombatUIModel->NotifyActionResolved();
		if (UEventLogger* EventLogger = GetWorldEventLogger(this))
		{
			// 액션 실행 로그는 UI로 띄우지 않고 소비만 해서, 다음 조준 프리뷰에 섞이지 않게 한다.
			EventLogger->PopSRPGLogs();
		}
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
		PushSkillUIData();
		});

	SkillComponentModel->OnEndMotionLayerUI.AddWeakLambda(this, [this](int32 MotionIndex) {
		mCombatUIModel->NotifyCombatFloatingLogMotionFinished(MotionIndex);
		});

	/* UI 조작 의도 라우팅 — 위젯 탭이 쏘는 Request*(OnCombatCommand)를 게임플레이 진입점에 연결 */

	mCombatUIModel->OnApplyDiceResults.AddUniqueDynamic(this, &ACombatGameMode::HandleApplyDiceResults);
	mCombatUIModel->OnCombatCommand.AddUniqueDynamic(this, &ACombatGameMode::HandleCombatCommand);
	mCombatUIModel->OnCombatWorldTouch.AddUniqueDynamic(this, &ACombatGameMode::HandleCombatWorldTouch);

	const FStage& CurStage = GetRunPersistData()->GetStage();
	const FRoom& CurRoom = GetRunPersistData()->GetCurrentRoom();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticStageSpawnData* StaticStageData = AssetManager->GetPrimaryAssetObject<UStaticStageSpawnData>(CurStage.mStaticStageSpawnDataId);
	checkf(StaticStageData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));
	UStaticCombatRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticCombatRoomSpawnData>(CurRoom.mStaticRoomSpawnDataId);
	checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = CurRoom.mType == ERoomType::BossMonster ? StaticStageData->mBossMonsterRoomBGM : StaticStageData->mMonsterRoomBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous(), false);

	CombatModel->InitCombat(StaticRoomData, GetPlayerUnitModel());
}

/**
 * @brief UIModel에서 올라온 버튼 입력을 전투 명령으로 보낸다.
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
		// 길게 누른 스킬의 상세 정보를 UIModel에 채운다.
		PushSkillDetailUIData(IntPayload);
		break;
	case ECombatInputType::LongPressEquip:
		// 길게 누른 장비의 상세 정보를 UIModel에 채운다.
		PushEquipmentDetailUIData(IntPayload);
		break;
	}
}

/**
 * @brief 월드 탭/롱프레스를 조준 입력으로 처리한다.
 *
 * @details
 * 지금 명령에는 ScreenPosition을 싣지 않는다. 전투 로직은 현재 커서 아래 타일을 직접 찾는다.
 */
void ACombatGameMode::HandleCombatWorldTouch(FVector2D ScreenPosition, bool bLongPress)
{
	/*
	 * 플레이어 턴이 아닐 때는 무시한다.
	 * 턴이 비어 있거나 적 턴일 때 넘기면 프리뷰 시뮬레이션에서 크래시가 날 수 있다.
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
		ResolveWorldLongPressEvent(ScreenPosition);
	}
	else
	{
		ResolveWorldTouchEvent(ScreenPosition);
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

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 시스템 모델 nullptr"));

	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (URDUserWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<URDUserWidget>())
		{
			CombatHUD->OpenUI();
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
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnSelectSkill.AddWeakLambda(this, [this](int32 SkillIndex) {
		PushSelectedSkillUIData(SkillIndex);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnChangeSkillBuildPhase.AddWeakLambda(this, [this](const USRPGSkillBuildAction* Action, ESRPGSkillBuildPhase Phase) {
		PushSkillBuildUIData(Phase);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnPostSimulateSkillAction.AddWeakLambda(this, [this](const TArray<FSRPGTurnEventLog>& EventLogs) {
		PushSimulationFloatingLogs(EventLogs);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnCancelSimulateSkillAction.AddWeakLambda(this, [this]() {
		mCombatUIModel->NotifyCombatFloatingLogsCleared();
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

bool ACombatGameMode::ResolveWorldTouchEvent(FVector2D ScreenPosition)
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> WorldTraceActionCommand;
	WorldTraceActionCommand.InitializeAs<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().mIsLongPress = false;
	// 모바일 터치는 커서가 없으므로, 탭 화면 좌표를 커맨드에 실어 월드 트레이스에 사용한다.
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().mScreenPosition = ScreenPosition;

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

bool ACombatGameMode::ResolveWorldLongPressEvent(FVector2D ScreenPosition)
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> WorldTraceActionCommand;
	WorldTraceActionCommand.InitializeAs<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().mIsLongPress = true;
	// 모바일 터치는 커서가 없으므로, 롱프레스 화면 좌표를 커맨드에 실어 월드 트레이스에 사용한다.
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().mScreenPosition = ScreenPosition;
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().OnShowTargetDetailPanelUI.AddWeakLambda(this, [this](IBoardSelectionTarget* Target) {
		PushCombatTargetDetailUIData(Target);
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
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMovementAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).RemoveAll(this);

	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);

	PushTurnUIData();
	PushUnitUIData();
}

void ACombatGameMode::PushTurnUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	const TArray<TObjectPtr<USRPGTurnContext>> TurnContexts = CombatModel->GetOrderedTurnContexts();
	if (TurnContexts.IsEmpty() == true)
	{
		return;
	}

	FTurnUI TurnUI;
	TurnUI.mCurrentUnitId = TurnContexts[0]->GetOwner()->GetModelId();
	TurnUI.mPhase = mCombatUIModel->GetTurnUI().mPhase;
	TurnUI.mRound = CombatModel->GetRoundCount();
	for (const TObjectPtr<USRPGTurnContext> TurnContext : TurnContexts)
	{
		TurnUI.mTurnOrderUnitIds.Add(TurnContext->GetOwner()->GetModelId());
	}
	mCombatUIModel->SetTurnUI(TurnUI);
}

void ACombatGameMode::PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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
		UnitUIData.mMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMovementAttribute());

		UnitUIData.mStatusTags = AttributeSetComponentModel->GetOwnedGameplayTags(); // 모든 소유 태그가 아닌 고의적으로 넣은 태그만 해당

		// HP바 밑 상태이상 칸용: 부모/분류 태그는 제외하고, 실제 표시 대상 상태만 (태그 + 스택 수)로 채운다.
		// 텍스처 선택은 HUD(UpdateUnitHpBarStatus)가 소유하고, 스택 수는 태그 컨테이너의 누적 카운트에서 읽는다.
		UnitUIData.mStatusEffects.Reset();
		for (const FGameplayTag& StatusTag : UnitUIData.mStatusTags)
		{
			if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect) == false)
			{
				continue;
			}

			FStatusEffectUI StatusEffect;
			StatusEffect.mTag = StatusTag;
			StatusEffect.mStackCount = AttributeSetComponentModel->GetTagCount(StatusTag);
			UnitUIData.mStatusEffects.Add(StatusEffect);
		}

		// 죽는 유닛 등 뷰가 이미 없는 경로에서도 push가 돌 수 있어 null 가드한다.
		// mWorldLocation은 스냅샷 폴백, mViewActor는 이동을 매 프레임 따라가는 라이브 투영 소스(UnitBars).
		if (AActor* UnitViewActor = UnitModel->GetView<AActor>())
		{
			UnitUIData.mWorldLocation = UnitViewActor->GetActorLocation();
			UnitUIData.mViewActor = UnitViewActor;
		}
	}

	mCombatUIModel->SetUnitUIs(UnitUIDatas);
}

void ACombatGameMode::PushDiceUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	UDicePoolModel* DicePoolModel = PlayerUnitModel->GetDicePoolModel();
	checkf(DicePoolModel != nullptr, TEXT("주사위 컴포넌트 nullptr"));

	mCombatUIModel->SetSelectedDice(DicePoolModel->GetSelectedDices(), DicePoolModel->GetSelectedDiceSum());
}

void ACombatGameMode::PushSkillUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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

void ACombatGameMode::PushSelectedSkillUIData(int32 SkillIndex) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	mCombatUIModel->SetSelectedSkill(SkillIndex);
}

void ACombatGameMode::PushCombatTargetDetailUIData(IBoardSelectionTarget* Target) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	IObjectView* ObjectView = Cast<IObjectView>(Target);
	checkf(ObjectView != nullptr, TEXT("선택한 오브젝트 뷰 nullptr"));

	UBoardActorModel* BoardActorModel = ObjectView->GetModel<UBoardActorModel>();
	checkf(BoardActorModel != nullptr, TEXT("선택한 보드 액터 모델 nullptr"));

	FUnitDetailUI UnitDetailUIData;
	UnitDetailUIData.mUnitId = BoardActorModel->GetModelId();
	UnitDetailUIData.mName = BoardActorModel->GetBoardActorDisplayName();
	UnitDetailUIData.mLevel = BoardActorModel->GetBoardActorLevel();
	UnitDetailUIData.mPortrait = BoardActorModel->GetBoardActorPortrait();
	
	UUnitModel* UnitModel = ObjectView->GetModel<UUnitModel>();
	if (UnitModel != nullptr)
	{
		UPassiveComponentModel* PassiveComponentModel = UnitModel->GetPassiveComponentModel();
		checkf(PassiveComponentModel != nullptr, TEXT("선택한 유닛 모델의 패시브 컴포넌트 nullptr"));
		for (const TObjectPtr<UTacticalPassive>& Passive : PassiveComponentModel->GetPassives())
		{
			UnitDetailUIData.mPassiveDescriptions.Add(Passive->GetStaticData()->mDescription);
		}
	}
	mCombatUIModel->SetUnitDetail(UnitDetailUIData);
}

/**
 * @brief 길게 누른 스킬의 상세 정보를 UIModel에 넣는다.
 *
 * @details
 * UI는 요청 직후 이 값을 읽는다. 데이터가 없는 슬롯이면 빈 상세 정보를 넣는다.
 */
void ACombatGameMode::PushSkillDetailUIData(int32 SkillIndex) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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

void ACombatGameMode::PushEquipmentDetailUIData(int32 EquipmentIndex) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	UEquipmentComponentModel* EquipmentComponentModel = PlayerUnitModel->GetEquipmentComponentModel();
	checkf(EquipmentComponentModel != nullptr, TEXT("장비 컴포넌트 nullptr"));

	const EEquipmentType EquipSlotType = StaticCast<EEquipmentType>(EquipmentIndex);
	const FEquippedEntry* EquippedEntry = EquipmentComponentModel->GetEquipped(EquipSlotType);
	if (EquippedEntry == nullptr)
	{
		// 빈 슬롯(장착 안 됨)을 롱프레스한 경우: 상세를 띄우지 않고 조용히 반환(크래시 방지).
		return;
	}

	const UStaticEquipmentData* StaticEquipmentData = EquippedEntry->mData.Get();
	if (StaticEquipmentData == nullptr)
	{
		return;
	}

	FEquipmentDetailUI EquipmentDetailUIData;
	EquipmentDetailUIData.mSlotIndex = EquipmentIndex;
	EquipmentDetailUIData.mItemId = StaticEquipmentData->GetPrimaryAssetId();
	EquipmentDetailUIData.mName = StaticEquipmentData->mName;
	EquipmentDetailUIData.mIcon = StaticEquipmentData->mIcon.LoadSynchronous();
	EquipmentDetailUIData.mIsEquipped = true;
	EquipmentDetailUIData.mDescription = StaticEquipmentData->mDescription;
	EquipmentDetailUIData.mRarityColor = GetRarityColor(StaticEquipmentData->mRarityType);

	mCombatUIModel->SetEquipmentDetail(EquipmentDetailUIData);
}

void ACombatGameMode::PushPlayerMetaUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

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

void ACombatGameMode::PushSimulationFloatingLogs(const TArray<FSRPGTurnEventLog>& TurnEventLogs, bool IsPreview) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	UTileMapModel* TileMapModel = CombatModel->GetTileMap();
	checkf(TileMapModel != nullptr, TEXT("타일맵 모델 nullptr"));

	/* 새로운 시뮬마다 이전 남은 로그 지우기 */

	if (IsPreview == true)
	{
		mCombatUIModel->NotifyCombatFloatingLogsCleared();
	}

	/* 모션 내 이벤트 로그 마다 UI 요청서 작성 함수 */

	auto AddFloatingLogs = [IsPreview](const FSRPGBoardActorEventLog& EventLog, const FVector& ViewActorLocation, const int32 MotionIndex, OUT int32& Sequence, OUT TArray<FCombatFloatingLogRequest>& Requests) {
		
		auto MakeLogRequest = [IsPreview](int32 Amount, EFloatingLogIconType IconType, EFloatingLogColorType ColorType, const FVector& ViewLocation, int32 MotionIndex, int32 Sequence) -> FCombatFloatingLogRequest {
			FCombatFloatingLogRequest Request;
			Request.mWorldLocation = ViewLocation;
			Request.mText = FText::FromString(FString::Printf(TEXT("%+d"), Amount));
			Request.mIconType = IconType;
			Request.mColorType = ColorType;
			Request.mSequence = Sequence;
			Request.mMotionIndex = IsPreview == true ? MotionIndex : INDEX_NONE;
			Request.mIsPreview = IsPreview;
			return Request;
			};
		
		for (const FSRPGAttributeEffectEventLog& AttrLog : EventLog.mAttributeEffectEventLogs)
		{
			EFloatingLogIconType IconType = EFloatingLogIconType::None;
			EFloatingLogColorType ColorType = EFloatingLogColorType::Neutral;
			ConvertFloatingLogUITypes(AttrLog, OUT IconType, OUT ColorType);

			if (IconType == EFloatingLogIconType::None && ColorType == EFloatingLogColorType::Neutral)
			{
				continue;
			}

			Requests.Add(MakeLogRequest(
				FMath::Floor(AttrLog.mMagnitude),
				IconType,
				ColorType,
				ViewActorLocation,
				MotionIndex,
				Sequence++
			));
		}
		for (const FSRPGTagEffectEventLog& TagLog : EventLog.mTagEffectEventLogs)
		{
			EFloatingLogIconType IconType = EFloatingLogIconType::None;
			EFloatingLogColorType ColorType = EFloatingLogColorType::Neutral;
			ConvertFloatingLogUITypes(TagLog, OUT IconType, OUT ColorType);

			if (IconType == EFloatingLogIconType::None && ColorType == EFloatingLogColorType::Neutral)
			{
				continue;
			}

			Requests.Add(MakeLogRequest(
				TagLog.mCount,
				IconType,
				ColorType,
				ViewActorLocation,
				MotionIndex,
				Sequence++
			));
		}
		for (const FSRPGTileEffectEventLog& TileLog : EventLog.mTileEffectEventLogs)
		{
			EFloatingLogIconType IconType = EFloatingLogIconType::None;
			EFloatingLogColorType ColorType = EFloatingLogColorType::Neutral;
			ConvertFloatingLogUITypes(TileLog, OUT IconType, OUT ColorType);

			if (IconType == EFloatingLogIconType::None && ColorType == EFloatingLogColorType::Neutral)
			{
				continue;
			}

			// TODO : 
			// 어디서 어디로 이동했다는 정보는 어떻게 알려야하나
			/*Requests.Add(MakeLogRequest(
				TagLog.mCount,
				IconType,
				ColorType,
				ViewActorLocation,
				MotionIndex,
				Sequence++
			));*/
		}
		};

	/* 시뮬레이션 로그 탐색 시작 */

	int32 Sequence = 0;
	TArray<FCombatFloatingLogRequest> Requests;
	TMap<int32, FVector> SpawnLocations;
	for (const FSRPGTurnEventLog& TurnLog : TurnEventLogs)
	{
		for (const FSRPGActionEventLog& ActionLog : TurnLog.mActionEventLogs)
		{
			const int32 MotionNum = ActionLog.mMotionEventLogs.Num();
			for (int32 MotionIndex = 0; MotionIndex < MotionNum; ++MotionIndex)
			{
				const FSRPGMotionEventLog& MotionLog = ActionLog.mMotionEventLogs[MotionIndex];

				// 새롭게 태어난 액터는 임시로 위치만 등록
				for (const auto& SpawnedBoardActorPositionPair : MotionLog.mSpawnedBoardActorPositions)
				{
					SpawnLocations.Add(SpawnedBoardActorPositionPair.Key, TileMapModel->TileToWorldLocation(SpawnedBoardActorPositionPair.Value));
				}

				// 생성 액터 탐색
				for (const auto& SpawnLocationPair : SpawnLocations)
				{
					const FSRPGBoardActorEventLog* EventLog = MotionLog.mBoardActorEventLogs.Find(SpawnLocationPair.Key);
					if (EventLog == nullptr)
					{
						continue;
					}

					AddFloatingLogs(*EventLog, SpawnLocationPair.Value, MotionIndex, OUT Sequence, OUT Requests);
				}

				// 유닛 탐색
				const TArray<TObjectPtr<UUnitModel>>& Units = CombatModel->GetUnits();
				for (const TObjectPtr<UUnitModel>& Unit : Units)
				{
					const FSRPGBoardActorEventLog* EventLog = MotionLog.mBoardActorEventLogs.Find(Unit->GetModelId());
					if (EventLog == nullptr)
					{
						continue;
					}

					AActor* ViewActor = Unit->GetView<AActor>();
					if (ViewActor == nullptr)
					{
						continue;
					}

					FVector ViewActorLocation = ViewActor->GetActorLocation();
					AddFloatingLogs(*EventLog, ViewActorLocation, MotionIndex, OUT Sequence, OUT Requests);
				}

				// 장애물 탐색
				const TArray<TObjectPtr<UBoardActorModel>>& Obstacles = CombatModel->GetObstacles();
				for (const TObjectPtr<UBoardActorModel>& Obstacle : Obstacles)
				{
					const FSRPGBoardActorEventLog* EventLog = MotionLog.mBoardActorEventLogs.Find(Obstacle->GetModelId());
					if (EventLog == nullptr)
					{
						continue;
					}

					AActor* ViewActor = Obstacle->GetView<AActor>();
					if (ViewActor == nullptr)
					{
						continue;
					}

					FVector ViewActorLocation = ViewActor->GetActorLocation();
					AddFloatingLogs(*EventLog, ViewActorLocation, MotionIndex, OUT Sequence, OUT Requests);
				}
			}
		}
	}

	if (Requests.Num() > 0)
	{
		mCombatUIModel->NotifyCombatFloatingLogs(Requests);
	}
}
