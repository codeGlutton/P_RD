#include "GameMode/CombatGameMode.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"
#include "Setting/RDWorldSettings.h"

#include "Pawn/Player/PlayerUnitModel.h"
#include "Pawn/Enemy/EnemyUnitModel.h"

#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatUIModel.h"

#include "Actor/ActorView.h"

#include "SRPGFramework/SRPGCommand.h"

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGDiceRollAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"
#include "SRPGFramework/SRPGEnemyIntent.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Dice/DicePoolModel.h"
#include "Dice/DiceModel.h"

#include "TAS/Passive/TacticalPassive.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/DiceData/StaticDiceData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "Simulation/Logger/EventLogger.h"

#include "Actor/BoardActor/BoardSelectionTarget.h"
#include "Actor/TileMap/TileMapModel.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

namespace
{
	const FName SmashSkillAssetName(TEXT("DA_SwordNormalSmash_Common"));
	const FName PullSkillAssetName(TEXT("DA_SwordBlade_Rare"));
	const FName StaggerSkillAssetName(TEXT("DA_NomalDefense_Common"));
	const FName SwapSkillAssetName(TEXT("DA_NomalHeal_Common"));
	constexpr float InterventionSmashAimRange = 8.0f;

	EEnemyIntentResultUI GetEnemyIntentResultUI(ESRPGEnemyIntentResult Result)
	{
		switch (Result)
		{
		case ESRPGEnemyIntentResult::Executing:    return EEnemyIntentResultUI::Executing;
		case ESRPGEnemyIntentResult::Completed:    return EEnemyIntentResultUI::Completed;
		case ESRPGEnemyIntentResult::Missed:       return EEnemyIntentResultUI::Missed;
		case ESRPGEnemyIntentResult::Collision:    return EEnemyIntentResultUI::Collision;
		case ESRPGEnemyIntentResult::FriendlyFire: return EEnemyIntentResultUI::FriendlyFire;
		case ESRPGEnemyIntentResult::HitPlayer:    return EEnemyIntentResultUI::HitPlayer;
		case ESRPGEnemyIntentResult::HitObstacle:  return EEnemyIntentResultUI::HitObstacle;
		case ESRPGEnemyIntentResult::Cancelled:    return EEnemyIntentResultUI::Cancelled;
		case ESRPGEnemyIntentResult::Planned:
		default:                                   return EEnemyIntentResultUI::Planned;
		}
	}

	FText GetEnemyIntentResultFallback(ESRPGEnemyIntentResult Result)
	{
		switch (Result)
		{
		case ESRPGEnemyIntentResult::Executing:    return NSLOCTEXT("CombatGameMode", "IntentExecuting", "예정대로 실행 중");
		case ESRPGEnemyIntentResult::Completed:    return NSLOCTEXT("CombatGameMode", "IntentCompleted", "예정 행동 완료");
		case ESRPGEnemyIntentResult::Missed:       return NSLOCTEXT("CombatGameMode", "IntentMissed", "공격 빗나감!");
		case ESRPGEnemyIntentResult::Collision:    return NSLOCTEXT("CombatGameMode", "IntentCollision", "이동 경로 충돌!");
		case ESRPGEnemyIntentResult::FriendlyFire: return NSLOCTEXT("CombatGameMode", "IntentFriendlyFire", "적끼리 오사!");
		case ESRPGEnemyIntentResult::HitPlayer:    return NSLOCTEXT("CombatGameMode", "IntentHitPlayer", "플레이어 명중");
		case ESRPGEnemyIntentResult::HitObstacle:  return NSLOCTEXT("CombatGameMode", "IntentHitObstacle", "장애물에 막힘!");
		case ESRPGEnemyIntentResult::Cancelled:    return NSLOCTEXT("CombatGameMode", "IntentCancelled", "예정 행동 취소!");
		case ESRPGEnemyIntentResult::Planned:
		default:                                   return NSLOCTEXT("CombatGameMode", "IntentPlanned", "목표·경로 공개");
		}
	}

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

	const FMonsterRoom* GetMonsterRewardRoom(const FRoom& Room)
	{
		switch (Room.mType)
		{
		case ERoomType::Monster:
		case ERoomType::EliteMonster:
		case ERoomType::BossMonster:
			return &static_cast<const FMonsterRoom&>(Room);
		default:
			return nullptr;
		}
	}

