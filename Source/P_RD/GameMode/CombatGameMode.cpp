#include "GameMode/CombatGameMode.h"

#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"

#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"
#include "Setting/RDWorldSettings.h"

#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "FunctionLibrary/CameraFunctionLibrary.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"

#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIDebugFixture.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/CombatUIWidgetBase.h"
#include "UI/Combat/SimulationPreviewUIModel.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/ArtifactRewardPolicy.h"

#include "Actor/ActorView.h"

#include "SRPGFramework/SRPGCommand.h"

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"

#include "TAS/Passive/TacticalPassive.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Setting/GameTeamType.h"
#include "Setting/GameBalanceSettings.h"

#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_GetActionPoint.h"
#include "Simulation/Logger/EventLogger.h"

#include "Actor/BoardActor/BoardSelectionTargetView.h"
#include "Actor/TileMap/TileMapModel.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

namespace
{
	/** @brief 결과 판이 열릴 때 전투 방 음악이 짧게 정리되는 시간. */
	constexpr float CombatResultBGMFadeOutSeconds = 0.35f;

	FString CombatPortraitIdentity(const UUnitModel* UnitModel)
	{
		return UnitModel != nullptr
			? FString::Printf(TEXT("%s %s %s"),
				*UnitModel->GetBoardActorKeyName().ToString(),
				*UnitModel->GetBoardActorAssetId().ToString(),
				*UnitModel->GetBoardActorDisplayName().ToString())
			: FString();
	}

	/**
	 * @brief Marchbound가 새로 그린 용병 전용 얼굴/히어로 그림을 직업으로 찾는다.
	 *
	 * 데이터에셋의 mIcon/mPortrait는 아직 구형 픽셀 초상을 가리키는 것이 있다.
	 * 용병 UI만 새 그림을 쓰도록 어댑터에서 한 번 정규화하면 턴바·목록·상세가
	 * 각자 다른 폴백 규칙을 갖지 않는다.
	 */
	UTexture2D* ResolveMarchboundMercenaryPortrait(const UUnitModel* UnitModel,
		const bool bHeroIllustration)
	{
		if (UnitModel == nullptr || UnitModel->IsPlayerUnitModel() == false)
		{
			return nullptr;
		}
		const FString Identity = CombatPortraitIdentity(UnitModel);
		struct FMercenaryPortraitRule
		{
			const TCHAR* KoreanNeedle;
			const TCHAR* EnglishNeedle;
			const TCHAR* AssetStem;
		};
		static const FMercenaryPortraitRule Rules[] = {
			{ TEXT("기사"), TEXT("Knight"), TEXT("Knight") },
			{ TEXT("마법사"), TEXT("Mage"), TEXT("Mage") },
			{ TEXT("궁수"), TEXT("Ranger"), TEXT("Ranger") },
			{ TEXT("도적"), TEXT("Rogue"), TEXT("Rogue") },
			{ TEXT("야만"), TEXT("Barbarian"), TEXT("Barbarian") },
			{ TEXT("드루이드"), TEXT("Druid"), TEXT("Druid") },
		};
		for (const FMercenaryPortraitRule& Rule : Rules)
		{
			if (Identity.Contains(Rule.KoreanNeedle, ESearchCase::IgnoreCase)
				|| Identity.Contains(Rule.EnglishNeedle, ESearchCase::IgnoreCase))
			{
				const FString AssetName = FString::Printf(TEXT("T_MB_Hire%s_%s"),
					bHeroIllustration ? TEXT("Hero") : TEXT("Icon"), Rule.AssetStem);
				const FString AssetPath = FString::Printf(
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/%s.%s"),
					*AssetName, *AssetName);
				return LoadObject<UTexture2D>(nullptr, *AssetPath);
			}
		}
		return nullptr;
	}

