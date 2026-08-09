#include "GameMode/CombatGameMode.h"

#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"

#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"
#include "Setting/RDWorldSettings.h"

#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "Pawn/Camera/CombatCameraPawn.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"

#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/CombatUIWidgetBase.h"
#include "UI/Reward/RewardUIModel.h"

#include "Actor/ActorView.h"

#include "SRPGFramework/SRPGCommand.h"

#include "SRPGFramework/SRPGSkillBuildAction.h"
#include "SRPGFramework/SRPGMoveBuildAction.h"
#include "SRPGFramework/SRPGTurnEndAction.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/EquipmentComponent/EquipmentComponentModel.h"
#include "Component/PassiveComponent/PassiveComponentModel.h"
#include "Component/SkillComponent/UnitSkillComponentModel.h"

#include "TAS/Passive/TacticalPassive.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"
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

	mGoldRewardClaimed = false;
	mExpRewardClaimed = false;
	mClaimedRewardChoiceIndices.Reset();
	mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);

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
		mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);
		PushPlayerMetaUIData();
		mCombatUIModel->OnBeginCombat.Broadcast(Barrier);
		});
	CombatModel->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result) {
		CancelPendingActionEndAfterCameraReturn();
		mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);
		PushPlayerMetaUIData();
		PushCombatResultUIData(Result);
		mCombatUIModel->OnEndCombat.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext) {
		// 차례가 왔으니 남의 카드를 접는다. 새 차례에 옛 유닛 카드가 떠 있으면
		// 무엇을 조종하는 중인지 알 수 없다.
		mInspectedUnitId = INDEX_NONE;
		mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);
		PushTurnUIData();
		PushUnitUIData();
		PushSkillUIData();
		// 턴 시작 연출: 배리어를 HUD로 넘겨 턴 배너가 끝날 때까지 실제 턴 실행을 대기시킨다.
		mCombatUIModel->OnBeginAnyTurn.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyRoundUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, int32 RoundCount) {
		PushTurnUIData();   // 배너 숫자용 mRound 갱신
		mCombatUIModel->OnBeginAnyRound.Broadcast(Barrier);
		});
	CombatModel->OnEndAnyTurnUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result) {
		CancelPendingActionEndAfterCameraReturn();
		mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);
		mCombatUIModel->OnEndAnyTurn.Broadcast(Barrier);
		});
	CombatModel->OnBeginAnyTurnActionUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, const USRPGAction* Action) {
		// 이전 액션의 카메라 복귀 대기가 남아 있어도 새 액션을 끝내면 안 된다.
		CancelPendingActionEndAfterCameraReturn();
		mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);
		mCombatUIModel->NotifyCombatFloatingLogsCleared();
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
		// 성공 이동은 실제 최종 AP와 같은 값으로 유지되고, 취소 이동은 실제
		// 차감이 없으므로 원래 AP로 돌아간다.
		mCombatUIModel->ClearMoveAPStepPresentation(INDEX_NONE);
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
	mCombatUIModel->OnAbandonRun.AddUniqueDynamic(this, &ACombatGameMode::HandleAbandonRun);
	mCombatUIModel->OnRetryCombat.AddUniqueDynamic(this, &ACombatGameMode::HandleRetryCombat);
	mRewardUIModel->OnRewardClaimRequested.AddUniqueDynamic(this, &ACombatGameMode::HandleRewardClaimed);

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
}

UCombatUIModel* ACombatGameMode::GetCombatUIModel() const
{
	return mCombatUIModel;
}

URewardUIModel* ACombatGameMode::GetRewardUIModel() const
{
	return mRewardUIModel;
}

void ACombatGameMode::HandleOpenInventory()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	URDUserWidget* InventoryWidget = WorldWidgetSubsystem != nullptr
		? Cast<URDUserWidget>(WorldWidgetSubsystem->GetWorldWidget(EWorldWidgetType::Inventory))
		: nullptr;
	if (InventoryWidget != nullptr)
	{
		InventoryWidget->OpenUI();
	}
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

