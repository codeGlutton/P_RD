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
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "PCGStage/Room.h"
#include "UI/Shop/ShopUIModel.h"

namespace
{
	// TODO: 희귀도 색상 임시값 (인벤토리 GetInventoryRarityColor와 동일) -> 나중에 확정판에서 용수님의 UI 값으로 수정
	FLinearColor GetShopRarityColor(ERarityType RarityType)
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
		mShopUIModel->OnDiscardArtifactRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleDiscardArtifactRequested);
		mShopUIModel->OnDiscardSkillRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleDiscardSkillRequested);
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

	// 소지품(버리기 대상)은 방 데이터와 무관하게 항상 채운다.
	FillOwnedItems(ShopUIData);

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
				// 아티펙트는 전용 자산에서 이름과 아이콘을 읽는다. 장비 폴백 없음.
				else if (Kind == EShopItemKind::Artifact)
				{
					if (const UStaticArtifactData* Data =
						AssetManager->GetPrimaryAssetObject<UStaticArtifactData>(ItemId))
					{
						Item.mName = Data->mName;
						Item.mIcon = Data->mIcon.LoadSynchronous();
					}
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
	AddList(ShopRoom->mSaleArtifactDataItems, EShopItemKind::Artifact);

	mShopUIModel->SetShop(ShopUIData);
}

/**
 * @brief 소지품(버리기 대상)을 표시값으로 채움
 * @details
 * 아티펙트는 파티 공용 배열의 index, 스킬은 (유닛 index, 슬롯 index) 쌍이
 * 그대로 버리기 요청 payload가 된다.
 * @param ShopUIData 채울 스냅샷
 */
void AShopGameMode::FillOwnedItems(FShopUI& ShopUIData) const
{
	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		return;
	}

	// 파티 공용 아티펙트
	if (const UPartyArtifactComponentModel* ArtifactModel = PartyModel->GetPartyArtifactComponentModel())
	{
		const TArray<TObjectPtr<UStaticArtifactData>>& Artifacts = ArtifactModel->GetPartyArtifacts();
		for (int32 ArtifactIndex = 0; ArtifactIndex < Artifacts.Num(); ++ArtifactIndex)
		{
			const UStaticArtifactData* Data = Artifacts[ArtifactIndex];
			if (Data == nullptr)
			{
				continue;
			}

			FShopOwnedArtifactUI OwnedArtifact;
			OwnedArtifact.mArtifactIndex = ArtifactIndex;
			OwnedArtifact.mName = Data->mName;
			OwnedArtifact.mIcon = Data->mIcon.LoadSynchronous();
			OwnedArtifact.mRarityColor = GetShopRarityColor(Data->mRarityType);
			ShopUIData.mOwnedArtifacts.Add(OwnedArtifact);
		}
	}

	// 유닛별 소지 스킬 (파티는 3슬롯 고정 + 빈 슬롯 허용)
	const TArray<TObjectPtr<UPlayerUnitModel>>& UnitModels = PartyModel->GetPlayerUnitModels();
	for (int32 UnitIndex = 0; UnitIndex < UnitModels.Num(); ++UnitIndex)
	{
		const UPlayerUnitModel* UnitModel = UnitModels[UnitIndex];
		USkillComponentModel* SkillModel = UnitModel != nullptr
			? UnitModel->GetSkillComponentModel() : nullptr;
		if (SkillModel == nullptr)
		{
			continue;
		}

		const TArray<FSkillEntry>& Skills = SkillModel->GetSkills();
		for (int32 SlotIndex = 0; SlotIndex < Skills.Num(); ++SlotIndex)
		{
			// 스킬이 존재하면 버릴 수 있는 유효한 상태로 본다
			const UStaticSkillData* Data = Skills[SlotIndex].mData;
			if (Data == nullptr)
			{
				continue;
			}

			FShopOwnedSkillUI OwnedSkill;
			OwnedSkill.mUnitIndex = UnitIndex;
			OwnedSkill.mSlotIndex = SlotIndex;
			OwnedSkill.mName = Data->mName;
			OwnedSkill.mIcon = Data->mIcon.LoadSynchronous();
			ShopUIData.mOwnedSkills.Add(OwnedSkill);
		}
	}
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

/**
 * @brief 소지 아티펙트 하나를 버림 (0골드 거래)
 * @param ArtifactIndex 파티 공용 아티펙트 배열 index (FShopOwnedArtifactUI가 돌려준 값)
 */
void AShopGameMode::HandleDiscardArtifactRequested(int32 ArtifactIndex)
{
	UPartyModel* PartyModel = GetPartyModel();
	UPartyArtifactComponentModel* ArtifactModel = PartyModel != nullptr
		? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	if (ArtifactModel == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UStaticArtifactData>>& Artifacts = ArtifactModel->GetPartyArtifacts();
	if (Artifacts.IsValidIndex(ArtifactIndex) == false || Artifacts[ArtifactIndex] == nullptr)
	{
		return;
	}

	// RemoveArtifact가 전 구성원 해제 배포와 저장 추적(OnChangeArtifact)까지 처리한다
	ArtifactModel->RemoveArtifact(Artifacts[ArtifactIndex]);
	PushShopUIData();
}

/**
 * @brief 유닛의 스킬 슬롯 하나를 버림 (0골드 거래)
 * @param UnitIndex 파티 유닛 슬롯 index (FShopOwnedSkillUI가 돌려준 값)
 * @param SlotIndex 유닛 내 스킬 슬롯 index
 */
void AShopGameMode::HandleDiscardSkillRequested(int32 UnitIndex, int32 SlotIndex)
{
	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		return;
	}

	// 파티원 인덱스와 유효성을 미리 검사
	const TArray<TObjectPtr<UPlayerUnitModel>>& UnitModels = PartyModel->GetPlayerUnitModels();
	if (UnitModels.IsValidIndex(UnitIndex) == false || UnitModels[UnitIndex] == nullptr)
	{
		return;
	}
	USkillComponentModel* SkillModel = UnitModels[UnitIndex]->GetSkillComponentModel();
	if (SkillModel == nullptr)
	{
		return;
	}
	const TArray<FSkillEntry>& Skills = SkillModel->GetSkills();
	if (Skills.IsValidIndex(SlotIndex) == false || Skills[SlotIndex].mData == nullptr)
	{
		return;
	}

	// 슬롯 비우기 (OnChangeSkillUI에서 저장 처리)
	SkillModel->RemoveSkill(SlotIndex);
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