	UTexture2D* ResolveTurnPortraitFallback(const UUnitModel* UnitModel)
	{
		if (UnitModel == nullptr)
		{
			return nullptr;
		}
		if (UTexture2D* MercenaryPortrait =
			ResolveMarchboundMercenaryPortrait(UnitModel, false))
		{
			return MercenaryPortrait;
		}
		const FString Identity = CombatPortraitIdentity(UnitModel);

		struct FPortraitRule
		{
			const TCHAR* Needle;
			const TCHAR* Path;
		};
		static const FPortraitRule Rules[] = {
			{ TEXT("독수리"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2") },
			{ TEXT("Eagle"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2") },
			{ TEXT("Werewolf"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Werewolf_HeadV2.KK_Face_Enemy_Werewolf_HeadV2") },
			{ TEXT("Leshy"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Leshy_HeadV2.KK_Face_Enemy_Leshy_HeadV2") },
			{ TEXT("Mushroom"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Mushroom_HeadV2.KK_Face_Enemy_Mushroom_HeadV2") },
			{ TEXT("Necromancer"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Necromancer_HeadV2.KK_Face_Enemy_Necromancer_HeadV2") },
			{ TEXT("SkeletonGolem"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_SkeletonGolem_HeadV2.KK_Face_Enemy_SkeletonGolem_HeadV2") },
			{ TEXT("SkeletonMinionRanged"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_SkeletonMinionRanged_HeadV2.KK_Face_Enemy_SkeletonMinionRanged_HeadV2") },
			{ TEXT("SkeletonMinionMelee"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_SkeletonMinionMelee_HeadV2.KK_Face_Enemy_SkeletonMinionMelee_HeadV2") },
			{ TEXT("SkeletonMinion"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_SkeletonMinion_HeadV2.KK_Face_Enemy_SkeletonMinion_HeadV2") },
			{ TEXT("Slime"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Slime_HeadV2.KK_Face_Enemy_Slime_HeadV2") },
			{ TEXT("Spider"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Spider_HeadV2.KK_Face_Enemy_Spider_HeadV2") },
			{ TEXT("Golem"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Golem_HeadV2.KK_Face_Enemy_Golem_HeadV2") },
		};
		for (const FPortraitRule& Rule : Rules)
		{
			if (Identity.Contains(Rule.Needle, ESearchCase::IgnoreCase))
			{
				return LoadObject<UTexture2D>(nullptr, Rule.Path);
			}
		}
		return LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Portraits/KK_Face_Enemy_Eagle_HeadV2.KK_Face_Enemy_Eagle_HeadV2"));
	}

	/** @brief 결과판용 용병 초상화를 기존 우선순위대로 해상한다. */
	UTexture2D* ResolveCombatPartyPortrait(const UPlayerUnitModel* PlayerUnitModel)
	{
		if (PlayerUnitModel == nullptr)
		{
			return nullptr;
		}

		UTexture2D* Portrait = ResolveMarchboundMercenaryPortrait(
			PlayerUnitModel, false);
		if (Portrait == nullptr)
		{
			Portrait = PlayerUnitModel->GetBoardActorIcon();
		}
		if (Portrait == nullptr)
		{
			Portrait = PlayerUnitModel->GetBoardActorPortrait();
		}
		if (Portrait == nullptr)
		{
			Portrait = ResolveTurnPortraitFallback(PlayerUnitModel);
		}
		return Portrait;
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
		if (TagLog.mEffectTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Vigor))
		{
			IconType = EFloatingLogIconType::Vigor;
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
		else if (AttrLog.mEffectAttribute == UUnitAttributeSet::GetActionPointAttribute())
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

#if !UE_BUILD_SHIPPING

namespace CombatAutoBattleDev
{
	void ExecuteCommand(const TArray<FString>& Args, UWorld* World)
	{
		ACombatGameMode* GameMode = World != nullptr
			? World->GetAuthGameMode<ACombatGameMode>() : nullptr;
		if (GameMode == nullptr)
		{
			UE_LOG(LogCombatGameMode, Warning,
				TEXT("P_RD.AutoBattle: 전투 게임 월드를 찾지 못했습니다."));
			return;
		}

		bool bEnabled = !GameMode->IsAutoBattleEnabled();
		if (Args.IsValidIndex(0))
		{
			const FString& Value = Args[0];
			if (Value == TEXT("1") || Value.Equals(TEXT("on"), ESearchCase::IgnoreCase)
				|| Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bEnabled = true;
			}
			else if (Value == TEXT("0") || Value.Equals(TEXT("off"), ESearchCase::IgnoreCase)
				|| Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				bEnabled = false;
			}
		}

		GameMode->SetAutoBattleEnabled(bEnabled);
	}

	FAutoConsoleCommandWithWorldAndArgs AutoBattleCommand(
		TEXT("P_RD.AutoBattle"),
		TEXT("개발용 자동전투를 켜거나 끈다. 사용법: P_RD.AutoBattle [1|0|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ExecuteCommand));
}

#endif

ACombatGameMode::ACombatGameMode()
{
	mCombatUIModel = CreateDefaultSubobject<UCombatUIModel>(TEXT("CombatUIModel"));
	mRewardUIModel = CreateDefaultSubobject<URewardUIModel>(TEXT("RewardUIModel"));
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

	InitializeCombat();
}

void ACombatGameMode::InitializeCombat()
{
	mGoldRewardClaimed = false;
	mExpRewardClaimed = false;
	mClaimedRewardChoiceIndices.Reset();
	mRewardSelectionClaimed = false;
	mSelectedRewardArtifactId = FPrimaryAssetId();

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

	CombatModel->OnSaveCombatPlay.AddWeakLambda(this, [this](const TArray<TObjectPtr<UUnitModel>>& PlayerModels, int32 RoundCount, int32 TurnCount) {
		UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
		checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));

		FRoomClearData RoomClearData;
		RoomClearData.mIsCleared = true;
		RoomClearData.mRoundCount = RoundCount;
		RoomClearData.mTurnCount = TurnCount;

		const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnitModels = GetPlayerUnitModels();
		for (const UPlayerUnitModel* PlayerUnitModel : PlayerUnitModels)
		{
			if (PlayerUnitModel != nullptr)
			{
				RoomClearData.mPlayerTileTransforms.Add(PlayerUnitModel->GetTileTransform());
			}
			else
			{
				RoomClearData.mPlayerTileTransforms.Add(FTileTransform::Invalid);
			}
		}

		GameProfileSubsystem->SetRoomClearData(RoomClearData);
		SaveRunWithUIAsync();
		});
	CombatModel->OnShowCombatResultUI.AddWeakLambda(this, [this](ESRPGCombatResult Result) {
		if (Result == ESRPGCombatResult::PlayerWin)
		{
			PushCombatRewardUIData();
			PushCombatRewardChoicesUIData();
		}
		mCombatUIModel->NotifyCombatResultOpenRequested();
		});

	CombatModel->OnBeginCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier) {
		PushPlayerMetaUIData();
		mCombatUIModel->OnBeginCombat.Broadcast(Barrier);
		});
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result) {
		CancelPendingActionEndAfterCameraReturn();
		// 승리와 패배 모두 결과 화면에 들어가기 전에 방 음악을 정리한다.
		// 화면 전환용 StartFadeOutUI와 달리 결과 UI는 같은 전투 맵 위에 뜬다.
		FadeOutMainBGM(CombatResultBGMFadeOutSeconds);
		PushPlayerMetaUIData();
		PushCombatResultUIData(Result);
		mCombatUIModel->OnEndCombat.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		// 차례가 왔으니 남의 카드를 접는다. 새 차례에 옛 유닛 카드가 떠 있으면
		// 무엇을 조종하는 중인지 알 수 없다.
		mInspectedUnitId = INDEX_NONE;
	#if !UE_BUILD_SHIPPING
		if (mAutoBattleEnabled && TurnContext != nullptr
			&& TurnContext->GetOwner() != nullptr
			&& TurnContext->GetOwner()->IsPlayerUnitModel())
		{
			// EndTurn 뒤의 새 플레이어 턴에서 다음 자동 행동을 시작한다.
			// 이전 액션 상태를 남겨 두면 턴이 바뀐 뒤에도 낡은 대상을 재확정할 수 있다.
			ResetAutoBattleAction();
		}
	#endif
		PushTurnUIData();
		PushUnitUIData();
		PushSkillUIData();
		// 턴 시작 연출: 배리어를 HUD로 넘겨 턴 배너가 끝날 때까지 실제 턴 실행을 대기시킨다.
		mCombatUIModel->OnBeginAnyTurn.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyRoundUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, int32 RoundCount) {
		PushTurnUIData();
		mCombatUIModel->OnBeginAnyRound.Broadcast(Barrier);
		});
	CombatModel->OnEndAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		CancelPendingActionEndAfterCameraReturn();
		// 턴이 실제로 끝났다 — 남은 미리보기는 전제부터 낡았으니 예측 쪽만 통째로 버린다.
		mCombatUIModel->GetSimulationPreviewUIModel()->ClearPreview();
		if (USimulationSubsystem* SimulationSubsystem = GetWorld()->GetSubsystem<USimulationSubsystem>())
		{
			PushCombatEventUIData(SimulationSubsystem->ConsumeGameEventLogs());
		}
		mCombatUIModel->OnEndAnyTurn.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		// 이전 액션의 카메라 복귀 대기가 남아 있어도 새 액션을 끝내면 안 된다.
		CancelPendingActionEndAfterCameraReturn();
	#if !UE_BUILD_SHIPPING
		if (mAutoBattleEnabled && mAutoBattlePhase == EAutoBattlePhase::WaitingForAction
			&& TurnContext != nullptr && Action != nullptr
			&& Action->GetInstigator() == mAutoBattleUnit.Get()
			&& Action->GetActionType() == ESRPGActionType::InPlayAction)
		{
			mAutoBattleSawInPlayAction = true;
		}
	#endif
		// 행동이 시작되면 그 전의 예측 전제는 낡았다 — 미리보기만 통째로 버린다.
		// (실전 juice 로그는 수명 규칙으로 스스로 사라진다.)
		mCombatUIModel->GetSimulationPreviewUIModel()->ClearPreview();
		mCombatUIModel->OnBeginAnyTurnAction.Broadcast(Barrier);
		});
	CombatModel->OnEndAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action, ESRPGActionResult Result) {
		// 방금 쓴 스킬에 쿨타임이 걸렸다. 다시 안 내리면 카드가 옛 숫자를 그대로
		// 들고 있어, 쓴 스킬이 아직 쓸 수 있는 것처럼 보인다.
		//
		// 행동력도 줄었으므로 유닛도 같이 내린다 -- 속성 델리게이트가 잡아
		// 주기는 하지만, 취소로 끝난 행동은 속성이 안 바뀌어 안 온다.
		PushSkillUIData();
		PushUnitUIData();
	#if !UE_BUILD_SHIPPING
		if (mAutoBattleEnabled && mAutoBattlePhase == EAutoBattlePhase::WaitingForAction
			&& TurnContext != nullptr && Action != nullptr
			&& Action->GetInstigator() == mAutoBattleUnit.Get()
			&& Action->GetActionType() == ESRPGActionType::InPlayAction)
		{
			// BuildAction 종료는 실제 스킬/이동이 시작되기 전 단계다. 실제
			// InPlayAction이 끝난 뒤에만 다음 자동 행동을 계획한다.
			ResetAutoBattleAction();
		}
	#endif
		mCombatUIModel->NotifyActionResolved();
		mCombatUIModel->OnEndAnyTurnAction.Broadcast(Barrier);
		});

	/* 스킬 대리자 연결 -- 파티 **전부**에게 건다.
	 *
	 * 0번에게만 걸고 있었다. 그러면 야만전사가 스킬을 바꾸거나 모션을 끝내도
	 * 화면이 모른다 -- 파티가 한 명일 때 짜 놓은 것이 셋이 되며 드러났다.
	 */

	for (UPlayerUnitModel* PartyUnitModel : GetPlayerUnitModels())
	{
		if (PartyUnitModel == nullptr)
		{
			continue;
		}
		USkillComponentModel* SkillComponentModel = PartyUnitModel->GetSkillComponentModel();
		if (SkillComponentModel == nullptr)
		{
			continue;
		}

		SkillComponentModel->OnChangeSkillUI.AddWeakLambda(this, [this](int32 SkillIndex, const UStaticSkillData* PreSkillData, const UStaticSkillData* NewSkillData) {
			PushSkillUIData();
			});

		SkillComponentModel->OnEndPhaseLayerUI.AddWeakLambda(this, [this](int32 PhaseIndex) {
			mCombatUIModel->NotifyCombatFloatingLogMotionFinished(PhaseIndex);
			});
	}

	/* UI 조작 의도 라우팅 — 위젯 탭이 쏘는 Request*(OnCombatCommand)를 게임플레이 진입점에 연결 */

	mCombatUIModel->OnCombatCommand.AddUniqueDynamic(this, &ACombatGameMode::HandleCombatCommand);
	mCombatUIModel->OnCombatWorldTouch.AddUniqueDynamic(this, &ACombatGameMode::HandleCombatWorldTouch);
#if !UE_BUILD_SHIPPING
	mCombatUIModel->OnAutoBattleToggleRequested.AddUniqueDynamic(
		this, &ACombatGameMode::HandleAutoBattleToggleRequested);
#endif
	mCombatUIModel->OnAbandonRun.AddUniqueDynamic(this, &ACombatGameMode::HandleAbandonRun);
	mCombatUIModel->OnSaveAndExitRun.AddUniqueDynamic(this, &ACombatGameMode::HandleSaveAndExitRun);
	mCombatUIModel->OnChangeFocusScreenAnchor.AddUObject(this, &ACombatGameMode::HandleChangeFocusScreenAnchor);
	// HUD가 먼저 만들어져 앵커를 등록한 경우에도 구독 직후 같은 값을 카메라에 적용한다.
	HandleChangeFocusScreenAnchor(mCombatUIModel->GetFocusScreenAnchor());
	mRewardUIModel->OnRewardClaimRequested.AddUniqueDynamic(this, &ACombatGameMode::HandleRewardClaimed);
	mRewardUIModel->OnRewardSelectionRequested.AddUniqueDynamic(
		this, &ACombatGameMode::HandleRewardSelectionRequested);

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

	// 사망 태그는 용병을 PartyModel에서 즉시 제거한다. 결과 데이터를 전투가
	// 끝난 뒤 조립해도 참가자 초상화가 남도록, 전투 진입 전에 따로 보존한다.
	mCombatStartPartyPortraits.Reset();
	mCombatStartPartyPortraits.Reserve(3);
	for (const UPlayerUnitModel* PlayerUnitModel : GetPlayerUnitModels())
	{
		if (PlayerUnitModel == nullptr)
		{
			continue;
		}
		mCombatStartPartyPortraits.Add(ResolveCombatPartyPortrait(PlayerUnitModel));
		if (mCombatStartPartyPortraits.Num() >= 3)
		{
			break;
		}
	}
	CombatModel->InitCombat(StaticRoomData, GetPlayerUnitModels(), SpawnPointTransform, CurStage.mClearData);
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
			// 모델은 **여기서 건네준다.** UI 가 게임모드를 거꾸로 찾아오면
			// UI 레이어에 게임모드 의존이 생긴다(#426 에서 걷어낸 그것).
			// 붙이고 나서 연다 -- 열면서 첫 갱신이 도는데 그때 모델이 있어야
			// 한 프레임 빈 화면이 안 스친다.
			if (UCombatUIWidgetBase* CombatUIWidget = Cast<UCombatUIWidgetBase>(CombatHUD))
			{
				CombatUIWidget->BindUIModel(mCombatUIModel);
			}
			if (UCombatLayoutHUDWidget* CombatLayoutHUDWidget = Cast<UCombatLayoutHUDWidget>(CombatHUD))
			{
				CombatLayoutHUDWidget->BindRewardUIModel(mRewardUIModel);
			}
			CombatHUD->OpenUI();
		}
	}

	CombatModel->BeginCombat();

#if !UE_BUILD_SHIPPING
	// 자동전투는 개발 빌드의 현재 전투방에서만 폴링한다. 기본값은 꺼져
	// 있으므로 기존 플레이 흐름과 저장 데이터에는 영향을 주지 않는다.
	GetWorld()->GetTimerManager().SetTimer(
		mAutoBattleTimerHandle, this, &ACombatGameMode::AutoBattlePulse, 0.15f, true);
#endif
}

UCombatUIModel* ACombatGameMode::GetCombatUIModel() const
{
	return mCombatUIModel;
}

URewardUIModel* ACombatGameMode::GetRewardUIModel() const
{
	return mRewardUIModel;
}

#if !UE_BUILD_SHIPPING

void ACombatGameMode::SetAutoBattleEnabled(const bool bEnabled)
{
	mAutoBattleEnabled = bEnabled;
	ResetAutoBattleAction();
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->SetAutoBattleEnabled(bEnabled);
	}

	UE_LOG(LogCombatGameMode, Display,
		TEXT("P_RD.AutoBattle: %s"), bEnabled ? TEXT("켜짐") : TEXT("꺼짐"));
}

bool ACombatGameMode::IsAutoBattleEnabled() const
{
	return mAutoBattleEnabled;
}

void ACombatGameMode::ResetAutoBattleAction()
{
	mAutoBattlePhase = EAutoBattlePhase::Idle;
	mAutoBattleUnit.Reset();
	mAutoBattleSkillIndex = INDEX_NONE;
	mAutoBattleTargetTile = FTileIndex::Invalid;
	mAutoBattleTurnId = INDEX_NONE;
	mAutoBattleSawInPlayAction = false;
	mAutoBattleWaitingPulseCount = 0;
}

bool ACombatGameMode::IsAutoBattleHostile(const UUnitModel* Unit) const
{
	return Unit != nullptr && Unit->IsDead() == false
		&& FGenericTeamId::GetAttitude(
			EGameTeamType::Adventurer, Unit->GetGenericTeamId()) == ETeamAttitude::Hostile;
}

bool ACombatGameMode::FindAutoBattleSkill(
	UPlayerUnitModel* PlayerUnit, OUT int32& SkillIndex, OUT FTileIndex& TargetTile) const
{
	SkillIndex = INDEX_NONE;
	TargetTile = FTileIndex::Invalid;
	if (PlayerUnit == nullptr)
	{
		return false;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	USkillComponentModel* SkillComponent = PlayerUnit->GetSkillComponentModel();
	if (TileMap == nullptr || SkillComponent == nullptr)
	{
		return false;
	}

	float BestScore = -MAX_flt;
	const TArray<FSkillEntry>& Skills = SkillComponent->GetSkills();
	for (int32 CandidateSkillIndex = 0; CandidateSkillIndex < Skills.Num(); ++CandidateSkillIndex)
	{
		if (Skills[CandidateSkillIndex].IsValid() == false
			|| SkillComponent->CanActiveSkill(CandidateSkillIndex) == false)
		{
			continue;
		}

		const UStaticUnitSkillData* SkillData = Cast<UStaticUnitSkillData>(
			Skills[CandidateSkillIndex].mData.Get());
		int32 MinDamage = 0;
		int32 MaxDamage = 0;
		if (SkillData != nullptr)
		{
			for (const FSkillPhaseLayer& PhaseLayer : SkillData->mSkillPhaseLayers)
			{
				for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer
					: PhaseLayer.mSkillEffectLayers)
				{
					if (const FSkillEffectLayer_Attack* Attack =
						EffectLayer.GetPtr<FSkillEffectLayer_Attack>())
					{
						MinDamage += Attack->mMinDamage;
						MaxDamage += Attack->mMaxDamage;
					}
				}
			}
		}
		const int32 ExpectedDamage = FMath::Max(
			FMath::RoundToInt((MinDamage + MaxDamage) * 0.5f), 0);
		const int32 ActionPointCost = SkillData != nullptr
			? FMath::Max(SkillData->mRequiredActionPoint, 0) : 0;

		const TArray<FTileIndex> AimableTiles = SkillComponent->GetAimableTiles(
			TileMap, CandidateSkillIndex);
		for (const FTileIndex& AimableTile : AimableTiles)
		{
			const TArray<FTileIndex> EffectTiles = SkillComponent->GetEffectTiles(
				TileMap, CandidateSkillIndex, AimableTile);
			TSet<int32> HitUnitIds;
			int32 EffectiveDamage = 0;
			int32 LethalCount = 0;
			float LowHealthFocus = 0.f;
			float ThreatValue = 0.f;
			for (const FTileIndex& EffectTile : EffectTiles)
			{
				for (UUnitModel* Unit : TileMap->GetActorsOnTile<UUnitModel>(
					EffectTile, ETileLayerFlag::Unit))
				{
					if (IsAutoBattleHostile(Unit)
						&& HitUnitIds.Contains(Unit->GetModelId()) == false)
					{
						HitUnitIds.Add(Unit->GetModelId());
						const UAttributeSetComponentModel* EnemyAttributes =
							Unit->GetAttributeComponentModel();
						const float EnemyHP = EnemyAttributes != nullptr
							? EnemyAttributes->GetAttributeCurrentValue(
								UCombatTargetAttributeSet::GetHPAttribute()) : 0.f;
						const float EnemyMaxHP = EnemyAttributes != nullptr
							? FMath::Max(EnemyAttributes->GetAttributeCurrentValue(
								UCombatTargetAttributeSet::GetMaxHPAttribute()), 1.f) : 1.f;
						const float MissingHealthRatio = FMath::Clamp(
							1.f - (EnemyHP / EnemyMaxHP), 0.f, 1.f);
						LowHealthFocus += MissingHealthRatio;
						if (MaxDamage > 0)
						{
							EffectiveDamage += FMath::Min(
								MaxDamage, FMath::RoundToInt(FMath::Max(EnemyHP, 0.f)));
							if (MaxDamage >= EnemyHP)
							{
								++LethalCount;
							}
						}
						// 먼저 처리할 가치가 높은 적(공격력/위협도가 높은 적)을
						// 약간 우선한다. AttackFactor는 적의 기본 위협도에 해당한다.
						if (EnemyAttributes != nullptr)
						{
							ThreatValue += EnemyAttributes->GetAttributeCurrentValue(
								UCombatTargetAttributeSet::GetAttackFactorAttribute());
						}
					}
				}
			}

			const int32 HitCount = HitUnitIds.Num();
			if (HitCount == 0)
			{
				continue;
			}

			const FTileIndex Origin = PlayerUnit->GetTileTransform().mIndex;
			const int32 AimDistance = FMath::Abs(Origin.mX - AimableTile.mX)
				+ FMath::Abs(Origin.mY - AimableTile.mY);
			// 점수 우선순위: 다수 타격 > 처치 가능성 > 실피 적 마무리
			// > 기대 피해/위협도 > AP 효율 > 가까운 조준점 순서다.
			const float Score = HitCount * 1000.f
				+ LethalCount * 5000.f
				+ EffectiveDamage * 12.f
				+ LowHealthFocus * 350.f
				+ ThreatValue * 8.f
				+ ExpectedDamage * HitCount * 2.f
				- ActionPointCost * 90.f
				- AimDistance * 3.f;
			if (Score > BestScore)
			{
				BestScore = Score;
				SkillIndex = CandidateSkillIndex;
				TargetTile = AimableTile;
			}
		}
	}

	return SkillIndex != INDEX_NONE && TargetTile != FTileIndex::Invalid;
}

bool ACombatGameMode::FindAutoBattleMove(
	UPlayerUnitModel* PlayerUnit, OUT FTileIndex& TargetTile) const
{
	TargetTile = FTileIndex::Invalid;
	if (PlayerUnit == nullptr || PlayerUnit->IsDead())
	{
		return false;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	UAttributeSetComponentModel* Attributes = PlayerUnit->GetAttributeComponentModel();
	if (CombatModel == nullptr || TileMap == nullptr || Attributes == nullptr)
	{
		return false;
	}

	const int32 ActionPoint = FMath::FloorToInt(
		Attributes->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute()));
	if (ActionPoint <= 0)
	{
		return false;
	}

	const FTileIndex Origin = PlayerUnit->GetTileTransform().mIndex;
	TArray<FTileIndex> ReachableTiles = TileMap->GetReachableTiles(
		Origin, ActionPoint, PlayerUnit);
	if (ReachableTiles.IsEmpty())
	{
		return false;
	}

	TArray<UUnitModel*> HostileUnits;
	TSet<FTileIndex> EnemyThreatTiles;
	for (const TObjectPtr<UUnitModel>& Unit : CombatModel->GetUnits())
	{
		if (IsAutoBattleHostile(Unit.Get()) == false)
		{
			continue;
		}

		HostileUnits.Add(Unit.Get());
		const FTileIndex EnemyTile = Unit->GetTileTransform().mIndex;
		EnemyThreatTiles.Add(EnemyTile);

		const UAttributeSetComponentModel* EnemyAttributes =
			Unit->GetAttributeComponentModel();
		const int32 EnemyActionPoint = EnemyAttributes != nullptr
			? FMath::FloorToInt(FMath::Max(EnemyAttributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetActionPointAttribute()), 0.f)) : 0;
		if (EnemyActionPoint > 0)
		{
			for (const FTileIndex& ThreatTile : TileMap->GetReachableTiles(
				EnemyTile, EnemyActionPoint, Unit))
			{
				EnemyThreatTiles.Add(ThreatTile);
			}
		}
	}
	if (HostileUnits.IsEmpty())
	{
		return false;
	}

	int32 CurrentNearestEnemyDistance = MAX_int32;
	for (const UUnitModel* Unit : HostileUnits)
	{
		const FTileIndex EnemyTile = Unit->GetTileTransform().mIndex;
		CurrentNearestEnemyDistance = FMath::Min(
			CurrentNearestEnemyDistance,
			FMath::Abs(Origin.mX - EnemyTile.mX) + FMath::Abs(Origin.mY - EnemyTile.mY));
	}
	if (CurrentNearestEnemyDistance == MAX_int32)
	{
		return false;
	}

	const float CurrentHP = Attributes->GetAttributeCurrentValue(
		UCombatTargetAttributeSet::GetHPAttribute());
	const float MaxHP = FMath::Max(Attributes->GetAttributeCurrentValue(
		UCombatTargetAttributeSet::GetMaxHPAttribute()), 1.f);
	const bool bNeedsSafety = CurrentHP / MaxHP < 0.4f;
	float BestScore = -MAX_flt;
	for (const FTileIndex& CandidateTile : ReachableTiles)
	{
		int32 CandidateNearestEnemyDistance = MAX_int32;
		for (const UUnitModel* Unit : HostileUnits)
		{
			const FTileIndex EnemyTile = Unit->GetTileTransform().mIndex;
			CandidateNearestEnemyDistance = FMath::Min(
				CandidateNearestEnemyDistance,
				FMath::Abs(CandidateTile.mX - EnemyTile.mX)
					+ FMath::Abs(CandidateTile.mY - EnemyTile.mY));
		}

		const int32 PathProgress = FMath::Abs(Origin.mX - CandidateTile.mX)
			+ FMath::Abs(Origin.mY - CandidateTile.mY);
		const bool bThreatened = EnemyThreatTiles.Contains(CandidateTile);
		const bool bImprovesPosition = CandidateNearestEnemyDistance
			< CurrentNearestEnemyDistance;
		if (bImprovesPosition == false && bNeedsSafety == false)
		{
			continue;
		}

		// 평소에는 적에게 접근하면서 위협 칸을 피한다. 체력이 낮으면
		// 접근성보다 생존을 우선하고, 같은 거리라면 안전한 칸을 고른다.
		const float Score = (CurrentNearestEnemyDistance
			- CandidateNearestEnemyDistance) * 1200.f
			+ PathProgress * 4.f
			+ (bThreatened ? (bNeedsSafety ? -5000.f : -700.f)
				: (bNeedsSafety ? 2600.f : 0.f));
		if (Score > BestScore)
		{
			BestScore = Score;
			TargetTile = CandidateTile;
		}
	}

	return TargetTile != FTileIndex::Invalid;
}

bool ACombatGameMode::TapAutoBattleTile(const FTileIndex& Tile)
{
	if (Tile == FTileIndex::Invalid)
	{
		return false;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap == nullptr || Controller == nullptr)
	{
		return false;
	}

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	if (Controller->ProjectWorldLocationToScreen(
		TileMap->TileToWorldLocation(Tile), OUT ScreenPosition) == false)
	{
		return false;
	}

	HandleCombatWorldTouch(ScreenPosition, false);
	return true;
}

void ACombatGameMode::AutoBattlePulse()
{
	if (mAutoBattleEnabled == false)
	{
		return;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	USRPGTurnContext* TurnContext = CombatModel != nullptr
		? CombatModel->GetCurrentTurnContext() : nullptr;
	UPlayerUnitModel* PlayerUnit = TurnContext != nullptr
		? Cast<UPlayerUnitModel>(TurnContext->GetOwner()) : nullptr;
	if (PlayerUnit == nullptr || PlayerUnit->IsDead())
	{
		return;
	}

	if (mAutoBattlePhase == EAutoBattlePhase::WaitingForTurn)
	{
		return;
	}

	if (mAutoBattlePhase == EAutoBattlePhase::WaitingForAction)
	{
		++mAutoBattleWaitingPulseCount;
		if (mAutoBattleWaitingPulseCount > 200)
		{
			UE_LOG(LogCombatGameMode, Warning,
				TEXT("P_RD.AutoBattle: 액션 완료 대기 시간이 초과되어 재계획합니다."));
			ResetAutoBattleAction();
		}
		return;
	}

	if (mAutoBattleUnit.Get() != PlayerUnit
		|| mAutoBattleTurnId != TurnContext->GetTurnId())
	{
		ResetAutoBattleAction();
		mAutoBattleUnit = PlayerUnit;
		mAutoBattleTurnId = TurnContext->GetTurnId();
	}

	USkillComponentModel* SkillComponent = PlayerUnit->GetSkillComponentModel();
	UBoardMovementComponentModel* Movement = PlayerUnit->GetBoardMovementComponentModel();
	if ((SkillComponent != nullptr && SkillComponent->IsAnySkillActivated())
		|| (Movement != nullptr && Movement->IsMoving()))
	{
		return;
	}

	const ECombatBuildPhaseUI BuildPhase = mCombatUIModel != nullptr
		? mCombatUIModel->GetTurnUI().mPhase : ECombatBuildPhaseUI::None;
	if (mAutoBattlePhase == EAutoBattlePhase::SelectingSkill
		|| mAutoBattlePhase == EAutoBattlePhase::ConfirmingSkill
		|| mAutoBattlePhase == EAutoBattlePhase::SelectingMove
		|| mAutoBattlePhase == EAutoBattlePhase::ConfirmingMove)
	{
		if (BuildPhase == ECombatBuildPhaseUI::AimSelection
			&& (mAutoBattlePhase == EAutoBattlePhase::SelectingSkill
				|| mAutoBattlePhase == EAutoBattlePhase::SelectingMove))
		{
			if (TapAutoBattleTile(mAutoBattleTargetTile))
			{
				mAutoBattlePhase = mAutoBattlePhase == EAutoBattlePhase::SelectingSkill
					? EAutoBattlePhase::ConfirmingSkill : EAutoBattlePhase::ConfirmingMove;
			}
			return;
		}

		if (BuildPhase == ECombatBuildPhaseUI::Preview
			&& (mAutoBattlePhase == EAutoBattlePhase::ConfirmingSkill
				|| mAutoBattlePhase == EAutoBattlePhase::ConfirmingMove)
			&& mPendingConfirmTile != FTileIndex::Invalid)
		{
			// 확인 호출 중에 InPlayAction이 동기적으로 시작할 수도 있으므로
			// 먼저 대기 상태를 세팅한다.
			mAutoBattlePhase = EAutoBattlePhase::WaitingForAction;
			mAutoBattleSawInPlayAction = false;
			mAutoBattleWaitingPulseCount = 0;
			ConfirmTargetTile();
		}
		return;
	}

	if (BuildPhase != ECombatBuildPhaseUI::None)
	{
		// 사용자가 열어 둔 수동 프리뷰를 자동전투가 덮어쓰지 않는다.
		return;
	}

	int32 CandidateSkillIndex = INDEX_NONE;
	FTileIndex CandidateSkillTarget = FTileIndex::Invalid;
	if (FindAutoBattleSkill(PlayerUnit, OUT CandidateSkillIndex, OUT CandidateSkillTarget))
	{
		mAutoBattleSkillIndex = CandidateSkillIndex;
		mAutoBattleTargetTile = CandidateSkillTarget;
		if (SelectSkill(CandidateSkillIndex))
		{
			mAutoBattlePhase = EAutoBattlePhase::SelectingSkill;
			UE_LOG(LogCombatGameMode, Verbose,
				TEXT("P_RD.AutoBattle: 유닛 %d 스킬 %d 대상 (%d,%d)"),
				PlayerUnit->GetModelId(), CandidateSkillIndex,
				CandidateSkillTarget.mX, CandidateSkillTarget.mY);
		}
		return;
	}

	FTileIndex CandidateMoveTarget = FTileIndex::Invalid;
	if (FindAutoBattleMove(PlayerUnit, OUT CandidateMoveTarget))
	{
		mAutoBattleTargetTile = CandidateMoveTarget;
		if (SelectMove())
		{
			mAutoBattlePhase = EAutoBattlePhase::SelectingMove;
			UE_LOG(LogCombatGameMode, Verbose,
				TEXT("P_RD.AutoBattle: 유닛 %d 이동 대상 (%d,%d)"),
				PlayerUnit->GetModelId(), CandidateMoveTarget.mX, CandidateMoveTarget.mY);
		}
		return;
	}

	if (EndTurn())
	{
		mAutoBattlePhase = EAutoBattlePhase::WaitingForTurn;
		UE_LOG(LogCombatGameMode, Verbose,
			TEXT("P_RD.AutoBattle: 유닛 %d 턴 종료"), PlayerUnit->GetModelId());
	}
}

#endif

void ACombatGameMode::HandleAutoBattleToggleRequested()
{
#if !UE_BUILD_SHIPPING
	SetAutoBattleEnabled(!mAutoBattleEnabled);
#endif
}

void ACombatGameMode::CancelPendingActionEndAfterCameraReturn()
{
	if (mPendingActionEndAfterCameraReturnHandle.IsValid() == false)
	{
		return;
	}

	/*if (UWorldCameraModel* WorldCameraModel =
		GetWorldSubsystemModel<UWorldCameraModel>(this))
	{
		WorldCameraModel->OnMainCameraReturned.Remove(
			mPendingActionEndAfterCameraReturnHandle);
	}*/
	mPendingActionEndAfterCameraReturnHandle.Reset();
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
		// 확정 단추가 재탭할 겨냥 칸. Preview 를 벗어나면 비워, 낡은 칸을
		// 재탭해 경로/겨냥만 무르는 일이 없게 한다.
		mPendingConfirmTile = (Phase == ESRPGSkillBuildPhase::Preview && Action != nullptr)
			? Action->GetSelectedTileIndex() : FTileIndex::Invalid;
		PushSkillBuildUIData(Phase);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnPostSimulateSkillAction.AddWeakLambda(this, [this](const TArray<FSRPGTurnEventLog>& EventLogs) {
		PushSimulationPreviewUIData(EventLogs);
		});
	SkillSelectCommand.GetMutable<FSRPGSkillSelectCommand>().OnCancelSimulateSkillAction.AddWeakLambda(this, [this]() {
		// 무름(취소)은 실전 표시를 건드리지 않는다 — 미리보기만 통째로 버린다.
		mCombatUIModel->GetSimulationPreviewUIModel()->ClearPreview();
		});

	return CommandRouterModel->SummitCommand(SkillSelectCommand);
}

bool ACombatGameMode::SelectMove()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> MoveSelectCommand;
	MoveSelectCommand.InitializeAs<FSRPGMoveSelectCommand>();
	MoveSelectCommand.GetMutable<FSRPGMoveSelectCommand>().OnChangeMoveBuildPhase.AddWeakLambda(this, [this](const USRPGMoveBuildAction* Action, ESRPGMoveBuildPhase Phase) {
		// 확정 단추가 재탭할 경로 목적지. UI Target(살펴보기)과 별개로 여기서
		// 챙긴다 -- 빈 칸에 경로를 그으면 Target 은 갱신되지 않는다(0810).
		mPendingConfirmTile = (Phase == ESRPGMoveBuildPhase::Preview && Action != nullptr)
			? Action->GetLastWaypoint() : FTileIndex::Invalid;
		PushMoveBuildUIData(Action, Phase);
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

void ACombatGameMode::HandleCombatCommand(ECombatInputType Type, int32 IntPayload)
{
#if !UE_BUILD_SHIPPING
	// 사용자가 수동 명령을 내리면 자동전투를 잠시 끈다. 현재 진행 중인
	// 액션은 끝까지 두고, 다음 행동부터 사람이 이어받게 한다.
	if (mAutoBattleEnabled)
	{
		SetAutoBattleEnabled(false);
	}
#endif

	switch (Type)
	{
	// 스킬/이동을 골라도 켜 둔 위협 범위는 걷지 않는다. "저기까지 오는데
	// 어디로 피하지"를 보면서 고르라고 켜 둔 것이다 -- 켜고 끄는 것은 적
	// 타일 탭만 한다. 턴이 끝나면 적이 움직여 칠이 낡으므로 그때는 걷는다.
	case ECombatInputType::SelectSkill:
		ClearSkillDetailPreview();
		SelectSkill(IntPayload);
		break;
	case ECombatInputType::Move:
		ClearSkillDetailPreview();
		SelectMove();
		break;
	case ECombatInputType::EndTurn:
		ClearSkillDetailPreview();
		ClearThreatRangeView();
		EndTurn();
		break;
	case ECombatInputType::LongPressSkill:
		// 길게 누른 스킬의 상세 정보를 UIModel에 채운다.
		{
			UPlayerUnitModel* UnitModel = FindPartyUnitModel(mInspectedUnitId);
			if (UnitModel == nullptr)
			{
				UnitModel = GetTurnPlayerUnitModel();
			}
			if (UnitModel == nullptr)
			{
				UnitModel = GetPlayerUnitModel(0);
			}
			ShowSkillDetailPreview(UnitModel, IntPayload);
		}
		PushSkillDetailUIData(IntPayload);
		break;
	case ECombatInputType::InspectUnitSkill:
		// 상세창의 스킬 칸을 탭했다. 기준은 그 상세창에 뜬 유닛이다.
		ShowSkillDetailPreview(mDetailUnitModel.Get(), IntPayload);
		PushUnitSkillDetailUIData(IntPayload);
		break;
	case ECombatInputType::LongPressUnit:
		// UI 가 상세 패널을 닫았다는 신호로 INDEX_NONE 을 보낸다. 패널과 함께
		// 뜬 위협 범위 칠도 같이 걷는다. 유닛 상세 요청 자체는 월드 롱프레스
		// (HandleCombatWorldTouch)가 트레이스로 처리하므로 여기서는 닫기만 맡는다.
		if (IntPayload == INDEX_NONE)
		{
			ClearSkillDetailPreview();
			ClearThreatRangeView();
		}
		break;
	case ECombatInputType::InspectUnit:
		// 유닛 하나를 살펴보겠다는 청 -- 그 유닛의 스킬로 카드를 갈아 끼우고,
		// 같은 id 로 PR457 유닛 상세도 함께 내린다. 아군(용병 탭)만이 아니라
		// **몬스터 탭도 이 길을 쓴다**: 아군만 찾으면 몬스터 탭 스킬 칸이
		// 영영 빈다(0807 감사).
		mInspectedUnitId = IntPayload;
		PushSkillUIData();
		PushBoardActorDetailUIData(FindUnitModelById(IntPayload));
		break;
	case ECombatInputType::FocusUnit:
		// 스킬 단추를 눌렀다. 그 스킬을 쓰는 유닛을 화면 가운데로 데려온다.
		FocusCameraOnUnit(IntPayload);
		break;
	case ECombatInputType::Confirm:
		// 겨냥해 둔 칸을 그대로 다시 누른다. 판에서 두 번째 탭이 확정인데,
		// 화면 단추로도 되게 하려면 그 탭을 여기서 대신 놓아 준다 -- 확정
		// 판정을 UI 가 흉내 내면 규칙이 두 곳에 생긴다.
		ConfirmTargetTile();
		break;
	case ECombatInputType::Cancel:
		// 현재 빌드와 같은 명령을 다시 보내 취소한다. 이동 중 선택 스킬 index를
		// 사용하면 엉뚱한 스킬이 열릴 수 있으므로 pending action 종류를 따른다.
		if (mCombatUIModel != nullptr
			&& mCombatUIModel->GetTurnUI().mPhase != ECombatBuildPhaseUI::None)
		{
			const FCombatPendingActionUI& PendingAction = mCombatUIModel->GetPendingAction();
			if (PendingAction.mType == ECombatPendingActionType::Move)
			{
				SelectMove();
			}
			else if (PendingAction.mType == ECombatPendingActionType::Skill)
			{
				SelectSkill(mCombatUIModel->GetSelectedSkillIndex());
			}
		}
		// 겨냥을 풀고 화면도 겨냥하기 전으로 되돌린다.
		ClearCombatTargetUIData();
		break;
	}
}

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

/**
 * @brief 겨냥해 둔 칸을 그대로 다시 누른다.
 *
 * @details
 * 판에서 두 번째 탭이 확정이다. 화면 아래 단추로도 되게 하려면 그 탭을
 * 대신 놓아 주면 된다 -- 확정 판정을 UI 나 여기서 흉내 내면 규칙이 두 곳에
 * 생긴다. 칸을 화면 좌표로 되돌려 같은 길로 흘려보낸다.
 */
void ACombatGameMode::ConfirmTargetTile()
{
	// 재탭할 칸은 빌드 프리뷰가 챙겨 둔 칸이 우선이다. UI Target(살펴보기
	// 대상)은 빈 칸으로 경로를 그으면 갱신되지 않아, 그걸 재탭하면 확정이
	// 아니라 무르기가 된다(0810).
	FTileIndex ConfirmTile = mPendingConfirmTile;
	if (ConfirmTile == FTileIndex::Invalid)
	{
		if (mCombatUIModel == nullptr || mCombatUIModel->GetTarget().mIsValid == false)
		{
			return;
		}
		ConfirmTile = mCombatUIModel->GetTarget().mTile;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (TileMap == nullptr || Controller == nullptr)
	{
		return;
	}

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const FVector World = TileMap->TileToWorldLocation(ConfirmTile);
	if (Controller->ProjectWorldLocationToScreen(World, OUT ScreenPosition) == false)
	{
		return;
	}
	ResolveWorldTouchEvent(ScreenPosition);
}

void ACombatGameMode::HandleRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	if (ClaimCombatReward(ClaimKind, ChoiceIndex) && mRewardUIModel != nullptr)
	{
		mRewardUIModel->ConfirmRewardClaim(ClaimKind, ChoiceIndex);
	}
}

void ACombatGameMode::HandleRewardSelectionRequested(
	const FPrimaryAssetId RewardId)
{
	if (ClaimCombatSelectedArtifact(RewardId) && mRewardUIModel != nullptr)
	{
		mRewardUIModel->ConfirmSelectedReward(RewardId);
	}
}

bool ACombatGameMode::ClaimCombatSelectedArtifact(
	const FPrimaryAssetId& RewardId)
{
	if (mRewardSelectionClaimed || RewardId.IsValid() == false)
	{
		return false;
	}

	URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || mRewardUIModel == nullptr)
	{
		return false;
	}

	const FRoom& CurrentRoom = RunPersistData->GetCurrentRoom();
	TArray<FPrimaryAssetId> CandidateIds;
	switch (CurrentRoom.mType)
	{
	case ERoomType::EliteMonster:
		CandidateIds = static_cast<const FEliteMonsterRoom&>(CurrentRoom)
			.GetEffectiveRewardArtifactDataIds();
		break;
	case ERoomType::BossMonster:
		CandidateIds = static_cast<const FBossMonsterRoom&>(CurrentRoom)
			.GetEffectiveRewardArtifactDataIds();
		break;
	default:
		return false;
	}

	FPrimaryAssetId SelectedId;
	if (ArtifactRewardPolicy::TrySelectOne(
		CandidateIds, RewardId, OUT SelectedId) == false)
	{
		return false;
	}

	UPartyModel* PartyModel = GetPartyModel();
	UPartyArtifactComponentModel* ArtifactModel = PartyModel != nullptr
		? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	if (ArtifactModel == nullptr || ArtifactModel->AddArtifact(SelectedId) == false)
	{
		return false;
	}

	mRewardSelectionClaimed = true;
	mSelectedRewardArtifactId = SelectedId;
	return true;
}

bool ACombatGameMode::ClaimCombatReward(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr)
	{
		return false;
	}

	const FRoom& CurrentRoomData = RunPersistData->GetCurrentRoom();
	const FMonsterRoom* CurrentRoom = GetMonsterRewardRoom(CurrentRoomData);

	if (CurrentRoom == nullptr)
	{
		return false;
	}

	if (ClaimKind == ERewardClaimKind::Gold)
	{
		if (mGoldRewardClaimed || CurrentRoom->mRewardMoney <= 0)
		{
			return false;
		}

		UPartyModel* PartyModel = GetPartyModel();
		if (PartyModel == nullptr)
		{
			return false;
		}

		UAttributeSetComponentModel* AttributeSetComponentModel = PartyModel->GetAttributeComponentModel();
		if (AttributeSetComponentModel == nullptr)
		{
			return false;
		}

		AttributeSetComponentModel->ApplyModToAttribute(UPartyAttributeSet::GetMoneyAttribute(), ETacticalModOp::AddBase, StaticCast<float>(CurrentRoom->mRewardMoney));
		mGoldRewardClaimed = true;
		PushPlayerMetaUIData();
		return true;
	}

	if (ClaimKind == ERewardClaimKind::Exp)
	{
		if (mExpRewardClaimed || CurrentRoom->mRewardExp <= 0)
		{
			return false;
		}

		const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnitModels = GetPlayerUnitModels();
		bool bGrantedToAnyPlayer = false;
		for (UPlayerUnitModel* PlayerUnitModel : PlayerUnitModels)
		{
			if (PlayerUnitModel == nullptr)
			{
				continue;
			}

			UAttributeSetComponentModel* AttributeSetComponentModel = PlayerUnitModel->GetAttributeComponentModel();
			if (AttributeSetComponentModel == nullptr)
			{
				continue;
			}

			AttributeSetComponentModel->ApplyModToAttribute(UPlayerUnitAttributeSet::GetExpAttribute(), ETacticalModOp::AddBase, StaticCast<float>(CurrentRoom->mRewardExp));
			bGrantedToAnyPlayer = true;
		}

		if (bGrantedToAnyPlayer == false)
		{
			return false;
		}

		mExpRewardClaimed = true;
		PushPlayerMetaUIData();
		return true;
	}

	if (ClaimKind != ERewardClaimKind::Choice
		|| ChoiceIndex == INDEX_NONE
		|| mClaimedRewardChoiceIndices.Contains(ChoiceIndex)
		|| mRewardUIModel == nullptr)
	{
		return false;
	}

	const FRewardChoiceUI* FoundChoice = mRewardUIModel->GetRewardChoices().FindByPredicate([ChoiceIndex](const FRewardChoiceUI& Choice)
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
	case ERewardChoiceKind::Artifact:
		bClaimed = ClaimCombatSelectedArtifact(FoundChoice->mSourceAssetId);
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
		mClaimedRewardChoiceIndices.Add(ChoiceIndex);
	}
	return bClaimed;
}

void ACombatGameMode::HandleAbandonRun()
{
	if (mSettingsRunActionPending)
	{
		// A second click does not represent a failed operation. Keep the first
		// asynchronous request authoritative and leave the UI locked until it ends.
		return;
	}

	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<USaveGameSubsystem>() : nullptr;
	if (SaveGameSubsystem == nullptr)
	{
		if (mCombatUIModel != nullptr)
		{
			mCombatUIModel->NotifyAbandonRunCompleted(false);
		}
		return;
	}

	mSettingsRunActionPending = true;
	SaveGameSubsystem->SaveOptionAsync(FAsyncSaveGameToSlotDelegate::CreateWeakLambda(
		this,
		[this](const FString& SlotName, const int32 UserIndex, const bool bSaveSucceeded)
		{
			const bool bAbandonStarted = bSaveSucceeded && AbandonRunFromRoom();
			if (!bAbandonStarted)
			{
				mSettingsRunActionPending = false;
			}
			if (mCombatUIModel != nullptr)
			{
				mCombatUIModel->NotifyAbandonRunCompleted(bAbandonStarted);
			}
		}));
}

void ACombatGameMode::HandleSaveAndExitRun()
{
	if (mSettingsRunActionPending)
	{
		// Do not publish a false completion for a duplicate click: that would unlock
		// the settings buttons while the original save/transition is still running.
		return;
	}

	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<USaveGameSubsystem>() : nullptr;
	if (SaveGameSubsystem == nullptr)
	{
		if (mCombatUIModel != nullptr)
		{
			mCombatUIModel->NotifySaveAndExitCompleted(false);
		}
		return;
	}

	mSettingsRunActionPending = true;
	SaveGameSubsystem->SaveOptionAsync(FAsyncSaveGameToSlotDelegate::CreateWeakLambda(
		this,
		[this](const FString& SlotName, const int32 UserIndex, const bool bSaveSucceeded)
		{
			if (!bSaveSucceeded)
			{
				mSettingsRunActionPending = false;
				if (mCombatUIModel != nullptr)
				{
					mCombatUIModel->NotifySaveAndExitCompleted(false);
				}
				return;
			}

			SaveAndExitRunFromRoomAsync(FOnRoomSaveAndExitComplete::CreateWeakLambda(
				this, [this](const bool bSuccess)
				{
					if (!bSuccess)
					{
						mSettingsRunActionPending = false;
					}
					if (mCombatUIModel != nullptr)
					{
						mCombatUIModel->NotifySaveAndExitCompleted(bSuccess);
					}
				}));
		}));
}

void ACombatGameMode::HandleChangeFocusScreenAnchor(const FVector2D& ScreenRatio)
{
	ACombatCameraPawn* CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	if (CameraPawn == nullptr)
	{
		return;
	}
	UCameraMovementComponent* CameraMovement = CameraPawn->GetCameraMovementComponent();
	if (CameraMovement == nullptr)
	{
		return;
	}

	CameraMovement->SetViewportOffset(ScreenRatio);
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
	// 톡 친 칸을 UI 에 알린다. 어느 타일인지는 트레이스한 쪽만 안다.
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().OnSelectTargetTile.AddWeakLambda(this,
		[this](const FTileIndex& Tile, AActor* HitActor) {
			PushCombatTargetUIData(Tile, HitActor);
		});

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
	WorldTraceActionCommand.GetMutable<FSRPGWorldTraceCommand>().OnShowTargetDetailPanelUI.AddWeakLambda(this, [this](IBoardSelectionTargetView* Target) {
		PushCombatTargetDetailUIData(Target);
		ShowThreatRangeForTarget(Target);
		});

	return CommandRouterModel->SummitCommand(WorldTraceActionCommand);
}

void ACombatGameMode::ShowThreatRangeForTarget(IBoardSelectionTargetView* Target) const
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap == nullptr)
	{
		return;
	}

	IObjectView* ObjectView = Cast<IObjectView>(Target);
	UUnitModel* UnitModel = ObjectView != nullptr ? ObjectView->GetModel<UUnitModel>() : nullptr;
	if (UnitModel == nullptr || UnitModel->IsPlayerUnitModel() == true)
	{
		// 아군/장애물에는 위협 범위가 없다. 이전 적의 칠만 지운다.
		TileMap->ClearThreatRange();
		return;
	}

	USkillComponentModel* SkillComponentModel = UnitModel->GetSkillComponentModel();
	UAttributeSetComponentModel* AttributeSetComponentModel = UnitModel->GetAttributeComponentModel();
	if (SkillComponentModel == nullptr || AttributeSetComponentModel == nullptr)
	{
		TileMap->ClearThreatRange();
		return;
	}

	// 적 플래너와 같은 규약: 장착돼 있고 쿨다운이 아닌 슬롯만 데이터 채움
	const TArray<FSkillEntry>& Skills = SkillComponentModel->GetSkills();
	TArray<const UStaticUnitSkillData*> SkillDatas;
	SkillDatas.Init(nullptr, Skills.Num());
	for (int32 Index = 0; Index < Skills.Num(); ++Index)
	{
		if (Skills[Index].IsValid() == true && SkillComponentModel->IsCooldown(Index) == false)
		{
			SkillDatas[Index] = StaticCast<const UStaticUnitSkillData*>(Skills[Index].mData);
		}
	}

	const int32 ActionPoint = FMath::Max(
		AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetRechargeActionPointAttribute()),
		0
	);

	TArray<FTileIndex> MoveTiles;
	TArray<FTileIndex> AttackTiles;
	TileMap->GetThreatRanges(UnitModel->GetTileTransform().mIndex, ActionPoint, SkillDatas, UnitModel, OUT MoveTiles, OUT AttackTiles);
	TileMap->SetThreatRange(MoveTiles, AttackTiles);
}

void ACombatGameMode::ClearThreatRangeView() const
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap != nullptr)
	{
		TileMap->ClearThreatRange();
	}
}

void ACombatGameMode::OnRegisterUnit(UUnitModel* Unit)
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Unit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	if (CombatUIDebugFixture::ShouldMutateActualHPOne())
	{
		AttributeSetComponentModel->SetAttributeBaseValue(
			UUnitAttributeSet::GetHPAttribute(), 1.f);
	}

	// 각 속성이 변경될 때마다 OnRefreshUnitUI를 브로드캐스트하도록 바인딩
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetActionPointAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		PushSkillUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetLastRechargedActionPointAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetLastRechargedSpeedPointAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});

	// 상태 이상 태그 변경 시에도 UI 갱신 바인딩
	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, ETacticalTagEventType::NewOrRemoved).AddWeakLambda(this, [this](const FGameplayTag Tag, int32 Count) {
		PushUnitUIData();
		});

	USkillComponentModel* SkillComponentModel = Unit->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("스킬 컴포넌트 nullptr"));

	SkillComponentModel->OnPrePlaySkillUI.AddWeakLambda(this, [this](const FActiveSkillContext& Context, const UStaticSkillData* SkillData, TSharedPtr<FPresentationBarrier> SkillPlayBarrier) {
		if (mCombatUIModel == nullptr)
		{
			UE_LOG(LogCombatGameMode, Warning,
				TEXT("Pre-skill cut-in skipped because CombatUIModel is unavailable."));
			SkillPlayBarrier.Reset();
			return;
		}

		FCombatSkillCutInRequest Request;
		Request.SkillIndex = Context.mSkillIndex;
		if (UUnitModel* CasterUnit = Cast<UUnitModel>(Context.mInstigator.GetObject()))
		{
			Request.bIsPlayerCaster = CasterUnit->IsPlayerUnitModel();
			Request.UnitId = CasterUnit->GetModelId();
			Request.ShortCut = CasterUnit->GetBoardActorShortCut();
			Request.Portrait = CasterUnit->GetBoardActorPortrait();
			Request.ViewActor = CasterUnit->GetView<AActor>();
		}

		mCombatUIModel->NotifyPrePlaySkillCutIn(Request, MoveTemp(SkillPlayBarrier));
		});

	PushTurnUIData();
	PushUnitUIData();
}

void ACombatGameMode::OnUnregisterUnit(UUnitModel* Unit)
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Unit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	Unit->OnEndMoveStep.RemoveAll(this);

	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetActionPointAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).RemoveAll(this);

	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, ETacticalTagEventType::NewOrRemoved).RemoveAll(this);

	USkillComponentModel* SkillComponentModel = Unit->GetSkillComponentModel();
	checkf(SkillComponentModel != nullptr, TEXT("스킬 컴포넌트 nullptr"));

	SkillComponentModel->OnPrePlaySkillUI.RemoveAll(this);

	if (Unit->IsPlayerUnitModel() == false)
	{
		++mDefeatedMonsterCount;
	}

	PushTurnUIData();
	PushUnitUIData();
}

void ACombatGameMode::PushCombatResultUIData(ESRPGCombatResult Result) const
{
	FCombatResultUI CombatResultUIData;
	CombatResultUIData.mIsWin = Result == ESRPGCombatResult::PlayerWin;
	CombatResultUIData.mLocationName = NSLOCTEXT("CombatGameMode", "CurrentCombatArea", "현재 전투 지역");
	CombatResultUIData.mRound = mCombatUIModel != nullptr ? mCombatUIModel->GetTurnUI().mRound : 0;
	CombatResultUIData.mDefeatedMonsterCount = mDefeatedMonsterCount;
	CombatResultUIData.mGoldGained = 0;
	CombatResultUIData.mExpGained = 0;
	CombatResultUIData.mPartyPortraits = mCombatStartPartyPortraits;
	mCombatUIModel->SetCombatResultUI(CombatResultUIData);
}

void ACombatGameMode::PushTurnUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	const TArray<TObjectPtr<USRPGTurnContext>> TurnContexts = CombatModel->GetOrderedTurnContexts();
	// 라운드 시작 UI는 새 턴 후보가 ApplyOrderedTurnCandidates 로 채워지기 전에
	// 방송된다. 이 구간에서 바로 반환하면 TurnUI.mRound 만 이전 값으로 남아
	// 실제 모델은 3라운드인데 배너/좌상단 표시는 ROUND 2를 그리게 된다.
	// 턴이 비어 있어도 모델이 이미 올린 라운드 번호는 먼저 UI 스냅샷에 내린다.
	if (TurnContexts.IsEmpty() == true)
	{
		FTurnUI RoundTransitionUI = mCombatUIModel->GetTurnUI();
		RoundTransitionUI.mCurrentUnitId = INDEX_NONE;
		RoundTransitionUI.mRound = CombatModel->GetRoundCount();
		RoundTransitionUI.mCurrentRoundRemainingTurnCount = 0;
		RoundTransitionUI.mTurnOrderUnitIds.Reset();
		RoundTransitionUI.mNextRoundUnitIds.Reset();
		RoundTransitionUI.mNextRoundOffset = 1;
		mCombatUIModel->SetTurnUI(RoundTransitionUI);
		return;
	}

	FTurnUI TurnUI;
	TurnUI.mCurrentUnitId = TurnContexts[0]->GetOwner()->GetModelId();
	TurnUI.mPhase = mCombatUIModel->GetTurnUI().mPhase;
	TurnUI.mRound = CombatModel->GetRoundCount();
	TurnUI.mCurrentRoundRemainingTurnCount = CombatModel->GetTurnContextCount();
	for (const TObjectPtr<USRPGTurnContext>& TurnContext : TurnContexts)
	{
		TurnUI.mTurnOrderUnitIds.Add(TurnContext->GetOwner()->GetModelId());
	}

	TArray<FSRPGTurnCandidate> ValidTurnCandidates;
	int32 ValidRoundOffset = INDEX_NONE;
	CombatModel->GetValidRoundAndOrderedTurnCandidates(OUT ValidTurnCandidates, OUT ValidRoundOffset);
	for (const FSRPGTurnCandidate& ValidTurnCandidate : ValidTurnCandidates)
	{
		TurnUI.mNextRoundUnitIds.Add(ValidTurnCandidate.mOwner->GetModelId());
	}
	TurnUI.mNextRoundOffset = ValidRoundOffset + 1;
	mCombatUIModel->SetTurnUI(TurnUI);
}

void ACombatGameMode::PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	switch (Phase)
	{
	case ESRPGSkillBuildPhase::None:
	case ESRPGSkillBuildPhase::Build:
		mCombatUIModel->SetPendingAction(FCombatPendingActionUI());
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

void ACombatGameMode::PushMoveBuildUIData(const USRPGMoveBuildAction* Action, ESRPGMoveBuildPhase Phase) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	switch (Phase)
	{
	case ESRPGMoveBuildPhase::None:
	case ESRPGMoveBuildPhase::Build:
		mCombatUIModel->SetPendingAction(FCombatPendingActionUI());
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::None);
		break;
	case ESRPGMoveBuildPhase::DestSelection:
		{
			FCombatPendingActionUI PendingAction;
			PendingAction.mType = ECombatPendingActionType::Move;
			mCombatUIModel->SetPendingAction(PendingAction);
		}
		mCombatUIModel->SetBuildPhase(ECombatBuildPhaseUI::AimSelection);
		break;
	case ESRPGMoveBuildPhase::Preview:
		{
			FCombatPendingActionUI PendingAction;
			PendingAction.mType = ECombatPendingActionType::Move;
			PendingAction.mActionPointCost = Action != nullptr
				? Action->GetPlannedMoveCost() : 0;
			mCombatUIModel->SetPendingAction(PendingAction);
		}
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

	int32 PlayerFixtureIndex = 0;
	int32 EnemyFixtureIndex = 0;
	for (int32 i = 0; i < UnitModelNum; ++i)
	{
		const TObjectPtr<UUnitModel>& UnitModel = UnitModels[i];
		FUnitUI& UnitUIData = UnitUIDatas[i];

		UAttributeSetComponentModel* AttributeSetComponentModel = UnitModel->GetAttributeComponentModel();
		checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

		UnitUIData.mIsPlayer = UnitModel->IsPlayerUnitModel();
		UnitUIData.mUnitId = UnitModel->GetModelId();
		UnitUIData.mName = UnitModel->GetBoardActorDisplayName();      // 아군 칸·턴 순서 칩이 읽는다. 안 채우면 빈칸으로 나온다.
		UnitUIData.mPortrait = UnitModel->GetBoardActorPortrait();
		if (UTexture2D* MarchboundHero =
			ResolveMarchboundMercenaryPortrait(UnitModel, true))
		{
			UnitUIData.mPortrait = MarchboundHero;
		}
		// 큰 카드의 972x1619 세로 초상을 턴 칩에 억지로 눌러 넣지 않는다.
		// DA mIcon에는 256x256 HeadV2 얼굴판을 두며, 아직 아이콘이 없는
		// 신규/임시 유닛만 기존 초상으로 안전하게 폴백한다.
		UnitUIData.mTurnPortrait = ResolveMarchboundMercenaryPortrait(
			UnitModel, false);
		if (UnitUIData.mTurnPortrait == nullptr)
		{
			UnitUIData.mTurnPortrait = UnitModel->GetBoardActorIcon();
		}
		if (UnitUIData.mTurnPortrait == nullptr)
		{
			UnitUIData.mTurnPortrait = ResolveTurnPortraitFallback(UnitModel);
		}
		if (UnitUIData.mTurnPortrait == nullptr)
		{
			UnitUIData.mTurnPortrait = UnitUIData.mPortrait;
		}
		UnitUIData.mTile = UnitModel->GetTileTransform().mIndex;
		UnitUIData.mLevel = UnitModel->GetBoardActorLevel();
		UnitUIData.mHP = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetHPAttribute());
		UnitUIData.mMaxHP = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetMaxHPAttribute());
		// Android 실기 UI 검수용 HP=1은 DTO에만 적용한다. 실제 Attribute와
		// 이를 추적하는 RunPersistData는 건드리지 않아 전투/세이브 결과가 바뀌지 않는다.
		CombatUIDebugFixture::ApplyDisplayHPOne(UnitModel->IsDead() == false, OUT UnitUIData);
		// 턴바 밑 "속도"는 라운드마다 충전되는 고유 속도(RechargeSpeedPoint)를 보여 준다.
		// SpeedPoint는 라운드 진행 중 소비/누적되는 현재값이라 표시 기준으로 쓰지 않는다.
		UnitUIData.mSpeedPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetLastRechargedSpeedPointAttribute());
		UnitUIData.mDefensePoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefenseAttribute());
		UnitUIData.mMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
		UnitUIData.mMaxMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetLastRechargedActionPointAttribute());
		// 용병 탭 상세의 "AP x / y" 표기용. Mock이 아닌 실전에서도 같은 자원을 보여 준다.
		UnitUIData.mActionPoints = FMath::RoundToInt(UnitUIData.mMovementPoint);
		UnitUIData.mMaxActionPoints = FMath::RoundToInt(UnitUIData.mMaxMovementPoint);

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

		const int32 FixtureSideIndex = UnitUIData.mIsPlayer
			? PlayerFixtureIndex++ : EnemyFixtureIndex++;
		CombatUIDebugFixture::AppendStatuses(UnitUIData.mIsPlayer, FixtureSideIndex,
			OUT UnitUIData.mStatusEffects);

		// 적 요약판의 "다음 스킬" 소켓: 장착 스킬 중 첫 유효 슬롯 아이콘을 대표로 건다.
		if (UnitUIData.mIsPlayer == false)
		{
			if (USkillComponentModel* SkillComponentModel = UnitModel->GetSkillComponentModel())
			{
				const TArray<FSkillEntry>& SkillEntries = SkillComponentModel->GetSkills();
				for (int32 SkillIndex = 0; SkillIndex < SkillEntries.Num(); ++SkillIndex)
				{
					const FSkillEntry& SkillEntry = SkillEntries[SkillIndex];
					const UStaticUnitSkillData* SkillData = SkillEntry.IsValid()
						? StaticCast<const UStaticUnitSkillData*>(SkillEntry.mData.Get()) : nullptr;
					if (SkillData != nullptr && SkillData->mIcon.IsNull() == false)
					{
						UnitUIData.mNextSkillIcon = SkillData->mIcon.LoadSynchronous();
						UnitUIData.mNextSkillIndex = SkillIndex;   // 소켓 클릭 → 상세용
						break;
					}
				}
			}
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

/**
 * @brief 지금 겨냥한 자리에 이 스킬을 쓸 수 있나.
 *
 * @details
 * 두 가지만 본다. 행동력이 남았는지, 겨냥한 칸이 사거리 안인지.
 *
 * 칸 사이 거리는 가로세로 중 먼 쪽으로 잰다. 대각선을 한 칸으로 치는 판이라
 * 그렇다. 조준 모양(십자/사각)까지 따지는 것은 조준 단계가 할 일이고 여기서는
 * 카드를 켤지 끌지만 정한다 -- 여기서 정밀하게 세면 조준 단계와 두 벌이 되고
 * 그 둘은 언젠가 어긋난다.
 *
 * 겨냥한 자리가 없으면 사거리는 안 본다. 아직 아무 데도 안 찍은 상태다.
 * @param PlayerUnitModel 지금 차례인 유닛
 * @param StaticSkillData 검사할 스킬
 * @return 카드를 켜도 되면 true
 */
bool ACombatGameMode::IsSkillUsableOnTarget(const UPlayerUnitModel* PlayerUnitModel,
	const UStaticSkillData& StaticSkillData) const
{
	if (PlayerUnitModel == nullptr)
	{
		return false;
	}

	const FCombatTargetUI& Target = mCombatUIModel != nullptr
		? mCombatUIModel->GetTarget() : FCombatTargetUI();
	if (Target.mIsValid == false)
	{
		return true;
	}

	/*
	 * 조준 중이 아닐 때의 겨냥은 "살펴보기"다(적 안내판·위협 범위 표시용).
	 *
	 * 아직 아무 스킬도 고르지 않았는데 그 적까지의 거리로 카드를 잠그면,
	 * 멀리 있는 적을 확인만 해도 스킬 전부가 잠긴 것처럼 보인다 -- 위협
	 * 범위를 보면서 스킬을 고르라고 만든 기능이 스킬 선택을 막았다.
	 * 사거리 판정은 실제로 조준에 들어간 뒤에만 한다.
	 */
	if (mCombatUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::None)
	{
		return true;
	}

	const FTileIndex& Here = PlayerUnitModel->GetTileTransform().mIndex;
	const int32 Distance = FMath::Max(
		FMath::Abs(Target.mTile.mX - Here.mX),
		FMath::Abs(Target.mTile.mY - Here.mY));
	return Distance <= StaticSkillData.mAimRange;
}

/**
 * @brief 파티에서 이 id 의 유닛을 찾는다.
 *
 * @details
 * UI 는 액터도 모델도 모르고 FUnitUI.mUnitId 만 안다. 그 id 로 되짚는 자리가
 * 여기다.
 * @param UnitId 찾을 유닛. INDEX_NONE 이면 안 찾는다
 * @return 없으면 nullptr
 */
/**
 * @brief 지금 차례인 아군.
 * @return 적 차례거나 없으면 nullptr
 */
UPlayerUnitModel* ACombatGameMode::GetTurnPlayerUnitModel() const
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	const USRPGTurnContext* TurnContext = CombatModel != nullptr
		? CombatModel->GetCurrentTurnContext() : nullptr;
	UUnitModel* TurnUnit = TurnContext != nullptr ? TurnContext->GetOwner() : nullptr;
	if (TurnUnit == nullptr || TurnUnit->IsPlayerUnitModel() == false)
	{
		return nullptr;
	}
	return FindPartyUnitModel(TurnUnit->GetModelId());
}

