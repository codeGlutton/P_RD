#include "GameMode/TreasureGameMode.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "DataAsset/RoomSpawnData/StaticTreasureRoomSpawnData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "Setting/RDWorldSettings.h"
#include "Actor/Party/PartyModel.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "PCGStage/Room.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/Treasure/TreasureUIModel.h"
#include "UI/Treasure/TreasureUIWidgetBase.h"
#include "UI/FrontendMapWidget.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/ArtifactRewardPolicy.h"

#define LOCTEXT_NAMESPACE "TreasureGameMode"

namespace
{
	// TODO: 희귀도 색상 임시값 (인벤토리 GetInventoryRarityColor와 동일) -> 나중에 확정판에서 UI 값으로 수정
	FLinearColor GetTreasureRarityColor(ERarityType RarityType)
	{
		switch (RarityType)
		{
		case ERarityType::Rare:
			return FLinearColor(0.42f, 0.66f, 0.95f, 1.f);
		case ERarityType::Epic:
			return FLinearColor(0.72f, 0.46f, 0.92f, 1.f);
		case ERarityType::Common:
		default:
			return FLinearColor(0.72f, 0.78f, 0.75f, 1.f);
		}
	}

	/**
	 * @brief 아티팩트 효과 줄 조립. 중복 조립 대신 공용 조립기 결과를 이어 붙인다.
	 * @details 패시브 설명 수집과 순수 스탯 폴백은 SkillDetailUIBuilder 가 담당한다.
	 */
	FText GetTreasureArtifactDescription(const UStaticArtifactData* Data)
	{
		if (Data == nullptr)
		{
			return FText::GetEmpty();
		}
		FCombatArtifactUI Detail;
		SkillDetailUIBuilder::FillFromArtifactData(Data, Detail);
		if (Detail.mEffectDescriptions.IsEmpty())
		{
			return LOCTEXT("TreasureArtifactFallback", "파티 전체에 적용됩니다.");
		}
		TArray<FString> Lines;
		for (const FText& Line : Detail.mEffectDescriptions)
		{
			Lines.Add(Line.ToString());
		}
		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}
}

/** @brief 보상 연출 WBP 기본값. BP 디자이너가 파생 BP에서 교체할 수 있다. */
ATreasureGameMode::ATreasureGameMode()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> RewardWidgetFinder(
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless"));
	static ConstructorHelpers::FClassFinder<UUserWidget> RewardWidgetNoArtifactFinder(
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless_NoArtifact"));
	if (RewardWidgetFinder.Succeeded())
	{
		mRewardWidgetClass = RewardWidgetFinder.Class;
	}
	if (RewardWidgetNoArtifactFinder.Succeeded())
	{
		mRewardWidgetClassNoArtifact = RewardWidgetNoArtifactFinder.Class;
	}
}

void ATreasureGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FStage& CurStage = GetRunPersistData()->GetStage();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticStageSpawnData* StaticStageData = AssetManager->GetPrimaryAssetObject<UStaticStageSpawnData>(CurStage.mStaticStageSpawnDataId);
	checkf(StaticStageData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = StaticStageData->mTreasureRoomBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous(), false);
}

void ATreasureGameMode::InitializeRoom()
{
	Super::InitializeRoom();
	mOpened = false;
	mGoldRewardGranted = false;
	mGrantedArtifactIds.Reset();
	mFailedArtifactIds.Reset();

	SpawnTreasureBox();

	// 방 데이터가 준비된 뒤라야 밀어넣을 값이 있으므로 뷰모델은 여기서 생성
	if (mTreasureUIModel == nullptr)
	{
		mTreasureUIModel = NewObject<UTreasureUIModel>(this, TEXT("TreasureUIModel"));
		mTreasureUIModel->OnOpenRequested.AddUniqueDynamic(
			this, &ATreasureGameMode::HandleOpenRequested);
		mTreasureUIModel->OnLeaveRequested.AddUniqueDynamic(
			this, &ATreasureGameMode::HandleLeaveRequested);
	}
}