bool ACombatGameMode::SelectMove()
{
	USRPGCommandRouterModel* CommandRouterModel = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouterModel != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> MoveSelectCommand;
	MoveSelectCommand.InitializeAs<FSRPGMoveSelectCommand>();
	MoveSelectCommand.GetMutable<FSRPGMoveSelectCommand>().OnChangeMoveBuildPhase.AddWeakLambda(this, [this](const USRPGMoveBuildAction* Action, ESRPGMoveBuildPhase Phase) {
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
	switch (Type)
	{
	// 스킬/이동을 골라도 켜 둔 위협 범위는 걷지 않는다. "저기까지 오는데
	// 어디로 피하지"를 보면서 고르라고 켜 둔 것이다 -- 켜고 끄는 것은 적
	// 타일 탭만 한다. 턴이 끝나면 적이 움직여 칠이 낡으므로 그때는 걷는다.
	case ECombatInputType::SelectSkill:
		SelectSkill(IntPayload);
		break;
	case ECombatInputType::Move:
		SelectMove();
		break;
	case ECombatInputType::EndTurn:
		ClearThreatRangeView();
		EndTurn();
		break;
	case ECombatInputType::LongPressSkill:
		// 길게 누른 스킬의 상세 정보를 UIModel에 채운다.
		PushSkillDetailUIData(IntPayload);
		break;
	case ECombatInputType::InspectUnitSkill:
		// 상세창의 스킬 칸을 탭했다. 기준은 그 상세창에 뜬 유닛이다.
		PushUnitSkillDetailUIData(IntPayload);
		break;
	case ECombatInputType::LongPressUnit:
		// UI 가 상세 패널을 닫았다는 신호로 INDEX_NONE 을 보낸다. 패널과 함께
		// 뜬 위협 범위 칠도 같이 걷는다. 유닛 상세 요청 자체는 월드 롱프레스
		// (HandleCombatWorldTouch)가 트레이스로 처리하므로 여기서는 닫기만 맡는다.
		if (IntPayload == INDEX_NONE)
		{
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
	if (mCombatUIModel == nullptr || mCombatUIModel->GetTarget().mIsValid == false)
	{
		return;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (TileMap == nullptr || Controller == nullptr)
	{
		return;
	}

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const FVector World = TileMap->TileToWorldLocation(mCombatUIModel->GetTarget().mTile);
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
	AbandonRunFromRoom();
}

void ACombatGameMode::HandleRetryCombat()
{
	const URunPersistData* RunData = GetRunPersistData();
	if (RunData == nullptr || RunData->IsActive() == false)
	{
		UE_LOG(LogCombatGameMode, Warning, TEXT("Retry combat ignored because no active run exists."));
		return;
	}

	const FRoom& CurrentRoom = RunData->GetCurrentRoom();
	const bool bTransitionStarted = PreloadAndTransitionRoomAsync(CurrentRoom.mRow, CurrentRoom.mColumn);
	if (bTransitionStarted == false)
	{
		UE_LOG(LogCombatGameMode, Warning, TEXT("Retry combat failed to preload current room (%d, %d)."),
			CurrentRoom.mRow, CurrentRoom.mColumn);
	}
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
	const int32 UnitId = Unit->GetModelId();

	Unit->OnMoveStepArrivedUI.AddWeakLambda(
		this,
		[this, UnitId](const int32 CompletedStepCount, const int32 /*TotalStepCount*/)
		{
			mCombatUIModel->SetMoveAPStepPresentation(UnitId, CompletedStepCount);
		});

	// 각 속성이 변경될 때마다 OnRefreshUnitUI를 브로드캐스트하도록 바인딩
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetActionPointAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		// 행동력이 줄면 못 쓰게 되는 카드가 생긴다. 같이 다시 내린다.
		PushSkillUIData();
		});
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).AddWeakLambda(this, [this](const FTacticalAttributeChangeData& Data) {
		PushUnitUIData();
		});

	// 상태 이상 태그 변경 시에도 UI 갱신 바인딩
	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, ETacticalTagEventType::NewOrRemoved).AddWeakLambda(this, [this](const FGameplayTag Tag, int32 Count) {
		PushUnitUIData();
		});

	PushTurnUIData();
	PushUnitUIData();
}

void ACombatGameMode::OnUnregisterUnit(UUnitModel* Unit)
{
	UAttributeSetComponentModel* AttributeSetComponentModel = Unit->GetAttributeComponentModel();
	checkf(AttributeSetComponentModel != nullptr, TEXT("속성 컴포넌트 nullptr"));

	Unit->OnMoveStepArrivedUI.RemoveAll(this);
	mCombatUIModel->ClearMoveAPStepPresentation(Unit->GetModelId());

	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetMaxHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetHPAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetActionPointAttribute()).RemoveAll(this);
	AttributeSetComponentModel->GetTacticalAttributeValueChangeDelegate(UPlayerUnitAttributeSet::GetDefenseAttribute()).RemoveAll(this);

	AttributeSetComponentModel->RegisterTacticalTagEvent(EffectTags::GameplayEffect_StatusEffect, ETacticalTagEventType::NewOrRemoved).RemoveAll(this);

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
	for (const UPlayerUnitModel* PlayerUnitModel : GetPlayerUnitModels())
	{
		if (PlayerUnitModel == nullptr || CombatResultUIData.mPartyPortraits.Num() >= 3)
		{
			continue;
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
		CombatResultUIData.mPartyPortraits.Add(Portrait);
	}
	mCombatUIModel->SetCombatResultUI(CombatResultUIData);
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
	TurnUI.mCurrentRoundRemainingTurnCount =
		CombatModel->GetTurnContextCount();
	for (const TObjectPtr<USRPGTurnContext>& TurnContext : TurnContexts)
	{
		TurnUI.mTurnOrderUnitIds.Add(TurnContext->GetOwner()->GetModelId());
	}

	/*
	 * 다음 라운드 미리보기.
	 *
	 * 모델의 후보 계산(GetOrderedTurnCandidates)은 **지금** 속도로 판정한다 --
	 * 라운드 교대 시점(충전 직후)에 쓰는 함수라 그렇다. 라운드 중반에 그대로
	 * 부르면 이번 턴에 속도를 이미 쓴 유닛이 전부 탈락해 미리보기가 빈다
	 * (0806 검수: 다음 라운드가 안 뜸). 그래서 여기서는 라운드가 넘어갈 때
	 * 받을 충전량을 더한 값으로 내다본다. 동률 무작위까지는 흉내 내지 않는다
	 * -- 이건 예고지 약속이 아니다.
	 */
	struct FNextRoundPreview
	{
		int32 mUnitId = INDEX_NONE;
		int32 mProjectedSpeed = 0;
	};
	TArray<FNextRoundPreview> Previews;
	const int32 RequiredSpeed =
		GetDefault<UGameBalanceSettings>()->mRequiredSpeedPointForTurn;

	/*
	 * 아무도 못 차는 빈 라운드는 모델이 즉시 건너뛴다(충전 5 · 비용 10이면
	 * 턴은 두 라운드에 한 번). 그래서 충전을 한 라운드씩 쌓아 보며 누군가
	 * 처음 차는 라운드를 찾고, 그 라운드를 "다음"으로 표기한다 -- 한 라운드
	 * 뒤만 보면 미리보기가 늘 비어 있었다(0806 검수).
	 */
	int32 RoundsAhead = 1;
	constexpr int32 MaxRoundsToScan = 20;
	for (; RoundsAhead <= MaxRoundsToScan && Previews.IsEmpty(); ++RoundsAhead)
	{
		for (const TObjectPtr<UUnitModel>& UnitModel : CombatModel->GetUnits())
		{
			UAttributeSetComponentModel* Attributes = UnitModel != nullptr
				? UnitModel->GetAttributeComponentModel() : nullptr;
			if (Attributes == nullptr)
			{
				continue;
			}
			// 죽은 유닛은 다음 라운드에 없다. 모델에서 빠지는 것은 턴이
			// 끝날 때라, 그 사이 미리보기에 끼는 것을 막는다(0807 감사).
			if (Attributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetHPAttribute()) <= 0.f)
			{
				continue;
			}
			const int32 ProjectedSpeed = StaticCast<int32>(FMath::Floor(
				Attributes->GetAttributeCurrentValue(
					UUnitAttributeSet::GetSpeedPointAttribute())
				+ Attributes->GetAttributeCurrentValue(
					UUnitAttributeSet::GetRechargeSpeedPointAttribute())
					* RoundsAhead));
			if (ProjectedSpeed < RequiredSpeed)
			{
				continue;
			}
			Previews.Add({ UnitModel->GetModelId(), ProjectedSpeed });
		}
		if (Previews.IsEmpty() == false)
		{
			break;
		}
	}
	// 동률 무작위까지는 흉내 내지 않는다 -- 이건 예고지 약속이 아니다.
	Previews.StableSort(
		[](const FNextRoundPreview& Lhs, const FNextRoundPreview& Rhs)
		{ return Lhs.mProjectedSpeed > Rhs.mProjectedSpeed; });
	for (const FNextRoundPreview& Preview : Previews)
	{
		TurnUI.mNextRoundUnitIds.Add(Preview.mUnitId);
	}
	TurnUI.mNextRoundOffset = FMath::Max(RoundsAhead, 1);
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
		// 턴바 밑 "속도"는 라운드마다 충전되는 고유 속도(RechargeSpeedPoint)를 보여 준다.
		// SpeedPoint는 라운드 진행 중 소비/누적되는 현재값이라 표시 기준으로 쓰지 않는다.
		UnitUIData.mSpeedPoint = AttributeSetComponentModel->GetAttributeCurrentValue(
			UUnitAttributeSet::GetRechargeSpeedPointAttribute());
		if (UnitUIData.mSpeedPoint <= 0.f)
		{
			UnitUIData.mSpeedPoint = AttributeSetComponentModel->GetAttributeCurrentValue(
				UUnitAttributeSet::GetSpeedPointFactorAttribute());
		}
		UnitUIData.mDefensePoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetDefenseAttribute());
		UnitUIData.mMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetActionPointAttribute());
		UnitUIData.mMaxMovementPoint = AttributeSetComponentModel->GetAttributeCurrentValue(UUnitAttributeSet::GetRechargeActionPointAttribute());
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