/** @brief 아군/적 가리지 않고 id 로 유닛 모델을 찾는다. 없으면 nullptr. */
UUnitModel* ACombatGameMode::FindUnitModelById(const int32 UnitId) const
{
	if (UnitId == INDEX_NONE)
	{
		return nullptr;
	}
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatModel == nullptr)
	{
		return nullptr;
	}
	for (const TObjectPtr<UUnitModel>& UnitModel : CombatModel->GetUnits())
	{
		if (UnitModel != nullptr && UnitModel->GetModelId() == UnitId)
		{
			return UnitModel;
		}
	}
	return nullptr;
}

UPlayerUnitModel* ACombatGameMode::FindPartyUnitModel(int32 UnitId) const
{
	if (UnitId == INDEX_NONE)
	{
		return nullptr;
	}

	for (UPlayerUnitModel* PartyUnitModel : GetPlayerUnitModels())
	{
		if (PartyUnitModel != nullptr && PartyUnitModel->GetModelId() == UnitId)
		{
			return PartyUnitModel;
		}
	}
	return nullptr;
}

void ACombatGameMode::FocusCameraOnUnit(const int32 UnitId) const
{
	if (UnitId == INDEX_NONE)
	{
		return;
	}

	ACombatCameraPawn* CameraPawn = UCameraFunctionLibrary::GetMainCameraPawn(this);
	if (CameraPawn == nullptr)
	{
		return;
	}
	UCameraMovementComponent* CameraMovement = CameraPawn->GetCameraMovementComponent();
	if (CameraMovement == nullptr)
	{
		return;
	}

	UUnitModel* UnitModel = FindUnitModelById(UnitId);
	AActor* ViewActor = UnitModel != nullptr ? UnitModel->GetView<AActor>() : nullptr;
	if (ViewActor == nullptr)
	{
		return;
	}

	CameraMovement->MoveToWorldPosition(ViewActor->GetActorLocation(), false);
}