/** @brief 방이 열리면 화면을 뷰모델에 붙여 열고 개봉 전 상태를 한 번 내림 */
void ATreasureGameMode::BeginRoom()
{
	Super::BeginRoom();

	if (OpenRewardPresentation())
	{
		return;
	}

	// 공용 보상 WBP를 불러오지 못했을 때만 기존 보물방 HUD로 안전하게 폴백한다.
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem =
		GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (URDUserWidget* TreasureHUD =
			WorldWidgetSubsystem->GetHUD<URDUserWidget>())
		{
			if (UTreasureUIWidgetBase* TreasureUIWidget =
				Cast<UTreasureUIWidgetBase>(TreasureHUD))
			{
				TreasureUIWidget->BindUIModel(mTreasureUIModel);
			}
			TreasureHUD->OpenUI();
		}
	}
	PushTreasureUIData();
}

bool ATreasureGameMode::OpenRewardPresentation()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World != nullptr
		? World->GetFirstPlayerController() : nullptr;
	const URunPersistData* RunPersistData = GetRunPersistData();
	if (World == nullptr || PlayerController == nullptr || RunPersistData == nullptr)
	{
		return false;
	}
	const FRoom& CurrentRoom = RunPersistData->GetCurrentRoom();
	if (CurrentRoom.mType != ERoomType::Treasure)
	{
		return false;
	}
	const FTreasureRoom& TreasureRoom =
		static_cast<const FTreasureRoom&>(CurrentRoom);
	const FPrimaryAssetId ArtifactId =
		TreasureRoom.mRewardArtifactDataId;
	const bool bHasArtifact = ArtifactId.IsValid() == false;
	// 클래스 지정은 BP 디자이너 몫(EditDefaultsOnly) -- 코드에는 경로 리터럴을 두지 않는다.
	UClass* WidgetClass = bHasArtifact
		? mRewardWidgetClass.Get() : mRewardWidgetClassNoArtifact.Get();
	if (WidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning,
			TEXT("보물방 공용 보상 WBP 클래스 미설정 (bHasArtifact=%d)"), bHasArtifact);
		return false;
	}

	mRewardUIModel = NewObject<URewardUIModel>(this, TEXT("TreasureRewardUIModel"));
	FRewardUI Reward;
	Reward.mTitle = LOCTEXT("TreasureRewardTitle", "보상");
	Reward.mGoldGained = FMath::Max(0, TreasureRoom.mRewardMoney);
	Reward.mGoldBalance = GetPartyGold() + Reward.mGoldGained;
	mRewardUIModel->SetReward(Reward);

	TArray<FRewardChoiceUI> Choices;
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	{
		FRewardChoiceUI Choice;
		Choice.mChoiceIndex = 0;
		Choice.mKind = ERewardChoiceKind::Artifact;
		Choice.mSourceAssetId = ArtifactId;
		Choice.mName = FText::FromName(ArtifactId.PrimaryAssetName);
		if (AssetManager != nullptr)
		{
			if (const UStaticArtifactData* Data =
				AssetManager->GetPrimaryAssetObject<UStaticArtifactData>(ArtifactId))
			{
				if (!Data->mName.IsEmpty())
				{
					Choice.mName = Data->mName;
				}
				Choice.mIcon = Data->mIcon.LoadSynchronous();
				Choice.mDescription = GetTreasureArtifactDescription(Data);
				Choice.mRarityColor = GetTreasureRarityColor(Data->mRarityType);
				Choice.mRarityName = StaticEnum<ERarityType>() != nullptr
					? StaticEnum<ERarityType>()->GetDisplayNameTextByValue(
						StaticCast<int64>(Data->mRarityType))
					: FText::GetEmpty();
				Choice.mRarityLevel = StaticCast<int32>(Data->mRarityType);
			}
		}
		Choices.Add(Choice);
	}
	FRewardGrantBundleUI GrantBundle;
	GrantBundle.mItems = MoveTemp(Choices);
	mRewardUIModel->SetGrantBundle(GrantBundle);
	mRewardUIModel->OnRewardClaimRequested.AddUniqueDynamic(
		this, &ATreasureGameMode::HandleRewardClaimRequested);
	mRewardUIModel->OnRewardGrantBundleRequested.AddUniqueDynamic(
		this, &ATreasureGameMode::HandleRewardGrantBundleRequested);

	mRewardWidget = CreateWidget<URewardConcept03Widget>(
		PlayerController, WidgetClass);
	if (mRewardWidget == nullptr)
	{
		mRewardUIModel = nullptr;
		return false;
	}
	mRewardWidget->BindUIModel(mRewardUIModel);
	mRewardWidget->OnRewardFlowCompleted.AddUniqueDynamic(
		this, &ATreasureGameMode::HandleRewardPresentationCompleted);
	mRewardWidget->AddToViewport(60);
	mRewardWidget->ResetRewardFlow();
	// 보물방은 EXP가 없으므로 첫 '다음' 입력을 자동 수행해 상자 단계부터 시작한다.
	mRewardWidget->AdvanceRewardFlow();

	if (UWorldWidgetSubsystem* WorldWidgetSubsystem =
		World->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (URDUserWidget* TreasureHUD =
			WorldWidgetSubsystem->GetHUD<URDUserWidget>())
		{
			TreasureHUD->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return true;
}

/**
 * @brief 상자 상태와 지급 내역을 화면에 내림
 *
 * @details
 * 개봉 전에는 상자 상태만 내리고 보상은 공개하지 않음.
 * 개봉 후에는 골드 카드 + 지급에 성공한 아티팩트 카드를 목록으로 내림.
 */
void ATreasureGameMode::PushTreasureUIData()
{
	if (mTreasureUIModel == nullptr)
	{
		return;
	}

	FTreasureUI TreasureUIData;
	TreasureUIData.mIsOpened = mOpened;

	// 개봉 전에는 보상 비공개
	if (mOpened == false)
	{
		mTreasureUIModel->SetTreasure(TreasureUIData);
		return;
	}

	// 방 데이터 다운캐스트 (FRoom은 갈래가 mType에 적혀 있고 알맹이는 파생 구조체에 있음)
	const URunPersistData* RunPersistData = GetRunPersistData();
	const FTreasureRoom* TreasureRoom = nullptr;
	if (RunPersistData != nullptr)
	{
		const FRoom& CurrentRoom = RunPersistData->GetCurrentRoom();
		if (CurrentRoom.mType == ERoomType::Treasure)
		{
			TreasureRoom = &static_cast<const FTreasureRoom&>(CurrentRoom);
		}
	}
	if (TreasureRoom == nullptr)
	{
		mTreasureUIModel->SetTreasure(TreasureUIData);
		return;
	}

	int32 SlotIndex = 0;

	// 골드 카드
	if (TreasureRoom->mRewardMoney > 0)
	{
		FTreasureItemUI GoldItem;
		GoldItem.mSlotIndex = SlotIndex++;
		GoldItem.mKind = ETreasureItemKind::Gold;
		GoldItem.mName = LOCTEXT("TreasureGoldName", "골드");
		GoldItem.mAmount = TreasureRoom->mRewardMoney;
		TreasureUIData.mItems.Add(GoldItem);
	}

	// 아티팩트 카드 (지급 성공분만, 로드 실패 시 에셋명 폴백)
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	for (const FPrimaryAssetId& ArtifactId : mGrantedArtifactIds)
	{
		FTreasureItemUI Item;
		Item.mSlotIndex = SlotIndex++;
		Item.mKind = ETreasureItemKind::Artifact;
		Item.mName = FText::FromName(ArtifactId.PrimaryAssetName);

		if (AssetManager != nullptr)
		{
			if (const UStaticArtifactData* Data = AssetManager->GetPrimaryAssetObject<UStaticArtifactData>(ArtifactId))
			{
				// 데이터 이름이 비어 있으면 에셋명 폴백 유지
				if (Data->mName.IsEmpty() == false)
				{
					Item.mName = Data->mName;
				}
				Item.mIcon = Data->mIcon.LoadSynchronous();
				Item.mRarityColor = GetTreasureRarityColor(Data->mRarityType);
			}
		}
		TreasureUIData.mItems.Add(Item);
	}

	mTreasureUIModel->SetTreasure(TreasureUIData);
}

/**
 * @brief 상자 개봉 및 보상 전부 지급
 * @details 재개봉 불가. 아티팩트 지급 실패는 로그만 남기고 나머지 지급은 계속 진행
 */
void ATreasureGameMode::HandleOpenRequested()
{
	if (mOpened == true)
	{
		return;
	}

	if (GetRunPersistData() == nullptr)
	{
		return;
	}

	FRewardGrantBundleResultUI IgnoredResult;
	GrantTreasureRewards(IgnoredResult);
}

bool ATreasureGameMode::GrantTreasureGold()
{
	if (mGoldRewardGranted)
	{
		return true;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr
		|| RunPersistData->GetCurrentRoom().mType != ERoomType::Treasure)
	{
		return false;
	}

	const FTreasureRoom& TreasureRoom = static_cast<const FTreasureRoom&>(
		RunPersistData->GetCurrentRoom());
	GivePartyGold(TreasureRoom.mRewardMoney);
	mGoldRewardGranted = true;
	return true;
}

FRewardGrantBundleResultUI ATreasureGameMode::GrantTreasureArtifactBundle()
{
	FRewardGrantBundleResultUI Result;
	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr
		|| RunPersistData->GetCurrentRoom().mType != ERoomType::Treasure)
	{
		return Result;
	}

	const FTreasureRoom& TreasureRoom = static_cast<const FTreasureRoom&>(
		RunPersistData->GetCurrentRoom());
	const FPrimaryAssetId ArtifactId =
		TreasureRoom.mRewardArtifactDataId;
	UPartyModel* PartyModel = GetPartyModel();
	UPartyArtifactComponentModel* ArtifactModel = PartyModel != nullptr
		? PartyModel->GetPartyArtifactComponentModel() : nullptr;

	Result = ArtifactRewardPolicy::GrantOne(
		ArtifactId,
		[ArtifactModel](const FPrimaryAssetId& ArtifactId)
		{
			return ArtifactModel != nullptr
				&& ArtifactModel->AddArtifact(ArtifactId);
		});

	mGrantedArtifactIds = Result.mGrantedItemIds;
	mFailedArtifactIds = Result.mFailedItemIds;
	for (const FPrimaryAssetId& ArtifactId : Result.mFailedItemIds)
	{
		UE_LOG(LogRD, Log, TEXT("보물방 아티팩트 지급 실패: %s"),
			*ArtifactId.ToString());
	}
	return Result;
}