	template <typename AssetType>
	const AssetType* LoadPrimaryAssetData(const FPrimaryAssetId& AssetId)
	{
		if (AssetId.IsValid() == false)
		{
			return nullptr;
		}

		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return nullptr;
		}

		if (const AssetType* LoadedAsset = AssetManager->GetPrimaryAssetObject<AssetType>(AssetId))
		{
			return LoadedAsset;
		}

		const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
		return Cast<AssetType>(AssetPath.TryLoad());
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
		if (AttrLog.mEffectAttribute == UCombatTargetAttributeSet::GetHPAttribute())
		{
			IconType = EFloatingLogIconType::HP;
			ColorType = AttrLog.mMagnitude > 0.f ? EFloatingLogColorType::Heal : EFloatingLogColorType::Damage;
		}
		else if (AttrLog.mEffectAttribute == UCombatTargetAttributeSet::GetMovementAttribute())
		{
			IconType = EFloatingLogIconType::GetMove;
			ColorType = EFloatingLogColorType::PointUp;
		}
		else if (AttrLog.mEffectAttribute == UCombatTargetAttributeSet::GetDefenseAttribute())
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

void ACombatGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FStage& CurStage = GetRunPersistData()->GetStage();
	const FRoom& CurRoom = GetRunPersistData()->GetCurrentRoom();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticStageSpawnData* StaticStageData = AssetManager->GetPrimaryAssetObject<UStaticStageSpawnData>(CurStage.mStaticStageSpawnDataId);
	checkf(StaticStageData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = CurRoom.mType == ERoomType::BossMonster ? StaticStageData->mBossMonsterRoomBGM : StaticStageData->mMonsterRoomBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous(), false);
}

void ACombatGameMode::InitializeRoom()
{
	Super::InitializeRoom();
	mCombatRewardClaimed = false;
	mCombatRewardGoldClaimed = false;
	mCombatRewardExpClaimed = false;
	mCombatRewardChoiceClaimedIndices.Reset();

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
	CombatModel->OnEnemyIntentsChangedUI.AddWeakLambda(this, [this]() {
		// 플레이어 행동 뒤 재계획 시점에는 강제 이동도 이미 끝나 있다. 컨텍스트 액션은
		// FUnitUI의 타일 스냅샷으로 인접 여부를 판단하므로, 계획보다 먼저 현재 좌표를 밀어
		// 기사 주변의 선택 칸까지 끌려온 적의 '붙잡아 던지기'가 즉시 다시 나타나게 한다.
		PushUnitUIData();
		PushEnemyIntentUIData();
		if (USRPGCombatModel* UpdatedCombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
		{
			UpdatedCombatModel->RefreshEnemyIntentHighlights();
		}
		});

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
		OnEndCombatUI.Broadcast(Barrier, Result);
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		PushTurnUIData();
		PushUnitUIData();
		PushDiceUIData();
		PushSkillUIData();
		PushEnemyIntentUIData();
		PushEquipmentUIData();
		if (USRPGCombatModel* CurrentCombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
		{
			CurrentCombatModel->RefreshEnemyIntentHighlights();
		}
		// 턴 시작 연출: 배리어를 HUD로 넘겨 턴 배너가 끝날 때까지 실제 턴 실행을 대기시킨다.
		OnBeginAnyTurnUI.Broadcast(Barrier, TurnContext);
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
		// 이동/조준이 자기 하이라이트를 정리한 뒤에도 아직 남은 공개 경로와 공격선을 다시 보여준다.
		if (USRPGCombatModel* CurrentCombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this))
		{
			CurrentCombatModel->RefreshEnemyIntentHighlights();
		}
		// [비활성화] 실행 후 잠깐 떴다 사라지는 레거시 실행 로그(mIsPreview=false). 프리뷰(조준)만 쓰기로 함.
		// 로그는 여전히 소비(Pop)해 쌓이지 않게 비운다.
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
		PushDiceUIData();
		});
	DicePoolModel->OnUnselectedDiceUI.AddWeakLambda(this, [this](const UDiceModel* Dice) {
		PushDiceUIData();
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
	UStaticCombatRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticCombatRoomSpawnData>(CurRoom.mStaticRoomSpawnDataId);
	checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));
	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RD 월드 세팅 nullptr"));

	FTransform SpawnPointTransform = FTransform::Identity;
	AActor* SettingPointActor = WorldSettings->GetRoomStartPoint(GetRoomSpawnSettingName());
	if (SettingPointActor != nullptr)
	{
		SpawnPointTransform = SettingPointActor->GetActorTransform();
	}
	CombatModel->InitCombat(StaticRoomData, GetPlayerUnitModel(), SpawnPointTransform);
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
	case ECombatInputType::ConfirmSkill:
		ConfirmSkill();
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
	case ECombatInputType::Cancel:
		CancelSkill();
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
		PushDisplacementPreviewUIData(Action, Phase);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnPostSimulateSkillAction.AddWeakLambda(this, [this](const TArray<FSRPGTurnEventLog>& EventLogs) {
		PushSimulationFloatingLogs(EventLogs);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnCancelSimulateSkillAction.AddWeakLambda(this, [this]() {
		mCombatUIModel->NotifyCombatFloatingLogsCleared();
		});

	return CommandRouterModel->SummitCommand(SkillSelectCommand);
}

bool ACombatGameMode::ConfirmSkill()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
	TInstancedStruct<FSRPGCommand> Command;
	Command.InitializeAs<FSRPGSkillConfirmCommand>();
	return CommandRouterModel->SummitCommand(Command);
}