void ACombatGameMode::PushSkillUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	// 차례가 적에게 있으면 마지막으로 움직인 아군 것을 그대로 둔다. 카드는
	// 적 차례에 어차피 접혀 있고, 여기서 비우면 차례가 돌아올 때 한 프레임
	// 빈 카드가 스친다.
	UPlayerUnitModel* TurnUnitModel = GetTurnPlayerUnitModel();
	if (TurnUnitModel == nullptr)
	{
		TurnUnitModel = GetPlayerUnitModel(0);
	}
	if (TurnUnitModel == nullptr)
	{
		return;
	}

	// 들여다보는 유닛이 있으면 그쪽 스킬을 보여 준다. 없으면 지금 차례인 유닛.
	UPlayerUnitModel* PlayerUnitModel = FindPartyUnitModel(mInspectedUnitId);
	if (PlayerUnitModel == nullptr)
	{
		PlayerUnitModel = TurnUnitModel;
	}

	// 제 차례가 아닌 유닛의 카드는 전부 꺼서 보여 준다. 무엇을 들고 있는지
	// 아는 것과 지금 쓸 수 있는 것은 다른 이야기라, 감추는 대신 끈다.
	const bool bIsOwnTurn = PlayerUnitModel == TurnUnitModel;

	UUnitSkillComponentModel* SkillComponentModel = Cast<UUnitSkillComponentModel>(PlayerUnitModel->GetSkillComponentModel());
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

		UStaticUnitSkillData* StaticSkillData = (SkillEntry.IsValid() == true) ? StaticCast<UStaticUnitSkillData*>(SkillEntry.mData.Get()) : nullptr;
		if (StaticSkillData != nullptr)
		{
			SkillUIData.mName = StaticSkillData->mName;
			SkillUIData.mIcon = StaticSkillData->mIcon.LoadSynchronous();
			SkillUIData.mActionPointCost = StaticSkillData->mRequiredActionPoint;

			// 쿨타임. 총량은 데이터에셋 값을 그대로 쓴다 -- GetCooldownDuration은
			// 걸려 있는 효과를 읽으므로 쿨이 안 돌 때는 값이 없다.
			SkillUIData.mCooldownTurns = FMath::Max(
				SkillComponentModel->GetStaticCooldownDuration(i), 0);
			SkillUIData.mRemainingCooldown = (SkillComponentModel->IsCooldown(i) == true)
				? FMath::Max(SkillComponentModel->GetRemainingCooldownTime(i), 0) : 0;

			// 피해. 한 스킬이 여러 모션으로 나뉘어 때리므로 다 더한 것이 카드에
			// 적을 수다. 자동 생성 설명도 같은 값을 쓴다. 데이터에셋의
			// mMinDamage/mMaxDamage 분리(0806, 모호재)를 그대로 읽는다.
			//
			// 버프는 안 들어간다. 실제 피해는 AttackPoint/AttackFactor 를 거쳐
			// 나오는데, 카드에 적는 것은 스킬이 원래 가진 수다.
			int32 MinSkillDamage = 0;
			int32 MaxSkillDamage = 0;
			int32 ActionPointGain = 0;
			for (const FSkillPhaseLayer& MotionLayer : StaticSkillData->mSkillPhaseLayers)
			{
				for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
				{
					if (const FSkillEffectLayer_Attack* Attack = EffectLayer.GetPtr<FSkillEffectLayer_Attack>())
					{
						MinSkillDamage += Attack->mMinDamage;
						MaxSkillDamage += Attack->mMaxDamage;
					}
					// 행동력을 돌려주는 스킬(회복류)의 회수량. 안 채우면 필드가
					// 늘 0이라 카드가 회복 스킬을 맹탕으로 보여 준다(0807 감사).
					if (const FSkillEffectLayer_GetActionPoint* Gain =
						EffectLayer.GetPtr<FSkillEffectLayer_GetActionPoint>())
					{
						ActionPointGain += Gain->mActionPointGain;
					}
				}
			}
			SkillUIData.mDamageMin = MinSkillDamage;
			SkillUIData.mDamageMax = MaxSkillDamage;
			SkillUIData.mActionPointGain = ActionPointGain;
			// [합의필요] 크리티컬은 최종 피해 x1.5 고정으로 정했다(0728). 아직
			// 피해 계산에 크리 분기가 없어 UI 가 곱해 보여 준다. 계산이 생기면
			// 그쪽 값을 받아 이 줄을 지운다.
			SkillUIData.mCriticalDamage = FMath::RoundToInt(SkillUIData.mDamageMax * 1.5f);
			// 쓸 수 있는지는 세 가지를 다 본다. 하나라도 아니면 카드를 끈다.
			//
			//   쿨타임이 도는 중        IsCooldown
			//   행동력이 모자람          HasRequiredMovement
			//   겨냥한 자리가 사거리 밖   IsSkillUsableOnTarget
			//
			// 앞의 둘을 안 보고 있었다. 쿨타임이 도는 스킬도, AP 가 0 인
			// 유닛의 스킬도 멀쩡히 켜져 있었다. 화면은 이 판정을 안 한다 --
			// 사거리를 두 곳에서 세면 어긋나는 날이 온다.
			SkillUIData.mIsUsable = bIsOwnTurn
				&& SkillComponentModel->IsCooldown(i) == false
				&& SkillComponentModel->HasRequiredActionPoint(i) == true
				&& IsSkillUsableOnTarget(PlayerUnitModel, *StaticSkillData);
			// 모양 변환은 화면 공용 변환표(SkillDetailUIBuilder) 하나만 쓴다.
			SkillUIData.mTargeting.mSelectShape = SkillDetailUIBuilder::ToSelectShape(StaticSkillData->mAimPattern);
			SkillUIData.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRange);
			SkillUIData.mTargeting.mHitShape = SkillDetailUIBuilder::ToHitShape(StaticSkillData->mEffectPattern);
			SkillUIData.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectArea);
			// 차단 레이어를 그대로 넘긴다. 예전에는 "비었나" 만 bool 로 넘겨서
			// 장애물만 막힘 / 유닛만 막힘 / 둘 다 막힘이 한 값으로 뭉개졌다.
			SkillUIData.mTargeting.mAimBlockerMask = StaticSkillData->mAimBlockerMask;
			SkillUIData.mTargeting.mEffectBlockerMask = StaticSkillData->mEffectBlockerMask;
			// PR #466 LineToTarget -- 상세 모식도가 시전자→조준 경로를 그린다.
			SkillUIData.mTargeting.mTargetPattern =
				StaticSkillData->mTargetPattern == ETargetPattern::LineToTarget
				? ECombatSkillTargetPatternUI::LineToTarget
				: ECombatSkillTargetPatternUI::Default;
		}
	}

	mCombatUIModel->SetSkillUIs(SkillUIDatas);
}