void ATreasureGameMode::GrantTreasureRewards(
	OUT FRewardGrantBundleResultUI& OutResult)
{
	if (mOpened)
	{
		OutResult.mGrantedItemIds = mGrantedArtifactIds;
		OutResult.mFailedItemIds = mFailedArtifactIds;
		return;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr
		|| RunPersistData->GetCurrentRoom().mType != ERoomType::Treasure)
	{
		return;
	}
	const FTreasureRoom& TreasureRoom = static_cast<const FTreasureRoom&>(
		RunPersistData->GetCurrentRoom());
	const int32 PreGold = GetPartyGold();
	GrantTreasureGold();
	OutResult = GrantTreasureArtifactBundle();

	// 전체 지급 처리가 끝난 뒤에만 구형 HUD와 다음 방 전환이 보상 완료로 본다.
	mOpened = true;
	PushTreasureUIData();

	UE_LOG(LogRD, Log,
		TEXT("보물상자 개봉: 골드 %d 지급 (잔액 %d → %d), 아티팩트 %d/%d개 지급"),
		TreasureRoom.mRewardMoney, PreGold, GetPartyGold(),
		OutResult.mGrantedItemIds.Num(),
		OutResult.mGrantedItemIds.Num() + OutResult.mFailedItemIds.Num());
}