bool ACombatGameMode::CancelSkill()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));
	TInstancedStruct<FSRPGCommand> Command;
	Command.InitializeAs<FSRPGSkillCancelCommand>();
	return CommandRouterModel->SummitCommand(Command);
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
	for (const TObjectPtr<USRPGTurnContext>& TurnContext : TurnContexts)
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
	case ESRPGSkillBuildPhase::ThrowDestinationSelection:
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::ThrowDestinationSelection);
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
		UnitUIData.mPortrait = UnitModel->GetBoardActorPortrait();   // 턴 순서 칩 등 상시 UI용(없으면 nullptr → 텍스트 폴백).
		UnitUIData.mTile = UnitModel->GetTileTransform().mIndex;
		UnitUIData.mHP = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetHPAttribute());
		UnitUIData.mMaxHP = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMaxHPAttribute());
		UnitUIData.mDefensePoint = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetDefenseAttribute());
		UnitUIData.mMaxMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UCombatTargetAttributeSet::GetMovementAttribute());
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

	mCombatUIModel->SetDiceState(
		DiceSlotUIDatas,
		DicePoolModel->GetSelectedDices(),
		DicePoolModel->GetSelectedDiceSum());
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
			const bool bIsSmash = StaticSkillData->GetFName() == SmashSkillAssetName;
			const bool bIsPull = StaticSkillData->GetFName() == PullSkillAssetName;
			const bool bIsStagger = StaticSkillData->GetFName() == StaggerSkillAssetName;
			const bool bIsSwap = StaticSkillData->GetFName() == SwapSkillAssetName;
			const bool bIsDisplacement = bIsSmash || bIsPull || bIsStagger || bIsSwap;
			SkillUIData.mName = bIsPull
				? NSLOCTEXT("CombatGameMode", "PullSkillName", "끌어당기기")
				: (bIsSmash
					? NSLOCTEXT("CombatGameMode", "ThrowSkillName", "밀기 · 던지기")
					: (bIsStagger
						? NSLOCTEXT("CombatGameMode", "StaggerSkillName", "다리 걸기")
						: (bIsSwap
							? NSLOCTEXT("CombatGameMode", "SwapSkillName", "자리 바꾸기")
							: StaticSkillData->mName)));
			SkillUIData.mIcon = StaticSkillData->mIcon.LoadSynchronous();
			SkillUIData.mDiceCost = StaticSkillData->mRequiredDiceCount;
			SkillUIData.mIsUsable = true;
			SkillUIData.mIsDisplacementSkill = bIsDisplacement;
			SkillUIData.mIsPullSkill = bIsPull;
			SkillUIData.mIsThrowSkill = bIsSmash;
			SkillUIData.mIsStaggerSkill = bIsStagger;
			SkillUIData.mIsSwapSkill = bIsSwap;
			SkillUIData.mTargeting.mSelectShape = bIsDisplacement
				? ECombatSkillSelectShapeUI::Square
				: GetCombatSkillSelectShape(StaticSkillData->mAimPattern);
			SkillUIData.mTargeting.mSelectRange = bIsPull || bIsStagger
				? InterventionSmashAimRange
				: (bIsSmash || bIsSwap ? 1.0f : StaticCast<float>(StaticSkillData->mAimRangeDefaultValue));
			SkillUIData.mTargeting.mSelectRangeRatio = bIsDisplacement ? 0.0f : StaticSkillData->mAimRangeRatio;
			SkillUIData.mTargeting.mHitShape = bIsDisplacement
				? ECombatSkillHitShapeUI::Single
				: GetCombatSkillHitShape(StaticSkillData->mEffectPattern);
			SkillUIData.mTargeting.mHitRange = bIsDisplacement ? 0.0f : StaticCast<float>(StaticSkillData->mEffectAreaDefaultValue);
			SkillUIData.mTargeting.mHitRangeRatio = bIsDisplacement ? 0.0f : StaticSkillData->mEffectAreaRatio;
			SkillUIData.mTargeting.mIsIndirect = bIsDisplacement || StaticSkillData->mIsIndirect;
			SkillUIData.mTargeting.mIsPenetration = StaticSkillData->mIsPenetration;
		}
	}

	mCombatUIModel->SetSkillUIs(SkillUIDatas);
}