void ACombatGameMode::PushSelectedSkillUIData(int32 SkillIndex) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	mCombatUIModel->SetSelectedSkill(SkillIndex);

	FCombatPendingActionUI PendingAction;
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex))
	{
		PendingAction.mType = ECombatPendingActionType::Skill;
		PendingAction.mActionPointCost = Skills[SkillIndex].mActionPointCost;
	}
	mCombatUIModel->SetPendingAction(PendingAction);
}

/**
 * @brief 톡 쳐서 고른 칸을 UI 에 내린다.
 *
 * @details
 * 겨냥한 자리가 바뀌면 그 자리에 쓸 수 있는 스킬이 달라진다. 그래서 스킬
 * 표시값도 같이 다시 내린다 -- 둘을 따로 내리면 한 프레임 동안 카드가 옛
 * 자리 기준으로 켜져 있다.
 * @param Tile     겨냥한 타일
 * @param HitActor 그 칸에 선 액터. 빈 칸이면 nullptr
 */
void ACombatGameMode::PushCombatTargetUIData(const FTileIndex& Tile, AActor* HitActor)
{
	if (mCombatUIModel == nullptr)
	{
		return;
	}

	/*
	 * 조준 중이 아닌 탭은 "살펴보기"다.
	 *
	 * 유닛을 짚었을 때만 겨냥을 움직인다. 빈 칸을 겨냥으로 세우면 카드 사용
	 * 가능 판정이 그 칸 사거리 기준으로 돌아서, 행동력이 멀쩡한데도 스킬이
	 * 잠겨 보인다 -- 카드를 펴려고 판을 누른 손이 카드를 잠갔다.
	 *
	 * 빈 칸 탭은 살펴보기를 **건드리지 않는다.** 위협 범위와 겨냥은 그 적을
	 * 다시 누르기 전까지 남는다 -- 카드를 펴려고 판을 누른 손이 봐 둔 위협을
	 * 지우면, 볼 때마다 다시 짚어야 한다.
	 *
	 * 조준 중에는 빈 칸도 의미가 있다(이동 목적지, 바닥 조준). 그쪽은 기존
	 * 규칙 그대로 둔다.
	 */
	const bool bBrowsing = mCombatUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::None;
	IBoardSelectionTargetView* SelectionTarget = Cast<IBoardSelectionTargetView>(HitActor);

	// 유닛이 서 있는 타일을 짚은 것은 유닛을 짚은 것이다. 트레이스가 유닛
	// 메시 대신 발밑 타일에 먼저 맞아도 뜻은 같다 -- 손가락은 칸을 누른다.
	if (bBrowsing == true && SelectionTarget == nullptr)
	{
		USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
		UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
		if (TileMap != nullptr)
		{
			if (UUnitModel* OccupantUnitModel = TileMap->GetActorOnTile<UUnitModel>(Tile))
			{
				HitActor = OccupantUnitModel->GetView<AActor>();
				SelectionTarget = Cast<IBoardSelectionTargetView>(HitActor);
			}
		}
	}
	if (bBrowsing == true && SelectionTarget == nullptr)
	{
		/*
		 * 빈 칸을 눌렀다. 살펴보던 것을 놓는다.
		 *
		 * 전에는 그대로 뒀다 -- 그때는 판 탭이 카드를 펴는 손이기도 해서,
		 * 카드를 부르려다 봐 둔 위협까지 지우면 곤란했다. 이제 카드는 턴
		 * 칸에서만 펴므로(0806) 판 탭은 "그만 보기" 하나만 뜻한다. 안 걷으면
		 * 요약판이 화면에 눌어붙어 안 내려간다(0806 검수).
		 */
		ClearThreatRangeView();
		ClearCombatTargetUIData();
		return;
	}

	// 겨냥한 칸을 다시 누르면 무른다.
	const FCombatTargetUI& Current = mCombatUIModel->GetTarget();
	if (Current.mIsValid == true && Current.mTile == Tile)
	{
		// 짚어서 칠해 둔 위협 범위도 같이 걷는다. 같은 적 재탭 = 그만 보기.
		ClearThreatRangeView();
		ClearCombatTargetUIData();
		return;
	}

	FCombatTargetUI TargetUIData;
	TargetUIData.mIsValid = true;
	TargetUIData.mTile = Tile;

	// 액터가 아니라 모델의 id 를 싣는다. UI 는 FUnitUI.mUnitId 와 같은 id
	// 공간만 알고 액터는 모른다.
	if (const IActorView* ActorView = Cast<IActorView>(HitActor))
	{
		if (const UBoardActorModel* BoardActorModel =
			Cast<UBoardActorModel>(ActorView->GetModel()))
		{
			TargetUIData.mUnitId = BoardActorModel->GetModelId();
		}
	}

	mCombatUIModel->SetTarget(TargetUIData);
	PushSkillUIData();

	/*
	 * 적을 짚으면 위협 범위를 판에 칠해 안내판(요약)과 나란히 읽히게 한다 --
	 * 길게 눌러야만 위협을 볼 수 있으면 확인이 조작 사이에 못 끼어든다.
	 * 아군을 짚으면 걷는다(ShowThreatRangeForTarget 내부 판정).
	 *
	 * 조준 중에는 건드리지 않는다. 사거리/효과 하이라이트와 겹치면 어느
	 * 칠이 무엇인지 읽을 수 없다.
	 */
	if (bBrowsing == true)
	{
		ShowThreatRangeForTarget(SelectionTarget);
	}
}