void ATreasureGameMode::HandleRewardGrantBundleRequested()
{
	if (mOpened || GetRunPersistData() == nullptr)
	{
		return;
	}

	GrantTreasureGold();
	FRewardGrantBundleResultUI Result = GrantTreasureArtifactBundle();
	mOpened = true;
	PushTreasureUIData();
	if (mRewardUIModel != nullptr)
	{
		mRewardUIModel->ConfirmGrantBundle(Result);
	}
}

/** @brief 나가기 의도 처리. 다음 방 선택은 지도(월드맵) 담당 */
void ATreasureGameMode::HandleLeaveRequested()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("보물방 나가기 실패: WorldWidgetSubsystem 없음"));
		return;
	}
	UFrontendMapWidget* MapWidget =
		WorldWidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
			EWorldWidgetType::WorldMap);
	if (MapWidget == nullptr)
	{
		WorldWidgetSubsystem->InitWorldWidget(EWorldWidgetType::WorldMap);
		MapWidget = WorldWidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
			EWorldWidgetType::WorldMap);
	}
	if (MapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("보물방 나가기 실패: WorldMap 위젯 미설정"));
		return;
	}
	MapWidget->SetRoomSelectionEnabled(true);
	MapWidget->ClearMapStatusOverride();
	MapWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(
		MapWidget, [](UUserWidget* OpenedWidget)
		{
			if (UFrontendMapWidget* OpenedMapWidget =
				Cast<UFrontendMapWidget>(OpenedWidget))
			{
				OpenedMapWidget->RefreshMap();
			}
		}));
	MapWidget->RefreshMap();
}