#if WITH_EDITOR
		// 에디터 Standalone/PIE에서 전투 HUD의 큰 상태 슬롯을 바로 검수한다.
		// 실제 태그가 없는 첫 아군과 첫 적에만 DTO 표시용 fixture를 보탠다.
		if (UnitUIData.mStatusEffects.IsEmpty())
		{
			const bool bFirstPlayer = UnitUIData.mIsPlayer && i == 0;
			const bool bFirstEnemy = !UnitUIData.mIsPlayer
				&& !UnitUIDatas.ContainsByPredicate([](const FUnitUI& Existing)
				{
					return !Existing.mIsPlayer && !Existing.mStatusEffects.IsEmpty();
				});
			if (bFirstPlayer || bFirstEnemy)
			{
				FStatusEffectUI PreviewStatus;
				PreviewStatus.mTag = bFirstPlayer
					? EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification
					: EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability;
				PreviewStatus.mStackCount = bFirstPlayer ? 1 : 2;
				UnitUIData.mStatusEffects.Add(PreviewStatus);
			}
		}
#endif

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
	APlayerController* PlayerController = GetWorld() != nullptr
		? GetWorld()->GetFirstPlayerController() : nullptr;
	ACombatCameraPawn* CameraPawn = PlayerController != nullptr
		? PlayerController->GetPawn<ACombatCameraPawn>() : nullptr;
	UCameraMovementComponent* CameraMovement = CameraPawn != nullptr
		? CameraPawn->GetCameraMovementComponent() : nullptr;
	if (CameraMovement == nullptr)
	{
		return;
	}

	// INDEX_NONE 은 "그만 봐도 된다" 는 뜻이다. 바로 옮기는 방식이라
	// 되돌릴 것이 없다 -- 카메라는 사람이 마지막으로 본 자리에 그대로 둔다.
	if (UnitId == INDEX_NONE)
	{
		return;
	}

	UUnitModel* UnitModel = FindUnitModelById(UnitId);
	AActor* ViewActor = UnitModel != nullptr
		? UnitModel->GetView<AActor>() : nullptr;
	if (ViewActor == nullptr)
	{
		return;
	}
	/*
	 * 연출 없이 바로 옮긴다. 부드럽게 따라가면 판이 흐르듯 움직여 어지럽다는
	 * 검수가 있었다(0806). 강조로 잡으면 카메라 조작까지 잠겨서 안 쓴다.
	 *
	 * 액터 피벗(발밑)이 아니라 **몸통 가운데**를 넘긴다. 기운 카메라에서
	 * 발을 가운데 두면 몸은 화면 위로 밀려 "가운데가 아니다"로 읽힌다
	 * (0806 검수). 충돌 바운즈 중심이 치비 몸통의 가운데다.
	 *
	 * 놓을 화면 자리는 UI 가 세워 둔 앵커(0~1 비율)를 쓴다 -- "가운데" 는
	 * 화면 한가운데가 아니라 스킬 카드들이 둘러싼 자리다(0807 합의).
	 */
	FVector BoundsOrigin = ViewActor->GetActorLocation();
	FVector BoundsExtent = FVector::ZeroVector;
	ViewActor->GetActorBounds(/*bOnlyCollidingComponents=*/true,
		OUT BoundsOrigin, OUT BoundsExtent);

	int32 ViewX = 0;
	int32 ViewY = 0;
	PlayerController->GetViewportSize(OUT ViewX, OUT ViewY);
	const FVector2D AnchorFraction = mCombatUIModel != nullptr
		? mCombatUIModel->GetFocusScreenAnchor() : FVector2D(0.5f, 0.5f);
	CameraMovement->JumpToWorldPositionAtScreen(BoundsOrigin,
		FVector2D(AnchorFraction.X * ViewX, AnchorFraction.Y * ViewY));
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
			SkillUIData.mTargeting.mSelectShape = GetCombatSkillSelectShape(StaticSkillData->mAimPattern);
			SkillUIData.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRange);
			SkillUIData.mTargeting.mHitShape = GetCombatSkillHitShape(StaticSkillData->mEffectPattern);
			SkillUIData.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectArea);
			// 차단 레이어를 그대로 넘긴다. 예전에는 "비었나" 만 bool 로 넘겨서
			// 장애물만 막힘 / 유닛만 막힘 / 둘 다 막힘이 한 값으로 뭉개졌다.
			SkillUIData.mTargeting.mAimBlockerMask = StaticSkillData->mAimBlockerMask;
			SkillUIData.mTargeting.mEffectBlockerMask = StaticSkillData->mEffectBlockerMask;
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
	OutDetail.mTargeting.mSelectShape = GetCombatSkillSelectShape(StaticSkillData->mAimPattern);
	OutDetail.mTargeting.mSelectRange = StaticCast<float>(StaticSkillData->mAimRange);
	OutDetail.mTargeting.mHitShape = GetCombatSkillHitShape(StaticSkillData->mEffectPattern);
	OutDetail.mTargeting.mHitRange = StaticCast<float>(StaticSkillData->mEffectArea);
	// 차단 레이어가 비어 있으면 곡사/관통으로 표시
	OutDetail.mTargeting.mAimBlockerMask = StaticSkillData->mAimBlockerMask;
	OutDetail.mTargeting.mEffectBlockerMask = StaticSkillData->mEffectBlockerMask;
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
		mRewardUIModel->SetRewardChoices(Choices);
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

	switch (CurrentRoom.mType)
	{
	case ERoomType::EliteMonster:
		AddEquipmentReward(static_cast<const FEliteMonsterRoom&>(CurrentRoom).mRewardArtifactDataId);
		break;
	case ERoomType::BossMonster:
		break;
	default:
		break;
	}

	mRewardUIModel->SetRewardChoices(Choices);
}