/**
 * @brief 겨냥을 푼다.
 *
 * 겨냥이 풀리면 그 자리 기준으로 켜고 끄던 카드도 다시 계산해야 한다.
 */
void ACombatGameMode::ClearCombatTargetUIData()
{
	if (mCombatUIModel == nullptr)
	{
		return;
	}
	mCombatUIModel->SetTarget(FCombatTargetUI());
	PushSkillUIData();
}

void ACombatGameMode::PushCombatTargetDetailUIData(IBoardSelectionTargetView* Target)
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	IObjectView* ObjectView = Cast<IObjectView>(Target);
	checkf(ObjectView != nullptr, TEXT("선택한 오브젝트 뷰 nullptr"));

	UBoardActorModel* BoardActorModel = ObjectView->GetModel<UBoardActorModel>();
	checkf(BoardActorModel != nullptr, TEXT("선택한 보드 액터 모델 nullptr"));

	PushBoardActorDetailUIData(BoardActorModel);
}

void ACombatGameMode::PushBoardActorDetailUIData(UBoardActorModel* BoardActorModel)
{
	if (mCombatUIModel == nullptr || BoardActorModel == nullptr)
	{
		return;
	}

	FUnitDetailUI UnitDetailUIData;
	UnitDetailUIData.mUnitId = BoardActorModel->GetModelId();
	UnitDetailUIData.mName = BoardActorModel->GetBoardActorDisplayName();
	UnitDetailUIData.mLevel = BoardActorModel->GetBoardActorLevel();
	UnitDetailUIData.mPortrait = BoardActorModel->GetBoardActorPortrait();

	UUnitModel* UnitModel = Cast<UUnitModel>(BoardActorModel);
	if (UTexture2D* MarchboundHero =
		ResolveMarchboundMercenaryPortrait(UnitModel, true))
	{
		UnitDetailUIData.mPortrait = MarchboundHero;
	}
	// 상세창 스킬 칸 탭을 되짚을 기준이다. 유닛이 아닌 것(장애물)을 골랐으면 비운다.
	mDetailUnitModel = UnitModel;
	if (UnitModel != nullptr)
	{
		UPassiveComponentModel* PassiveComponentModel = UnitModel->GetPassiveComponentModel();
		checkf(PassiveComponentModel != nullptr, TEXT("선택한 유닛 모델의 패시브 컴포넌트 nullptr"));
		for (const TObjectPtr<UTacticalPassive>& Passive : PassiveComponentModel->GetPassives())
		{
			UnitDetailUIData.mPassiveDescriptions.Add(Passive->GetStaticData()->mDescription);
		}

		// 들고 있는 스킬을 칸으로 내린다. 쿨타임이 돌든 안 돌든 다 내린다 --
		// 상세창은 "무엇을 할 수 있는 유닛인가"를 보는 곳이고, 지금 쓸 수
		// 있는지는 카드 레일이 말한다.
		if (USkillComponentModel* SkillComponentModel = UnitModel->GetSkillComponentModel())
		{
			const TArray<FSkillEntry>& SkillEntries = SkillComponentModel->GetSkills();
			for (int32 Index = 0; Index < SkillEntries.Num(); ++Index)
			{
				const UStaticSkillData* StaticSkillData = SkillEntries[Index].IsValid() == true
					? SkillEntries[Index].mData.Get() : nullptr;
				if (StaticSkillData == nullptr)
				{
					continue;
				}
				FUnitDetailSkillUI& SkillIcon = UnitDetailUIData.mSkills.AddDefaulted_GetRef();
				SkillIcon.mSkillIndex = Index;
				SkillIcon.mName = StaticSkillData->mName;
				SkillIcon.mIcon = StaticSkillData->mIcon.LoadSynchronous();
			}
		}
	}
	mCombatUIModel->SetUnitDetail(UnitDetailUIData);
}