void ACombatGameMode::PushDisplacementPreviewUIData(
	const USRPGSkillBuildAction* Action,
	ESRPGSkillBuildPhase Phase) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));
	FDisplacementPreviewUI Preview;
	if (Action == nullptr
		|| (Action->IsPullDisplacementPreview() == false
			&& Action->IsThrowDisplacementPreview() == false
			&& Action->IsStaggerDisplacementPreview() == false
			&& Action->IsSwapDisplacementPreview() == false)
		|| Phase == ESRPGSkillBuildPhase::None
		|| Phase == ESRPGSkillBuildPhase::Build)
	{
		mCombatUIModel->SetDisplacementPreview(Preview);
		return;
	}

	const USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	const UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	UUnitModel* Target = Action->GetDisplacementTarget();
	if (TileMap == nullptr || Target == nullptr)
	{
		mCombatUIModel->SetDisplacementPreview(Preview);
		return;
	}

	Preview.mIsActive = true;
	Preview.mIsPull = Action->IsPullDisplacementPreview();
	Preview.mIsThrow = Action->IsThrowDisplacementPreview();
	Preview.mIsStagger = Action->IsStaggerDisplacementPreview();
	Preview.mIsSwap = Action->IsSwapDisplacementPreview();
	Preview.mTargetUnitId = Target->GetModelId();
	Preview.mTargetName = Target->GetBoardActorDisplayName();
	Preview.mTargetTile = Target->GetTileTransform().mIndex;
	Preview.mTargetWorldLocation = TileMap->TileToWorldLocation(Preview.mTargetTile);
	for (const FTileIndex& TileIndex : Action->GetDisplacementTrajectory())
	{
		Preview.mTrajectoryWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
	}
	for (const FTileIndex& TileIndex : Action->GetDisplacementDestinationCandidates())
	{
		Preview.mDirectionCandidateWorldLocations.Add(TileMap->TileToWorldLocation(TileIndex));
	}
	if (Action->GetDisplacementTrajectory().IsEmpty() == false)
	{
		Preview.mLandingTile = Action->GetDisplacementTrajectory().Last();
		Preview.mLandingWorldLocation = TileMap->TileToWorldLocation(Preview.mLandingTile);
		Preview.mMoveDistance = FMath::Max(Action->GetDisplacementTrajectory().Num() - 1, 0);
	}
	if (UBoardActorModel* Blocker = Action->GetDisplacementCollisionBlocker())
	{
		Preview.mCollisionTile = Action->GetDisplacementDestination();
		Preview.mCollisionWorldLocation = TileMap->TileToWorldLocation(Preview.mCollisionTile);
		Preview.mCollisionName = Blocker->GetBoardActorDisplayName();
	}
	mCombatUIModel->SetDisplacementPreview(Preview);
}

