#include "GameMode/ShopGameMode.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "Simulation/Factory/ObjectModelFactory.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Setting/RDWorldSettings.h"
#include "Actor/Party/PartyModel.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "PCGStage/Room.h"
#include "UI/Shop/ShopUIModel.h"

void AShopGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FStage& CurStage = GetRunPersistData()->GetStage();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticStageSpawnData* StaticStageData = AssetManager->GetPrimaryAssetObject<UStaticStageSpawnData>(CurStage.mStaticStageSpawnDataId);
	checkf(StaticStageData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = StaticStageData->mShopRoomBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous(), false);
}

void AShopGameMode::InitializeRoom()
{
	Super::InitializeRoom();

	SpawnTileMap();

	// 뷰모델을 여기서 만든다. 방이 준비된 뒤라야 파티와 방 데이터를 읽을 수
	// 있다 -- 생성자에서 만들면 밀어 넣을 값이 아직 없다.
	if (mShopUIModel == nullptr)
	{
		mShopUIModel = NewObject<UShopUIModel>(this, TEXT("ShopUIModel"));
		mShopUIModel->OnBuyRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleBuyRequested);
		mShopUIModel->OnLeaveRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleLeaveRequested);
	}
}

/** @brief 방이 열리면 파는 것을 한 번 내린다. */
void AShopGameMode::BeginRoom()
{
	Super::BeginRoom();

	PushShopUIData();
}

/**
 * @brief 지금 파는 것과 가진 돈을 화면에 내린다.
 *
 * @details
 * 파는 것은 방 데이터에 자리가 넷으로 나뉘어 있다 -- 직업별 기술, 공용 기술,
 * 장비. 화면은 한 줄로 늘어놓으므로 여기서 이어 붙이고 **자리 번호**를 매긴다.
 * 그 번호가 곧 사겠다고 돌려보낼 값이다.
 *
 * 살 수 있나(mIsAffordable)는 여기서 가린다. 화면이 돈과 값을 견주면 규칙이
 * 두 곳에 생긴다.
 */