void ACombatGameMode::PushUnitSkillDetailUIData(int32 SkillIndex) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	// 상세창이 닫힌 뒤 늦게 온 탭이거나 유닛이 죽어 사라졌을 수 있다.
	const UUnitModel* UnitModel = mDetailUnitModel.Get();
	if (UnitModel == nullptr)
	{
		return;
	}

	FSkillDetailUI SkillDetailUIData;
	FillSkillDetailUIData(UnitModel->GetSkillComponentModel(), SkillIndex, OUT SkillDetailUIData);
	mCombatUIModel->SetSkillDetail(SkillDetailUIData);
}

void ACombatGameMode::FillSkillDetailUIData(USkillComponentModel* SkillComponentModel,
	int32 SkillIndex, OUT FSkillDetailUI& OutDetail) const
{
	OutDetail.mSkillIndex = SkillIndex;
	if (SkillComponentModel == nullptr)
	{
		return;
	}

	const FSkillEntry* SkillEntry = SkillComponentModel->GetSkill(SkillIndex);
	UStaticSkillData* StaticSkillData = (SkillEntry != nullptr && SkillEntry->IsValid() == true) ? SkillEntry->mData.Get() : nullptr;
	if (StaticSkillData == nullptr)
	{
		return;
	}

	OutDetail.mName = StaticSkillData->mName;
	OutDetail.mDescription = StaticSkillData->mDescription;
	OutDetail.mIcon = StaticSkillData->mIcon.LoadSynchronous();
	// 이 상세는 플레이어 카드 레일뿐 아니라 임의 유닛(몬스터 포함)의 스킬에도
	// 쓰인다. 유닛별 슬롯 index만 넘기면 HUD에서 다른 유닛의 같은 슬롯과
	// 구분할 수 없으므로, 표시할 정적 수치를 응답 DTO에 함께 싣는다.
	if (const UStaticUnitSkillData* UnitSkill = Cast<UStaticUnitSkillData>(StaticSkillData))
	{
		OutDetail.mActionPointCost = FMath::Max(UnitSkill->mRequiredActionPoint, 0);
	}
	OutDetail.mCooldownTurns = FMath::Max(
		SkillComponentModel->GetStaticCooldownDuration(SkillIndex), 0);
	for (const FSkillPhaseLayer& MotionLayer : StaticSkillData->mSkillPhaseLayers)
	{
		for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer :
			MotionLayer.mSkillEffectLayers)
		{
			if (const FSkillEffectLayer_Attack* Attack =
				EffectLayer.GetPtr<FSkillEffectLayer_Attack>())
			{
				OutDetail.mDamageMin += Attack->mMinDamage;
				OutDetail.mDamageMax += Attack->mMaxDamage;
			}
			if (const FSkillEffectLayer_GetActionPoint* Gain =
				EffectLayer.GetPtr<FSkillEffectLayer_GetActionPoint>())
			{
				OutDetail.mActionPointGain += Gain->mActionPointGain;
			}
		}
	}
	OutDetail.mCriticalDamage = FMath::RoundToInt(OutDetail.mDamageMax * 1.5f);
	// 모양 변환은 화면 공용 변환표(SkillDetailUIBuilder) 하나만 쓴다.
	OutDetail.mTargeting.mSelectShape = SkillDetailUIBuilder::ToSelectShape(StaticSkillData->mAimPattern);
	OutDetail.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRange);
	OutDetail.mTargeting.mHitShape = SkillDetailUIBuilder::ToHitShape(StaticSkillData->mEffectPattern);
	OutDetail.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectArea);
	// 차단 레이어가 비어 있으면 곡사/관통으로 표시
	OutDetail.mTargeting.mAimBlockerMask = StaticSkillData->mAimBlockerMask;
	OutDetail.mTargeting.mEffectBlockerMask = StaticSkillData->mEffectBlockerMask;
	OutDetail.mTargeting.mTargetPattern =
		StaticSkillData->mTargetPattern == ETargetPattern::LineToTarget
		? ECombatSkillTargetPatternUI::LineToTarget
		: ECombatSkillTargetPatternUI::Default;
}

void ACombatGameMode::PushSkillDetailUIData(int32 SkillIndex) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	// 카드 레일(PushSkillUIData)과 같은 유닛을 봐야 한다. 들여다보는 유닛이
	// 있으면 그쪽, 없으면 지금 차례인 유닛 -- 0번으로 고정하면 다른 용병의
	// 카드를 길게 눌렀는데 기사의 스킬 설명이 뜬다.
	UPlayerUnitModel* PlayerUnitModel = FindPartyUnitModel(mInspectedUnitId);
	if (PlayerUnitModel == nullptr)
	{
		PlayerUnitModel = GetTurnPlayerUnitModel();
	}
	if (PlayerUnitModel == nullptr)
	{
		PlayerUnitModel = GetPlayerUnitModel(0);
	}
	checkf(PlayerUnitModel != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	FSkillDetailUI SkillDetailUIData;
	FillSkillDetailUIData(PlayerUnitModel->GetSkillComponentModel(), SkillIndex, OUT SkillDetailUIData);
	mCombatUIModel->SetSkillDetail(SkillDetailUIData);
}

void ACombatGameMode::PushPlayerMetaUIData() const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	// TODO : 여러 플레이어 등록해야함
	UPlayerUnitModel* PlayerUnitModel = GetPlayerUnitModel(0);
	if (PlayerUnitModel == nullptr)
	{
		return;
	}

	UAttributeSetComponentModel* PlayerAttributeSetComponentModel = PlayerUnitModel->GetAttributeComponentModel();
	checkf(PlayerAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	UPartyModel* PartyModel = GetPartyModel();
	checkf(PartyModel != nullptr, TEXT("파티 모델 nullptr"));

	UAttributeSetComponentModel* PartyAttributeSetComponentModel = PartyModel->GetAttributeComponentModel();
	checkf(PartyAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	FPlayerMetaUI PlayerMetaUIData;
	PlayerMetaUIData.mLevel = PlayerUnitModel->GetPlayerLevel();
	PlayerMetaUIData.mGold = PartyAttributeSetComponentModel->GetAttributeCurrentValue(UPartyAttributeSet::GetMoneyAttribute());
	PlayerMetaUIData.mExp = PlayerAttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetExpAttribute());
	PlayerMetaUIData.mMaxExp = PlayerAttributeSetComponentModel->GetAttributeCurrentValue(UPlayerUnitAttributeSet::GetMaxExpAttribute());

	if (const UPartyArtifactComponentModel* PartyArtifacts =
		PartyModel->GetPartyArtifactComponentModel())
	{
		for (const UStaticArtifactData* ArtifactData : PartyArtifacts->GetPartyArtifacts())
		{
			if (ArtifactData == nullptr)
			{
				continue;
			}
			FCombatArtifactUI& ArtifactUI =
				PlayerMetaUIData.mArtifacts.AddDefaulted_GetRef();
			ArtifactUI.mName = ArtifactData->mName;
			ArtifactUI.mIcon = ArtifactData->mIcon.LoadSynchronous();
			ArtifactUI.mRarityColor = GetRarityColor(ArtifactData->mRarityType);
			ArtifactUI.mRarityName = StaticEnum<ERarityType>() != nullptr
				? StaticEnum<ERarityType>()->GetDisplayNameTextByValue(
					StaticCast<int64>(ArtifactData->mRarityType))
				: FText::GetEmpty();
			ArtifactUI.mPrice = ArtifactData->mPrice;
			ArtifactUI.mRarityLevel = StaticCast<int32>(ArtifactData->mRarityType);
			// 설명은 붙은 패시브에서 모은다. 유닛 상세가 패시브를 모으는 것과 같다.
			for (const TSoftObjectPtr<UStaticPassiveData>& PassiveSoft : ArtifactData->mStaticPassiveData)
			{
				if (const UStaticPassiveData* Passive = PassiveSoft.LoadSynchronous())
				{
					ArtifactUI.mEffectDescriptions.Add(Passive->mDescription);
				}
			}
		}
	}

#if WITH_EDITOR
	// UI 검수용 에디터 실행에서는 빈 파티도 아티팩트 줄을 확인할 수 있게 한다.
	// 실제 보유물이 하나라도 있으면 이 fixture는 전혀 개입하지 않는다.
	if (PlayerMetaUIData.mArtifacts.IsEmpty())
	{
		struct FPreviewArtifact
		{
			const TCHAR* Name;
			const TCHAR* IconPath;
			FLinearColor RarityColor;
		};
		static const FPreviewArtifact PreviewArtifacts[] = {
			{ TEXT("정찰의 나침반"),
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Treasure.T_MapNode_Treasure"),
				FLinearColor(0.86f, 0.98f, 0.94f, 1.f) },
			{ TEXT("푸른 깃털"),
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_RareTreasure.T_MapNode_RareTreasure"),
				FLinearColor(0.55f, 0.72f, 1.f, 1.f) },
			{ TEXT("왕의 부적"),
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_EpicTreasure.T_MapNode_EpicTreasure"),
				FLinearColor(0.82f, 0.58f, 1.f, 1.f) },
		};
		for (const FPreviewArtifact& Preview : PreviewArtifacts)
		{
			FCombatArtifactUI& Artifact =
				PlayerMetaUIData.mArtifacts.AddDefaulted_GetRef();
			Artifact.mName = FText::FromString(Preview.Name);
			Artifact.mIcon = LoadObject<UTexture2D>(nullptr, Preview.IconPath);
			Artifact.mRarityColor = Preview.RarityColor;
		}
	}
#endif

	mCombatUIModel->SetPlayerMeta(PlayerMetaUIData);
}

void ACombatGameMode::BuildCombatFloatingLogRequests(const TArray<FSRPGTurnEventLog>& TurnEventLogs, const bool bBindMotionIndices, OUT TArray<FCombatFloatingLogRequest>& OutRequests) const
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	UTileMapModel* TileMapModel = CombatModel->GetTileMap();
	checkf(TileMapModel != nullptr, TEXT("타일맵 모델 nullptr"));

	/* 모션 내 이벤트 로그 마다 UI 요청서 작성 함수 */

	auto AddFloatingLogs = [bBindMotionIndices](const FSRPGBoardActorEventLog& EventLog, const FVector& ViewActorLocation, const int32 TurnIndex, const int32 ActionIndex, const int32 MotionIndex, OUT int32& Sequence, OUT TArray<FCombatFloatingLogRequest>& Requests) {

		auto MakeLogRequest = [bBindMotionIndices, TurnIndex, ActionIndex, MotionIndex](int32 Amount, EFloatingLogIconType IconType, EFloatingLogColorType ColorType, const FVector& ViewLocation, int32 Sequence) -> FCombatFloatingLogRequest {
			FCombatFloatingLogRequest Request;
			Request.mWorldLocation = ViewLocation;
			Request.mText = FText::FromString(FString::Printf(TEXT("%+d"), Amount));
			Request.mIconType = IconType;
			Request.mColorType = ColorType;
			Request.mSequence = Sequence;
			Request.mTurnIndex = bBindMotionIndices == true ? TurnIndex : INDEX_NONE;
			Request.mActionIndex = bBindMotionIndices == true ? ActionIndex : INDEX_NONE;
			Request.mMotionIndex = bBindMotionIndices == true ? MotionIndex : INDEX_NONE;
			// mIsPreview는 여기서 정하지 않는다 — 표시 경로(예측/실전)는 호출자가 정한다.
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

					AddFloatingLogs(*EventLog, SpawnLocationPair.Value, TurnIndex, ActionIndex, MotionIndex, OUT Sequence, OUT OutRequests);
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
					AddFloatingLogs(*EventLog, ViewActorLocation, TurnIndex, ActionIndex, MotionIndex, OUT Sequence, OUT OutRequests);
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
					AddFloatingLogs(*EventLog, ViewActorLocation, TurnIndex, ActionIndex, MotionIndex, OUT Sequence, OUT OutRequests);
				}
			}
		}
	}
}