/** @brief 전투 모델의 공개 계획을 UObject 참조 없는 HUD 스냅샷으로 변환한다. */
void ACombatGameMode::PushEnemyIntentUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	const USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatModel == nullptr)
	{
		return;
	}

	TArray<FEnemyIntentUI> IntentUIDatas;
	IntentUIDatas.Reserve(CombatModel->GetEnemyIntents().Num());
	for (const FSRPGEnemyIntent& Intent : CombatModel->GetEnemyIntents())
	{
		FEnemyIntentUI& IntentUI = IntentUIDatas.AddDefaulted_GetRef();
		IntentUI.mExecutionOrder = Intent.mExecutionOrder;
		IntentUI.mActionName = Intent.mSkillName.IsEmpty()
			? NSLOCTEXT("CombatGameMode", "IntentFallbackAction", "이동 후 공격")
			: Intent.mSkillName;
		IntentUI.mGoalText = Intent.mGoalText;
		IntentUI.mPlannedOrigin = Intent.mPlannedOrigin;
		IntentUI.mPlannedDestination = Intent.mPlannedDestination;
		IntentUI.mTargetTile = Intent.mTargetTile;
		IntentUI.mCurrentTile = IsValid(Intent.mEnemy)
			? Intent.mEnemy->GetTileTransform().mIndex
			: FTileIndex::Invalid;
		if (IntentUI.mCurrentTile == FTileIndex::Invalid)
		{
			IntentUI.mCurrentTile = Intent.mDisplacedToTile != FTileIndex::Invalid
				? Intent.mDisplacedToTile
				: Intent.mPlannedOrigin;
		}
		IntentUI.mPathTileIndexes = Intent.mPathTileIndexes;
		IntentUI.mEffectTileIndexes = Intent.mEffectTileIndexes;
		IntentUI.mPreviousPathTileIndexes = Intent.mPreviousPathTileIndexes;
		IntentUI.mPreviousDestination = Intent.mPreviousDestination;
		IntentUI.mResult = GetEnemyIntentResultUI(Intent.mResult);
		IntentUI.mResultText = Intent.mResultText.IsEmpty() ? GetEnemyIntentResultFallback(Intent.mResult) : Intent.mResultText;
		IntentUI.mWasDisplaced = Intent.mWasDisplaced;
		IntentUI.mPlanRevision = Intent.mPlanRevision;
		IntentUI.mResponseCostSpent = Intent.mResponseCostSpent;
		IntentUI.mIsRecommendedInterventionTarget = Intent.mIsRecommendedInterventionTarget;

		if (IsValid(Intent.mEnemy))
		{
			IntentUI.mEnemyUnitId = Intent.mEnemy->GetModelId();
			IntentUI.mEnemyName = Intent.mEnemy->GetBoardActorDisplayName();
			switch (Intent.mEnemy->GetDisplacementWeight())
			{
			case ESRPGDisplacementWeight::Light:
				IntentUI.mDisplacementWeightLabel = NSLOCTEXT("CombatGameMode", "WeightLight", "경량 · 멀리 날아감");
				break;
			case ESRPGDisplacementWeight::Heavy:
				IntentUI.mDisplacementWeightLabel = NSLOCTEXT("CombatGameMode", "WeightHeavy", "중량 · 투척 -2칸");
				break;
			case ESRPGDisplacementWeight::Medium:
			default:
				IntentUI.mDisplacementWeightLabel = NSLOCTEXT("CombatGameMode", "WeightMedium", "중형 · 투척 -1칸");
				break;
			}
		}
		if (IntentUI.mEnemyName.IsEmpty())
		{
			IntentUI.mEnemyName = FText::Format(
				NSLOCTEXT("CombatGameMode", "IntentEnemyFallback", "적 {0}"),
				FText::AsNumber(IntentUI.mExecutionOrder > 0 ? IntentUI.mExecutionOrder : IntentUIDatas.Num()));
		}
	}

	IntentUIDatas.Sort([](const FEnemyIntentUI& Lhs, const FEnemyIntentUI& Rhs) {
		return Lhs.mExecutionOrder < Rhs.mExecutionOrder;
		});
	mCombatUIModel->SetEnemyIntentUIs(IntentUIDatas);
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
		const bool bIsSmash = StaticSkillData->GetFName() == SmashSkillAssetName;
		const bool bIsPull = StaticSkillData->GetFName() == PullSkillAssetName;
		const bool bIsDisplacement = bIsSmash || bIsPull;
		SkillDetailUIData.mName = bIsPull
			? NSLOCTEXT("CombatGameMode", "PullSkillName", "끌어당기기")
			: (bIsSmash
				? NSLOCTEXT("CombatGameMode", "ThrowSkillName", "붙잡아 던지기")
				: StaticSkillData->mName);
		SkillDetailUIData.mDescription = bIsSmash
			? NSLOCTEXT(
				"CombatGameMode",
				"ThrowDescription",
				"[던지기] 인접한 적을 붙잡고 8방향 중 하나를 선택합니다. 큰 화살표를 고르면 실제 착지와 충돌을 미리 보여주며, 실행 버튼을 눌러 확정합니다.")
			: (bIsPull
				? NSLOCTEXT(
					"CombatGameMode",
					"PullSkillDescription",
					"[당기기] 적을 드래그해 플레이어 주변의 유효한 빈칸을 직접 고릅니다. 손가락을 따라오는 적과 밝은 착지 타일을 보고 놓으면 실행됩니다.")
				: StaticSkillData->mDescription);
		SkillDetailUIData.mIcon = StaticSkillData->mIcon.LoadSynchronous();
		SkillDetailUIData.mDiceCost = StaticSkillData->mRequiredDiceCount;
		SkillDetailUIData.mTargeting.mSelectShape = bIsDisplacement
			? ECombatSkillSelectShapeUI::Square
			: GetCombatSkillSelectShape(StaticSkillData->mAimPattern);
		SkillDetailUIData.mTargeting.mSelectRange = bIsPull
			? InterventionSmashAimRange
			: (bIsSmash ? 1.0f : StaticCast<float>(StaticSkillData->mAimRangeDefaultValue));
		SkillDetailUIData.mTargeting.mSelectRangeRatio = bIsDisplacement ? 0.0f : StaticSkillData->mAimRangeRatio;
		SkillDetailUIData.mTargeting.mHitShape = bIsDisplacement
			? ECombatSkillHitShapeUI::Single
			: GetCombatSkillHitShape(StaticSkillData->mEffectPattern);
		SkillDetailUIData.mTargeting.mHitRange = bIsDisplacement ? 0.0f : StaticCast<float>(StaticSkillData->mEffectAreaDefaultValue);
		SkillDetailUIData.mTargeting.mHitRangeRatio = bIsDisplacement ? 0.0f : StaticSkillData->mEffectAreaRatio;
		SkillDetailUIData.mTargeting.mIsIndirect = bIsDisplacement || StaticSkillData->mIsIndirect;
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

