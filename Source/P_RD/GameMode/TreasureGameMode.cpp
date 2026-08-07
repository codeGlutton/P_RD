#include "GameMode/TreasureGameMode.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "DataAsset/RoomSpawnData/StaticTreasureRoomSpawnData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "Setting/RDWorldSettings.h"
#include "Actor/Party/PartyModel.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "PCGStage/Room.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/Treasure/TreasureUIModel.h"
#include "UI/Treasure/TreasureUIWidgetBase.h"

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

	// 모델은 게임모드가 위젯에 건네줌 (위젯이 게임모드를 역참조하면 UI에 게임플레이 의존이 생김)
	// 붙인 뒤 열어야 첫 갱신 시점에 모델이 있어 빈 화면이 스치지 않음
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (URDUserWidget* TreasureHUD = WorldWidgetSubsystem->GetHUD<URDUserWidget>())
		{
			if (UTreasureUIWidgetBase* TreasureUIWidget = Cast<UTreasureUIWidgetBase>(TreasureHUD))
			{
				TreasureUIWidget->BindUIModel(mTreasureUIModel);
			}
			TreasureHUD->OpenUI();
		}
	}

	PushTreasureUIData();
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
	// 재개봉 가드
	if (mOpened == true)
	{
		return;
	}

	// 방 데이터 다운캐스트
	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr)
	{
		return;
	}
	const FRoom& CurrentRoom = RunPersistData->GetCurrentRoom();
	if (CurrentRoom.mType != ERoomType::Treasure)
	{
		return;
	}
	const FTreasureRoom& TreasureRoom = static_cast<const FTreasureRoom&>(CurrentRoom);

	// 골드 지급 (실패 케이스 없음)
	const int32 PreGold = GetPartyGold();
	GivePartyGold(TreasureRoom.mRewardMoney);

	// 아티팩트 지급 (파티 전원 배포와 저장 추적은 컴포넌트가 처리)
	mGrantedArtifactIds.Reset();
	UPartyModel* PartyModel = GetPartyModel();
	UPartyArtifactComponentModel* ArtifactModel = PartyModel != nullptr
		? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	for (const FPrimaryAssetId& ArtifactId : TreasureRoom.mRewardArtifactDataIds)
	{
		if (ArtifactModel != nullptr && ArtifactModel->AddArtifact(ArtifactId) == true)
		{
			mGrantedArtifactIds.Add(ArtifactId);
		}
		else
		{
			UE_LOG(LogRD, Log, TEXT("보물방 아티팩트 지급 실패: %s"), *ArtifactId.ToString());
		}
	}

	// 개봉 확정 후 지급 내역 공개
	mOpened = true;
	PushTreasureUIData();

	UE_LOG(LogRD, Log, TEXT("보물상자 개봉: 골드 %d 지급 (잔액 %d → %d), 아티팩트 %d/%d개 지급"),
		TreasureRoom.mRewardMoney, PreGold, GetPartyGold(),
		mGrantedArtifactIds.Num(), TreasureRoom.mRewardArtifactDataIds.Num());
}

/** @brief 나가기 의도 처리. 다음 방 선택은 지도(월드맵) 담당 */
void ATreasureGameMode::HandleLeaveRequested()
{
	// [합의필요] 보물방을 나가면 바로 지도인가, 방 안에 서 있다가 나가는가.
	// 지금은 지도를 여는 길이 방 HUD에만 있어 여기서 할 일이 없다.
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