TArray<FUnitPredictionUI> ACombatGameMode::BuildUnitPredictions(const TArray<FSRPGTurnEventLog>& TurnEventLogs) const
{
	// 로그에 등장한 보드 액터 id마다 요약 하나. 등장 순서를 지켜 쌓는다.
	// (모델 조회 없이 로그만으로 집계한다 — 예측은 로그가 이미 전부를 안다.)
	TArray<FUnitPredictionUI> Predictions;
	TMap<int32, int32> PredictionIndexById;

	for (const FSRPGTurnEventLog& TurnLog : TurnEventLogs)
	{
		for (const FSRPGActionEventLog& ActionLog : TurnLog.mActionEventLogs)
		{
			for (const FSRPGMotionEventLog& MotionLog : ActionLog.mMotionEventLogs)
			{
				for (const TPair<int32, FSRPGBoardActorEventLog>& EventLogPair : MotionLog.mBoardActorEventLogs)
				{
					int32 PredictionIndex = INDEX_NONE;
					if (const int32* FoundIndex = PredictionIndexById.Find(EventLogPair.Key))
					{
						PredictionIndex = *FoundIndex;
					}
					else
					{
						FUnitPredictionUI& NewPrediction = Predictions.AddDefaulted_GetRef();
						NewPrediction.mUnitId = EventLogPair.Key;
						PredictionIndex = Predictions.Num() - 1;
						PredictionIndexById.Add(EventLogPair.Key, PredictionIndex);
					}
					FUnitPredictionUI& Prediction = Predictions[PredictionIndex];

					// HP 증감: HP 속성 로그의 Magnitude 합. 피해는 음수로 온다.
					for (const FSRPGAttributeEffectEventLog& AttrLog : EventLogPair.Value.mAttributeEffectEventLogs)
					{
						if (AttrLog.mEffectAttribute == UUnitAttributeSet::GetHPAttribute())
						{
							Prediction.mHPDelta += AttrLog.mMagnitude;
						}
					}

					// 사망/도착 타일: 점유 로그에서 Exit=퇴장(사망·제거),
					// Move/Enter=도달 타일(마지막 값이 최종 자리).
					for (const FSRPGTileEffectEventLog& TileLog : EventLogPair.Value.mTileEffectEventLogs)
					{
						if (TileLog.mOccupancyState == ESRPGTileOccupancyState::Exit)
						{
							Prediction.mWillDie = true;
						}
						else if (TileLog.mOccupancyState == ESRPGTileOccupancyState::Move
							|| TileLog.mOccupancyState == ESRPGTileOccupancyState::Enter)
						{
							Prediction.mPredictedTile = TileLog.mNextTileIndex;
						}
					}

					// mPredictedStatuses는 채우지 않는다 — 태그 로그(mTagEffectEventLogs)는
					// 증감(델타)뿐이라 절대 스택 수를 지어낼 수 없다. 절대값 공급 경로가
					// 생기면 그때 채운다.
				}
			}
		}
	}

	return Predictions;
}

void ACombatGameMode::PushSimulationPreviewUIData(const TArray<FSRPGTurnEventLog>& TurnEventLogs) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	USimulationPreviewUIModel* SimulationPreviewUIModel = mCombatUIModel->GetSimulationPreviewUIModel();
	checkf(SimulationPreviewUIModel != nullptr, TEXT("시뮬레이션 미리보기 UI Model nullptr"));

	// 새 미리보기 세대를 연다 — 이전 미리보기 payload는 여기서 통째로 버려진다.
	const int32 PreviewGeneration = SimulationPreviewUIModel->BeginPreview();

	TArray<FCombatFloatingLogRequest> Requests;
	BuildCombatFloatingLogRequests(TurnEventLogs, /*bBindMotionIndices=*/true, OUT Requests);

	// 미리보기 표시 규칙(수명 소멸 없음·즉시 스폰)은 이 경로가 정한다 — 빌더는 중립이다.
	for (FCombatFloatingLogRequest& Request : Requests)
	{
		Request.mIsPreview = true;
	}

	SimulationPreviewUIModel->SetPreviewEventBatch(PreviewGeneration, Requests);
	SimulationPreviewUIModel->SetPredictedUnits(PreviewGeneration, BuildUnitPredictions(TurnEventLogs));
}

void ACombatGameMode::PushCombatEventUIData(const TArray<FSRPGTurnEventLog>& TurnEventLogs) const
{
	checkf(mCombatUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	TArray<FCombatFloatingLogRequest> Requests;
	BuildCombatFloatingLogRequests(TurnEventLogs, /*bBindMotionIndices=*/false, OUT Requests);

	// 실전 juice 로그다. mIsPreview는 빌더 기본값(false) 그대로 둔다.
	mCombatUIModel->SetCombatEventBatch(ECombatEventDataSourceUI::LiveCombat, Requests);
}

void ACombatGameMode::ShowSkillDetailPreview(UUnitModel* UnitModel,
	const int32 SkillIndex)
{
	ClearSkillDetailPreview();
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	USkillComponentModel* SkillComponent = UnitModel != nullptr
		? UnitModel->GetSkillComponentModel() : nullptr;
	if (TileMap == nullptr || SkillComponent == nullptr
		|| SkillComponent->GetSkill(SkillIndex) == nullptr)
	{
		return;
	}

	const TArray<FTileIndex> AimTiles = SkillComponent->GetAimableTiles(
		TileMap, SkillIndex);
	if (AimTiles.IsEmpty())
	{
		return;
	}

	const FTileIndex Origin = UnitModel->GetTileTransform().mIndex;
	FTileIndex PreviewTarget = AimTiles[0];
	bool bFoundBoardActor = false;
	int32 BestDistance = -1;
	for (const FTileIndex& Tile : AimTiles)
	{
		// 실제 유닛이 사거리 안에 있으면 빈 칸보다 우선한다. 없을 때는 가장 먼
		// 합법 타일을 골라 사거리 끝과 효과 범위가 한 화면에서 읽히게 한다.
		const UUnitModel* Occupant = TileMap->GetActorOnTile<UUnitModel>(Tile);
		const bool bOtherUnit = Occupant != nullptr && Occupant != UnitModel;
		const int32 Distance = FMath::Abs(Tile.mX - Origin.mX)
			+ FMath::Abs(Tile.mY - Origin.mY);
		if ((bOtherUnit && bFoundBoardActor == false)
			|| (bOtherUnit == bFoundBoardActor && Distance > BestDistance))
		{
			PreviewTarget = Tile;
			BestDistance = Distance;
			bFoundBoardActor = bOtherUnit;
		}
	}

	const TArray<FTileIndex> TargetTiles = SkillComponent->GetTargetTiles(
		TileMap, SkillIndex, PreviewTarget);
	const TArray<FTileIndex> EffectTiles = SkillComponent->GetEffectTiles(
		TileMap, SkillIndex, TargetTiles);
	TileMap->SetTileHighlight(AimTiles, ETileHighlightFlag::Aim);
	TileMap->SetTileHighlight(TargetTiles, ETileHighlightFlag::Select);
	TileMap->SetTileHighlight(EffectTiles, ETileHighlightFlag::Effect);
	mSkillDetailPreviewActive = true;
}

void ACombatGameMode::ClearSkillDetailPreview()
{
	if (mSkillDetailPreviewActive == false)
	{
		return;
	}
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	if (TileMap != nullptr)
	{
		TileMap->ClearTileHighlight(ETileHighlightFlag::Aim
			| ETileHighlightFlag::Select | ETileHighlightFlag::Effect);
	}
	mSkillDetailPreviewActive = false;
}

void ACombatGameMode::PushCombatRewardUIData() const
{
	checkf(mRewardUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	UPartyModel* PartyModel = GetPartyModel();
	checkf(PartyModel != nullptr, TEXT("파티 모델 nullptr"));

	UAttributeSetComponentModel* PartyAttributeSetComponentModel = PartyModel->GetAttributeComponentModel();
	checkf(PartyAttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	FRewardUI RewardUIData;
	RewardUIData.mTitle = NSLOCTEXT("CombatGameMode", "VictoryRewardTitle", "전투 보상");

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData != nullptr)
	{
		if (const FMonsterRoom* CurrentRoom = GetMonsterRewardRoom(RunPersistData->GetCurrentRoom()))
		{
			RewardUIData.mGoldGained = CurrentRoom->mRewardMoney;
			RewardUIData.mExpGained = CurrentRoom->mRewardExp;
		}
	}

	{
		const float CurrentGold = PartyAttributeSetComponentModel->GetAttributeCurrentValue(UPartyAttributeSet::GetMoneyAttribute());
		RewardUIData.mGoldBalance = FMath::RoundToInt(CurrentGold) + RewardUIData.mGoldGained;
	}

	const TArray<TObjectPtr<UPlayerUnitModel>>& PlayerUnitModels = GetPlayerUnitModels();
	RewardUIData.mMercenaryExp.Reserve(PlayerUnitModels.Num());
	for (const UPlayerUnitModel* PlayerUnitModel : PlayerUnitModels)
	{
		if (PlayerUnitModel == nullptr)
		{
			continue;
		}

		const UAttributeSetComponentModel* PlayerAttributes =
			PlayerUnitModel->GetAttributeComponentModel();
		if (PlayerAttributes == nullptr)
		{
			continue;
		}

		FRewardMercenaryExpUI& MercenaryExp =
			RewardUIData.mMercenaryExp.AddDefaulted_GetRef();
		MercenaryExp.mName = PlayerUnitModel->GetBoardActorDisplayName();
		if (MercenaryExp.mName.IsEmpty())
		{
			MercenaryExp.mName = NSLOCTEXT(
				"CombatGameMode", "UnknownRewardMercenary", "Mercenary");
		}
		MercenaryExp.mPortrait = ResolveMarchboundMercenaryPortrait(
			PlayerUnitModel, false);
		if (MercenaryExp.mPortrait == nullptr)
		{
			MercenaryExp.mPortrait = PlayerUnitModel->GetBoardActorIcon();
		}
		if (MercenaryExp.mPortrait == nullptr)
		{
			MercenaryExp.mPortrait = PlayerUnitModel->GetBoardActorPortrait();
		}
		if (MercenaryExp.mPortrait == nullptr)
		{
			MercenaryExp.mPortrait = ResolveTurnPortraitFallback(PlayerUnitModel);
		}
		const int32 PlayerLevel = PlayerUnitModel->GetPlayerLevel();
		MercenaryExp.mLevel = PlayerLevel;
		MercenaryExp.mExpBefore = PlayerAttributes->GetAttributeCurrentValue(
			UPlayerUnitAttributeSet::GetExpAttribute());
		MercenaryExp.mExpAfter = MercenaryExp.mExpBefore
			+ StaticCast<float>(RewardUIData.mExpGained);
		MercenaryExp.mMaxExp = PlayerAttributes->GetAttributeCurrentValue(
			UPlayerUnitAttributeSet::GetMaxExpAttribute());
	}

	// 기존 WBP/Blueprint가 단일 진행도 필드를 읽는 경우에는 첫 용병을
	// 대표 fallback으로 유지한다. 네이티브 보상 행은 위 배열을 사용한다.
	if (RewardUIData.mMercenaryExp.IsEmpty() == false)
	{
		const FRewardMercenaryExpUI& First = RewardUIData.mMercenaryExp[0];
		RewardUIData.mLevelBefore = First.mLevel;
		RewardUIData.mLevelAfter = First.mLevel;
		RewardUIData.mExpBefore = First.mExpBefore;
		RewardUIData.mExpAfter = First.mExpAfter;
		RewardUIData.mMaxExp = First.mMaxExp;
	}

	mRewardUIModel->SetReward(RewardUIData);
}

void ACombatGameMode::PushCombatRewardChoicesUIData() const
{
	checkf(mRewardUIModel != nullptr, TEXT("전투 UI Model nullptr"));

	TArray<FRewardChoiceUI> Choices;

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr)
	{
		FRewardSelectionOfferUI EmptyOffer;
		mRewardUIModel->SetSelectionOffer(EmptyOffer);
		return;
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
			Choice.mKind = ERewardChoiceKind::Artifact;
			Choice.mSourceAssetId = EquipmentId;
			Choice.mName = FText::FromName(EquipmentId.PrimaryAssetName);

			// StageBuilder rolls Artifact primary assets here. Reading them through
			// UStaticEquipmentData silently left the reward/detail DTO with fallback
			// names and no icon/effect data.
			if (const UStaticArtifactData* ArtifactData =
				LoadPrimaryAssetData<UStaticArtifactData>(EquipmentId))
			{
				Choice.mName = ArtifactData->mName.IsEmpty()
					? Choice.mName : ArtifactData->mName;
				Choice.mIcon = ArtifactData->mIcon.LoadSynchronous();
				Choice.mRarityColor = GetRarityColor(ArtifactData->mRarityType);
				Choice.mRarityName = StaticEnum<ERarityType>() != nullptr
					? StaticEnum<ERarityType>()->GetDisplayNameTextByValue(
						StaticCast<int64>(ArtifactData->mRarityType))
					: FText::GetEmpty();
				Choice.mRarityLevel = StaticCast<int32>(ArtifactData->mRarityType);

				TArray<FString> EffectLines;
				for (const TSoftObjectPtr<UStaticPassiveData>& PassiveSoft
					: ArtifactData->mStaticPassiveData)
				{
					if (const UStaticPassiveData* Passive =
						PassiveSoft.LoadSynchronous())
					{
						if (!Passive->mDescription.IsEmpty())
						{
							EffectLines.Add(Passive->mDescription.ToString());
						}
					}
				}
				if (EffectLines.IsEmpty() && !ArtifactData->mStatModifiers.IsEmpty())
				{
					EffectLines.Add(TEXT("파티 전체 능력치를 강화합니다."));
				}
				Choice.mDescription = EffectLines.IsEmpty()
					? FText::FromString(TEXT("파티 전체에 적용됩니다."))
					: FText::FromString(FString::Join(EffectLines, TEXT("\n")));
			}

			Choices.Add(Choice);
		};

	switch (CurrentRoom.mType)
	{
	case ERoomType::EliteMonster:
	{
		const FEliteMonsterRoom& EliteRoom = static_cast<const FEliteMonsterRoom&>(CurrentRoom);
		for (const FPrimaryAssetId& ArtifactId : EliteRoom.GetEffectiveRewardArtifactDataIds())
		{
			if (Choices.Num() >= 3)
			{
				break;
			}
			AddEquipmentReward(ArtifactId);
		}
		break;
	}
	case ERoomType::BossMonster:
	{
		const FBossMonsterRoom& BossRoom = static_cast<const FBossMonsterRoom&>(CurrentRoom);
		for (const FPrimaryAssetId& ArtifactId : BossRoom.GetEffectiveRewardArtifactDataIds())
		{
			if (Choices.Num() >= 3)
			{
				break;
			}
			AddEquipmentReward(ArtifactId);
		}
		break;
	}
	default:
		break;
	}

	FRewardSelectionOfferUI SelectionOffer;
	SelectionOffer.mOptions = MoveTemp(Choices);
	SelectionOffer.mSelectionCount = 1;
	mRewardUIModel->SetSelectionOffer(SelectionOffer);
}