FRewardUI ACombatGameMode::MakeCombatRewardUI() const
{
	FRewardUI RewardUI;
	RewardUI.mTitle = NSLOCTEXT("CombatGameMode", "VictoryRewardTitle", "VICTORY REWARD");

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData != nullptr)
	{
		if (const FMonsterRoom* CurrentRoom = GetMonsterRewardRoom(RunPersistData->GetCurrentRoom()))
		{
			RewardUI.mGoldGained = CurrentRoom->mRewardMoney;
			RewardUI.mExpGained = CurrentRoom->mRewardExp;
		}
	}

	const UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	const UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnitModel != nullptr ? PlayerUnitModel->GetAttributeComponentModel() : nullptr;
	if (AttributeSetComponentModel != nullptr)
	{
		const float CurrentGold = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMoneyAttribute());
		const float CurrentExp = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
		const float MaxExp = AttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxExpAttribute());

		RewardUI.mGoldBalance = FMath::RoundToInt(CurrentGold) + RewardUI.mGoldGained;
		RewardUI.mExpBefore = CurrentExp;
		RewardUI.mExpAfter = CurrentExp + StaticCast<float>(RewardUI.mExpGained);
		RewardUI.mMaxExp = MaxExp;
	}

	if (PlayerUnitModel != nullptr)
	{
		const int32 PlayerLevel = PlayerUnitModel->GetPlayerLevel();
		RewardUI.mLevelBefore = PlayerLevel;
		RewardUI.mLevelAfter = PlayerLevel;
	}

	return RewardUI;
}