void ATreasureGameMode::HandleRewardClaimRequested(
	const ERewardClaimKind ClaimKind, const int32 ChoiceIndex)
{
	if (ClaimKind == ERewardClaimKind::Gold)
	{
		if (GrantTreasureGold() && mRewardUIModel != nullptr)
		{
			mRewardUIModel->ConfirmRewardClaim(ClaimKind, ChoiceIndex);
		}
		return;
	}

	// 구형 Settlement 위젯은 Choice 한 건을 직접 요청할 수 있다. 신규
	// Concept03은 GrantAll delegate를 사용하므로 이 호환 경로만 전체 지급
	// 어댑터를 유지한다.
	if (ClaimKind == ERewardClaimKind::Choice && !mOpened)
	{
		FRewardGrantBundleResultUI Result;
		GrantTreasureRewards(Result);
		if (mRewardUIModel != nullptr)
		{
			mRewardUIModel->ConfirmRewardClaim(ClaimKind, ChoiceIndex);
		}
	}
}

void ATreasureGameMode::HandleRewardPresentationCompleted(int32 ArtifactIndex)
{
	if (!mOpened)
	{
		HandleOpenRequested();
	}
	if (mRewardWidget != nullptr)
	{
		mRewardWidget->RemoveFromParent();
		mRewardWidget = nullptr;
	}
	HandleLeaveRequested();
}

/** @brief 파티 골드 지급. 0 이하 금액은 무시 */
void ATreasureGameMode::GivePartyGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	if (UPartyModel* PartyModel = GetPartyModel())
	{
		if (UAttributeSetComponentModel* Attributes =
			PartyModel->GetAttributeComponentModel())
		{
			Attributes->ApplyModToAttribute(
				UPartyAttributeSet::GetMoneyAttribute(), ETacticalModOp::AddBase,
				StaticCast<float>(Amount));
		}
	}
}

/** @brief 현재 파티 골드 */
int32 ATreasureGameMode::GetPartyGold() const
{
	UPartyModel* PartyModel = GetPartyModel();
	UAttributeSetComponentModel* Attributes = PartyModel != nullptr
		? PartyModel->GetAttributeComponentModel() : nullptr;
	if (Attributes == nullptr)
	{
		return 0;
	}
	return FMath::RoundToInt(Attributes->GetAttributeCurrentValue(
		UPartyAttributeSet::GetMoneyAttribute()));
}

void ATreasureGameMode::SpawnTreasureBox()
{
	checkf(mTreasureBox == nullptr, TEXT("이미 보물상자 존재"));

	// 방 스폰데이터에서 상자 클래스 조회
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	const FRoom& CurrentRoom = GetRunPersistData()->GetCurrentRoom();
	UStaticTreasureRoomSpawnData* SpawnData = AssetManager->GetPrimaryAssetObject<UStaticTreasureRoomSpawnData>(CurrentRoom.mStaticRoomSpawnDataId);
	if (SpawnData == nullptr || SpawnData->mTreasureBoxClass.IsNull() == true)
	{
		return;
	}

	// 스폰 세팅의 스타트포인트 트랜스폼 기준으로 상자 배치
	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RD 월드 세팅 nullptr"));
	FTransform SpawnPointTransform = FTransform::Identity;
	AActor* SettingPointActor = WorldSettings->GetRoomStartPoint(GetRoomSpawnSettingName());
	if (SettingPointActor != nullptr)
	{
		SpawnPointTransform = SettingPointActor->GetActorTransform();
	}

	UClass* TreasureBoxClass = SpawnData->mTreasureBoxClass.LoadSynchronous();
	if (TreasureBoxClass != nullptr)
	{
		mTreasureBox = GetWorld()->SpawnActor<AActor>(TreasureBoxClass, SpawnPointTransform);
	}
}

#undef LOCTEXT_NAMESPACE