void AShopGameMode::PushShopUIData() const
{
	if (mShopUIModel == nullptr)
	{
		return;
	}

	FShopUI ShopUIData;
	ShopUIData.mGold = GetPartyGold();

	// 방을 갈래로 가른다. 전투 쪽이 GetMonsterRewardRoom 으로 하는 것과 같다 --
	// FRoom 은 갈래가 mType 에 적혀 있고 실제 알맹이는 파생 구조체에 있다.
	const URunPersistData* RunPersistData = GetRunPersistData();
	const FShopRoom* ShopRoom = nullptr;
	if (RunPersistData != nullptr)
	{
		const FRoom& CurrentRoom = RunPersistData->GetCurrentRoom();
		if (CurrentRoom.mType == ERoomType::Shop)
		{
			ShopRoom = &static_cast<const FShopRoom&>(CurrentRoom);
		}
	}
	if (ShopRoom == nullptr)
	{
		mShopUIModel->SetShop(ShopUIData);
		return;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	int32 SlotIndex = 0;

	auto AddList = [&](const FShopItemList& List, EShopItemKind Kind)
		{
			for (const FPrimaryAssetId& ItemId : List.mSaleItemIds)
			{
				const int32 Slot = SlotIndex++;
				if (ItemId.IsValid() == false || AssetManager == nullptr)
				{
					continue;
				}

				FShopItemUI Item;
				Item.mSlotIndex = Slot;
				Item.mKind = Kind;
				Item.mIsSoldOut = mSoldSlots.Contains(Slot);
				// 값이 아직 자산에 없다. 붙는 대로 여기서 읽는다.
				// [합의필요] 값을 자산에 둘 것인가 표로 둘 것인가.
				Item.mPrice = 0;
				Item.mName = FText::FromName(ItemId.PrimaryAssetName);

				if (Kind == EShopItemKind::Skill)
				{
					if (const UStaticSkillData* Data =
						AssetManager->GetPrimaryAssetObject<UStaticSkillData>(ItemId))
					{
						Item.mName = Data->mName;
						Item.mIcon = Data->mIcon.LoadSynchronous();
					}
				}
				else if (const UStaticEquipmentData* Data =
					AssetManager->GetPrimaryAssetObject<UStaticEquipmentData>(ItemId))
				{
					Item.mName = Data->mName;
				}

				Item.mIsAffordable = Item.mIsSoldOut == false
					&& Item.mPrice <= ShopUIData.mGold;
				ShopUIData.mItems.Add(Item);
			}
		};

	for (const FShopItemList& List : ShopRoom->mSaleJobSkillDataItems)
	{
		AddList(List, EShopItemKind::Skill);
	}
	AddList(ShopRoom->mSaleCommonSkillDataItems, EShopItemKind::Skill);
	AddList(ShopRoom->mSaleEquipmentDataItems, EShopItemKind::Equipment);

	mShopUIModel->SetShop(ShopUIData);
}

/**
 * @brief 한 칸을 샀다.
 * @param SlotIndex 화면이 돌려보낸 자리 번호
 */
void AShopGameMode::HandleBuyRequested(int32 SlotIndex)
{
	if (mShopUIModel == nullptr || mSoldSlots.Contains(SlotIndex) == true)
	{
		return;
	}

	const TArray<FShopItemUI>& Items = mShopUIModel->GetShop().mItems;
	const FShopItemUI* Item = Items.FindByPredicate(
		[SlotIndex](const FShopItemUI& Candidate)
		{ return Candidate.mSlotIndex == SlotIndex; });
	if (Item == nullptr || Item->mIsAffordable == false)
	{
		return;
	}

	if (Item->mPrice > 0)
	{
		if (UPartyModel* PartyModel = GetPartyModel())
		{
			if (UAttributeSetComponentModel* Attributes =
				PartyModel->GetAttributeComponentModel())
			{
				Attributes->ApplyModToAttribute(
					UPartyAttributeSet::GetMoneyAttribute(), ETacticalModOp::AddBase,
					-StaticCast<float>(Item->mPrice));
			}
		}
	}

	// [합의필요] 산 것을 어디에 넣나. 기술은 스킬 칸, 장비는 인벤토리인데
	// 둘 다 "누구에게" 가 안 정해졌다. 지금은 품절만 매긴다.
	mSoldSlots.Add(SlotIndex);
	PushShopUIData();
}

/** @brief 나간다. 다음 방을 고르는 것은 지도가 한다. */
void AShopGameMode::HandleLeaveRequested()
{
	// [합의필요] 상점을 나가면 바로 지도인가, 방 안에 서 있다가 나가는가.
	// 지금은 지도를 여는 길이 방 HUD 에만 있어 여기서 할 일이 없다.
}

/** @brief 지금 가진 돈. */
int32 AShopGameMode::GetPartyGold() const
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

void AShopGameMode::SpawnTileMap()
{
	checkf(mTileMap == nullptr, TEXT("이미 타일 존재"));

	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RD 월드 세팅 nullptr"));

	// 스폰 세팅의 스타트포인트 트랜스폼 기준으로 타일맵 배치
	FTransform SpawnPointTransform = FTransform::Identity;
	AActor* SettingPointActor = WorldSettings->GetRoomStartPoint(GetRoomSpawnSettingName());
	if (SettingPointActor != nullptr)
	{
		SpawnPointTransform = SettingPointActor->GetActorTransform();
	}

	// 모델 팩토리가 모델과 뷰(ATileMap)를 함께 스폰
	mTileMap = GetWorldModelFactory(this)->NewModel<UTileMapModel>(SpawnPointTransform);
	checkf(mTileMap != nullptr, TEXT("타일맵 모델 생성 실패"));

	// 유닛 배치 전에 모델이 타일 저장소를 직접 빌드
	mTileMap->RebuildTiles();
}