TArray<FRewardChoiceUI> ACombatGameMode::MakeCombatRewardChoicesUI() const
{
	TArray<FRewardChoiceUI> Choices;

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr)
	{
		return Choices;
	}

	const FRoom& CurrentRoom = RunPersistData->GetCurrentRoom();
	auto AddEquipmentReward = [&Choices](const FPrimaryAssetId& EquipmentId)
	{
		if (EquipmentId.IsValid() == false)
		{
			return;
		}

		FRewardChoiceUI Choice;
		Choice.mChoiceIndex = Choices.Num();
		Choice.mKind = ERewardChoiceKind::Equipment;
		Choice.mSourceAssetId = EquipmentId;
		Choice.mName = FText::FromName(EquipmentId.PrimaryAssetName);

		if (const UStaticEquipmentData* EquipmentData = LoadPrimaryAssetData<UStaticEquipmentData>(EquipmentId))
		{
			Choice.mName = EquipmentData->mName.IsEmpty() ? Choice.mName : EquipmentData->mName;
			Choice.mDescription = EquipmentData->mDescription;
			Choice.mIcon = EquipmentData->mIcon.LoadSynchronous();
			Choice.mRarityColor = GetRarityColor(EquipmentData->mRarityType);
		}

		Choices.Add(Choice);
	};

	auto AddDiceReward = [&Choices](const FPrimaryAssetId& DiceId)
	{
		if (DiceId.IsValid() == false)
		{
			return;
		}

		FRewardChoiceUI Choice;
		Choice.mChoiceIndex = Choices.Num();
		Choice.mKind = ERewardChoiceKind::Dice;
		Choice.mSourceAssetId = DiceId;
		Choice.mName = FText::FromName(DiceId.PrimaryAssetName);

		if (const UStaticDiceData* DiceData = LoadPrimaryAssetData<UStaticDiceData>(DiceId))
		{
			Choice.mDescription = FText::Format(
				NSLOCTEXT("CombatGameMode", "DiceRewardDescription", "d{0}"),
				FText::AsNumber(DiceData->mFaceCount));
			Choice.mRarityColor = GetRarityColor(DiceData->mRarityType);

			for (const FStaticDiceFaceData& FaceData : DiceData->mFaces)
			{
				if (FaceData.mTexture.IsNull() == false)
				{
					Choice.mIcon = FaceData.mTexture.LoadSynchronous();
					break;
				}
			}
		}

		Choices.Add(Choice);
	};

	switch (CurrentRoom.mType)
	{
	case ERoomType::EliteMonster:
		AddEquipmentReward(static_cast<const FEliteMonsterRoom&>(CurrentRoom).mRewardEquipmentDataId);
		break;
	case ERoomType::BossMonster:
		AddDiceReward(static_cast<const FBossMonsterRoom&>(CurrentRoom).mRewardDiceDataId);
		break;
	default:
		break;
	}

	return Choices;
}

bool ACombatGameMode::ClaimCombatReward()
{
	if (mCombatRewardClaimed)
	{
		return false;
	}

	bool bClaimedAny = false;
	bClaimedAny |= ClaimCombatReward(ERewardClaimKind::Gold, INDEX_NONE);
	bClaimedAny |= ClaimCombatReward(ERewardClaimKind::Exp, INDEX_NONE);

	for (const FRewardChoiceUI& Choice : MakeCombatRewardChoicesUI())
	{
		bClaimedAny |= ClaimCombatReward(ERewardClaimKind::Choice, Choice.mChoiceIndex);
	}

	mCombatRewardClaimed = true;
	return bClaimedAny;
}

bool ACombatGameMode::ClaimCombatReward(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	URunPersistData* RunPersistData = GetRunPersistData();
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel();
	UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnitModel != nullptr ? PlayerUnitModel->GetAttributeComponentModel() : nullptr;
	if (RunPersistData == nullptr)
	{
		return false;
	}

	const FRoom& CurrentRoomData = RunPersistData->GetCurrentRoom();
	const FMonsterRoom* CurrentRoom = GetMonsterRewardRoom(CurrentRoomData);

	if (ClaimKind == ERewardClaimKind::Gold)
	{
		if (mCombatRewardGoldClaimed || CurrentRoom == nullptr || AttributeSetComponentModel == nullptr || CurrentRoom->mRewardMoney == 0)
		{
			mCombatRewardGoldClaimed = true;
			return false;
		}

		AttributeSetComponentModel->ApplyModToAttribute(
			UPlayerUnitAttributeSet::GetMoneyAttribute(),
			ETacticalModOp::AddBase,
			StaticCast<float>(CurrentRoom->mRewardMoney));

		mCombatRewardGoldClaimed = true;
		PushPlayerMetaUIData();
		return true;
	}

	if (ClaimKind == ERewardClaimKind::Exp)
	{
		if (mCombatRewardExpClaimed || CurrentRoom == nullptr || AttributeSetComponentModel == nullptr || CurrentRoom->mRewardExp == 0)
		{
			mCombatRewardExpClaimed = true;
			return false;
		}

		AttributeSetComponentModel->ApplyModToAttribute(
			UPlayerUnitAttributeSet::GetExpAttribute(),
			ETacticalModOp::AddBase,
			StaticCast<float>(CurrentRoom->mRewardExp));

		mCombatRewardExpClaimed = true;
		PushPlayerMetaUIData();
		return true;
	}

	if (ClaimKind != ERewardClaimKind::Choice || ChoiceIndex == INDEX_NONE || mCombatRewardChoiceClaimedIndices.Contains(ChoiceIndex))
	{
		return false;
	}

	const TArray<FRewardChoiceUI> Choices = MakeCombatRewardChoicesUI();
	const FRewardChoiceUI* FoundChoice = Choices.FindByPredicate([ChoiceIndex](const FRewardChoiceUI& Choice)
	{
		return Choice.mChoiceIndex == ChoiceIndex;
	});
	if (FoundChoice == nullptr)
	{
		return false;
	}

	bool bClaimed = false;
	switch (FoundChoice->mKind)
	{
	case ERewardChoiceKind::Equipment:
		bClaimed = RunPersistData->AddRewardEquipment(FoundChoice->mSourceAssetId);
		break;
	case ERewardChoiceKind::Dice:
		bClaimed = RunPersistData->AddRewardDice(FoundChoice->mSourceAssetId);
		break;
	case ERewardChoiceKind::Skill:
		bClaimed = RunPersistData->AddRewardSkill(FoundChoice->mSourceAssetId);
		break;
	case ERewardChoiceKind::Gold:
	default:
		break;
	}

	if (bClaimed)
	{
		mCombatRewardChoiceClaimedIndices.Add(ChoiceIndex);
	}

	return bClaimed;
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

	auto AddFloatingLogs = [IsPreview](const FSRPGBoardActorEventLog& EventLog, const FVector& ViewActorLocation, const int32 TurnIndex, const int32 ActionIndex, const int32 MotionIndex, OUT int32& Sequence, OUT TArray<FCombatFloatingLogRequest>& Requests) {

		auto MakeLogRequest = [IsPreview, TurnIndex, ActionIndex, MotionIndex](int32 Amount, EFloatingLogIconType IconType, EFloatingLogColorType ColorType, const FVector& ViewLocation, int32 Sequence) -> FCombatFloatingLogRequest {
			FCombatFloatingLogRequest Request;
			Request.mWorldLocation = ViewLocation;
			Request.mText = FText::FromString(FString::Printf(TEXT("%+d"), Amount));
			Request.mIconType = IconType;
			Request.mColorType = ColorType;
			Request.mSequence = Sequence;
			Request.mTurnIndex = IsPreview == true ? TurnIndex : INDEX_NONE;
			Request.mActionIndex = IsPreview == true ? ActionIndex : INDEX_NONE;
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
				Sequence++
			));*/
		}
		};

	/* 시뮬레이션 로그 탐색 시작 */

	int32 Sequence = 0;
	TArray<FCombatFloatingLogRequest> Requests;
	TMap<int32, FVector> SpawnLocations;
	const int32 TurnNum = TurnEventLogs.Num();
	for (int32 TurnIndex = 0; TurnIndex < TurnNum; ++TurnIndex)
	{
		const FSRPGTurnEventLog& TurnLog = TurnEventLogs[TurnIndex];
		const int32 ActionNum = TurnLog.mActionEventLogs.Num();
		for (int32 ActionIndex = 0; ActionIndex < ActionNum; ++ActionIndex)
		{
			const FSRPGActionEventLog& ActionLog = TurnLog.mActionEventLogs[ActionIndex];
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

					AddFloatingLogs(*EventLog, SpawnLocationPair.Value, TurnIndex, ActionIndex, MotionIndex, OUT Sequence, OUT Requests);
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
					AddFloatingLogs(*EventLog, ViewActorLocation, TurnIndex, ActionIndex, MotionIndex, OUT Sequence, OUT Requests);
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
					AddFloatingLogs(*EventLog, ViewActorLocation, TurnIndex, ActionIndex, MotionIndex, OUT Sequence, OUT Requests);
				}
			}
		}
	}

	if (Requests.Num() > 0)
	{
		mCombatUIModel->NotifyCombatFloatingLogs(Requests);
	}
}
