#include "GameMode/ShopGameMode.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "DataAsset/StageSpawnData/StaticStageSpawnData.h"
#include "Simulation/Factory/ObjectModelFactory.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Setting/RDWorldSettings.h"
#include "Actor/Party/PartyModel.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "PCGStage/Room.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/Shop/ShopUIWidgetBase.h"
#include "UI/FrontendMapWidget.h"

namespace
{
	constexpr int32 ShopRestPrice = 100;
	constexpr int32 ReplaceableSkillStartIndex = 2;
	constexpr int32 ReplaceableSkillSlotCount = 4;

	FText ShopMercenaryName(const EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight: return NSLOCTEXT("ShopGameMode", "Knight", "기사");
		case EUnitJobType::Mage: return NSLOCTEXT("ShopGameMode", "Mage", "마법사");
		case EUnitJobType::Ranger: return NSLOCTEXT("ShopGameMode", "Ranger", "레인저");
		case EUnitJobType::Rogue: return NSLOCTEXT("ShopGameMode", "Rogue", "도적");
		case EUnitJobType::Barbarian: return NSLOCTEXT("ShopGameMode", "Barbarian", "야만전사");
		case EUnitJobType::Druid: return NSLOCTEXT("ShopGameMode", "Druid", "드루이드");
		default: return NSLOCTEXT("ShopGameMode", "UnknownMercenary", "용병");
		}
	}

	FText ShopMercenaryRole(const EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight: return NSLOCTEXT("ShopGameMode", "KnightRole", "방패 탱커 · 근접");
		case EUnitJobType::Mage: return NSLOCTEXT("ShopGameMode", "MageRole", "주문 술사 · 원거리");
		case EUnitJobType::Ranger: return NSLOCTEXT("ShopGameMode", "RangerRole", "명사수 · 원거리");
		case EUnitJobType::Rogue: return NSLOCTEXT("ShopGameMode", "RogueRole", "기습 암살자 · 근접");
		case EUnitJobType::Barbarian: return NSLOCTEXT("ShopGameMode", "BarbarianRole", "광전사 · 근접");
		case EUnitJobType::Druid: return NSLOCTEXT("ShopGameMode", "DruidRole", "자연 술사 · 지원");
		default: return FText::GetEmpty();
		}
	}

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
		mShopUIModel->OnBuySkillRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleBuySkillRequested);
		mShopUIModel->OnHireMercenaryRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleHireMercenaryRequested);
		mShopUIModel->OnDiscardArtifactRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleDiscardArtifactRequested);
		mShopUIModel->OnDiscardSkillRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleDiscardSkillRequested);
		mShopUIModel->OnRestRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleRestRequested);
		mShopUIModel->OnLeaveRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleLeaveRequested);
		mShopUIModel->OnItemDetailRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleItemDetailRequested);
		mShopUIModel->OnOwnedSkillDetailRequested.AddUniqueDynamic(
			this, &AShopGameMode::HandleOwnedSkillDetailRequested);
	}
}

/** @brief 방이 열리면 파는 것을 한 번 내리고, HUD로 지정된 상점 화면에 뷰모델을 물려 연다. */
void AShopGameMode::BeginRoom()
{
	Super::BeginRoom();

	PushShopUIData();

	// HUD(mHUDClass)가 상점 화면이면 뷰모델을 붙이고 나서 연다
	// — 열면서 도는 첫 갱신에 모델이 있어야 빈 화면이 한 프레임 스치지 않음.
	// HUD 미설정이거나 다른 위젯이면 생략 (RD.ShopPreview 콘솔 경로는 그대로 동작)
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UShopUIWidgetBase* ShopHUD = WorldWidgetSubsystem->GetHUD<UShopUIWidgetBase>())
		{
			ShopHUD->BindUIModel(mShopUIModel);
			ShopHUD->OpenUI();
		}
	}
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
void AShopGameMode::PushShopUIData()
{
	if (mShopUIModel == nullptr)
	{
		return;
	}

	// 슬롯 → 상품 매핑은 판매 목록과 함께 매번 다시 기록한다
	mSlotSources.Reset();

	FShopUI ShopUIData;
	ShopUIData.mGold = GetPartyGold();
	ShopUIData.mRest.mPrice = ShopRestPrice;
	ShopUIData.mRest.mIsUsed = mRestUsed;
	ShopUIData.mRest.mIsAffordable = !mRestUsed
		&& ShopRestPrice <= ShopUIData.mGold;

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

				// 구매 지급 시 슬롯 번호로 상품을 찾을 수 있게 기록
				mSlotSources.Add(Slot, { Kind, ItemId });

				FShopItemUI Item;
				Item.mSlotIndex = Slot;
				Item.mKind = Kind;
				Item.mIsSoldOut = mSoldSlots.Contains(Slot);
				// 정가는 에셋(mPrice)에서 읽는다. 로드 실패 시 0 유지.
				// 할인 규칙이 생기면 여기서 최종가를 계산해 담는다.
				Item.mPrice = 0;
				Item.mName = FText::FromName(ItemId.PrimaryAssetName);

				if (Kind == EShopItemKind::Skill)
				{
					if (const UStaticSkillData* Data =
						AssetManager->GetPrimaryAssetObject<UStaticSkillData>(ItemId))
					{
						// 이름 미설정 DA(placeholder)는 에셋 이름 폴백 유지
						if (Data->mName.IsEmpty() == false)
						{
							Item.mName = Data->mName;
						}
						Item.mIcon = Data->mIcon.LoadSynchronous();
						Item.mDescription = Data->mDescription;
						// 가격은 유닛 스킬 파생 타입(UStaticUnitSkillData)에 있다
						if (const UStaticUnitSkillData* UnitSkillData = Cast<UStaticUnitSkillData>(Data))
						{
							Item.mPrice = UnitSkillData->mPrice;
							Item.mRequiredJobType = UnitSkillData->mJobType;
							Item.mRarityColor = GetShopRarityColor(
								UnitSkillData->mRarityType);
						}
					}
				}
				// 아티펙트는 전용 자산에서 이름과 아이콘을 읽는다. 장비 폴백 없음.
				else if (Kind == EShopItemKind::Artifact)
				{
					if (const UStaticArtifactData* Data =
						AssetManager->GetPrimaryAssetObject<UStaticArtifactData>(ItemId))
					{
						// 이름 미설정 DA(placeholder)는 에셋 이름 폴백 유지
						if (Data->mName.IsEmpty() == false)
						{
							Item.mName = Data->mName;
						}
						Item.mIcon = Data->mIcon.LoadSynchronous();
						Item.mPrice = Data->mPrice;
						Item.mRarityColor = GetShopRarityColor(Data->mRarityType);
						// 아티팩트 자체에는 설명 필드가 없으므로 첫 패시브의 UI 설명을
						// 상점 상세 카드에 사용한다. 설명 없는 순수 스탯 아티팩트는
						// 위젯의 종류별 기본 문구로 폴백한다.
						for (const TSoftObjectPtr<UStaticPassiveData>& PassiveSoft
							: Data->mStaticPassiveData)
						{
							if (const UStaticPassiveData* Passive = PassiveSoft.LoadSynchronous();
								Passive != nullptr && !Passive->mDescription.IsEmpty())
							{
								Item.mDescription = Passive->mDescription;
								break;
							}
						}
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

	// 용병 후보는 기존 캐릭터 선택 DTO를 그대로 채워 전용 고용 화면에 내린다.
	// FShopRoom에는 이미 후보/레벨/추가 스킬이 저장돼 있었지만, 이전 상점
	// 어댑터가 이 목록을 누락해 UI의 Mercenary 종류가 항상 비어 있었다.
	const int32 PartyDifficulty = GetPartyModel() != nullptr
		? GetPartyModel()->GetDifficulty() : 1;
	for (int32 CandidateIndex = 0;
		CandidateIndex < ShopRoom->mSaleMercenaryDataCandidates.mCandidates.Num();
		++CandidateIndex)
	{
		const FMercenaryCandidate& Candidate =
			ShopRoom->mSaleMercenaryDataCandidates.mCandidates[CandidateIndex];
		const int32 Slot = SlotIndex++;
		if (!Candidate.mSaleMercenaryId.IsValid() || AssetManager == nullptr)
		{
			continue;
		}

		mSlotSources.Add(Slot,
			{ EShopItemKind::Mercenary, Candidate.mSaleMercenaryId, CandidateIndex });
		FShopMercenaryUI& Mercenary =
			ShopUIData.mMercenaries.AddDefaulted_GetRef();
		Mercenary.mSlotIndex = Slot;
		Mercenary.mLevel = FMath::Max(1, Candidate.mLevel);
		// 구형 저장 데이터는 가격이 0이었다. 새 생성 룸은 StageBuilder가 같은
		// 공식으로 값을 기록하고, 구형 런도 여기서 정상 가격으로 보정한다.
		Mercenary.mPrice = Candidate.mPrice > 0
			? Candidate.mPrice : 100 + Mercenary.mLevel * 50;
		Mercenary.mIsSoldOut = mSoldSlots.Contains(Slot);

		FFrontendCharacterOption& Character = Mercenary.mCharacter;
		Character.mIndex = CandidateIndex;
		Character.mPlayerUnitId = Candidate.mSaleMercenaryId;
		const UStaticPlayerUnitSpawnData* UnitData =
			AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(
				Candidate.mSaleMercenaryId);
		if (UnitData != nullptr)
		{
			Character.mJobType = UnitData->mJobType;
			Character.mDisplayName = ShopMercenaryName(UnitData->mJobType);
			Character.mRoleText = ShopMercenaryRole(UnitData->mJobType);
			Character.mRoleShort = FText::Format(
				NSLOCTEXT("ShopGameMode", "MercenaryLevel", "Lv.{0}"),
				FText::AsNumber(Mercenary.mLevel));
			Character.mDescription = UnitData->mDescription.IsEmpty()
				? Character.mRoleText : UnitData->mDescription;
			Character.mPortrait = UnitData->mPortrait;
			Character.mIcon = UnitData->mIcon;
			Character.mMaxHP = FMath::RoundToInt(UnitData->GetDefaultAttributeValue(
				GetWorld(), UPlayerUnitAttributeSet::StaticClass(),
				UPlayerUnitAttributeSet::GetMaxHPAttribute(), PartyDifficulty));
			Character.mMaxAP = FMath::RoundToInt(UnitData->GetDefaultAttributeValue(
				GetWorld(), UPlayerUnitAttributeSet::StaticClass(),
				UUnitAttributeSet::GetRechargeActionPointAttribute(), PartyDifficulty));
			Character.mSpeed = FMath::RoundToInt(UnitData->GetDefaultAttributeValue(
				GetWorld(), UPlayerUnitAttributeSet::StaticClass(),
				UUnitAttributeSet::GetRechargeSpeedPointAttribute(), PartyDifficulty));
			Character.mStatSummary = FText::Format(
				NSLOCTEXT("ShopGameMode", "MercenarySummary", "Lv.{0} · HP {1}"),
				FText::AsNumber(Mercenary.mLevel), FText::AsNumber(Character.mMaxHP));

			auto AppendSkill = [&Character](const UStaticSkillData* Skill)
			{
				if (Skill == nullptr)
				{
					return;
				}
				Character.mSkillNames.Add(Skill->mName);
				Character.mSkillIcons.Add(Skill->mIcon);
				FFrontendSkillOption& Detail =
					Character.mSkillDetails.AddDefaulted_GetRef();
				Detail.mName = Skill->mName;
				Detail.mDescription = Skill->mDescription;
				Detail.mIcon = Skill->mIcon;
				Detail.mCooldownTurns = FMath::Max(0, Skill->mCooldownDuration);
				Detail.mAimPattern = Skill->mAimPattern;
				Detail.mAimRange = Skill->mAimRange;
				Detail.mEffectPattern = Skill->mEffectPattern;
				Detail.mEffectArea = Skill->mEffectArea;
				Detail.mAimBlockerMask = Skill->mAimBlockerMask;
				Detail.mEffectBlockerMask = Skill->mEffectBlockerMask;
				Detail.mTargetPattern = Skill->mTargetPattern;
				if (const UStaticUnitSkillData* UnitSkill =
					Cast<UStaticUnitSkillData>(Skill))
				{
					Detail.mActionPointCost =
						FMath::Max(0, UnitSkill->mRequiredActionPoint);
				}
			};
			for (const TSoftObjectPtr<UStaticSkillData>& Skill : UnitData->mSkillDatas)
			{
				AppendSkill(Skill.LoadSynchronous());
			}
			for (const FPrimaryAssetId& SkillId : Candidate.mOwingSkillIds)
			{
				AppendSkill(AssetManager->GetPrimaryAssetObject<UStaticSkillData>(SkillId));
			}
			Character.mSelectable = !Mercenary.mIsSoldOut
				&& !UnitData->mModelClass.IsNull();
		}
		Mercenary.mIsAffordable = !Mercenary.mIsSoldOut
			&& Mercenary.mPrice <= ShopUIData.mGold;
		if (Mercenary.mIsSoldOut)
		{
			Character.mDisabledReason =
				NSLOCTEXT("ShopGameMode", "MercenarySold", "고용 완료");
		}
	}

	mShopUIModel->SetShop(ShopUIData);
}

/**
 * @brief 소지품과 파티 유닛을 표시값으로 채움
 * @details
 * 아티펙트는 파티 소유 — 배열 index가 그대로 버리기 요청 payload.
 * 스킬은 유닛+슬롯 소유 — 유닛 카드에 슬롯째(빈 칸 포함) 담아
 * (유닛 index, 슬롯 배열 위치)가 구매/교체 대상 지정 payload가 된다.
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

	// 파티 유닛 카드 (파티는 3슬롯 고정 + 빈 슬롯 허용, 빈 파티 슬롯은 생략)
	// 유닛 모델엔 표시용 이름이 없어 직업+레벨로 식별
	const TArray<TObjectPtr<UPlayerUnitModel>>& UnitModels = PartyModel->GetPlayerUnitModels();
	for (int32 UnitIndex = 0; UnitIndex < UnitModels.Num(); ++UnitIndex)
	{
		const UPlayerUnitModel* UnitModel = UnitModels[UnitIndex];
		FShopMercenaryPartySlotUI& HireSlot =
			ShopUIData.mMercenaryPartySlots.AddDefaulted_GetRef();
		HireSlot.mUnitIndex = UnitIndex;
		if (UnitModel == nullptr)
		{
			continue;
		}
		HireSlot.mIsOccupied = true;
		HireSlot.mJobType = UnitModel->GetUnitJobType();
		HireSlot.mLevel = UnitModel->GetPlayerLevel();

		FShopOwnedUnitUI OwnedUnit;
		OwnedUnit.mUnitIndex = UnitIndex;
		OwnedUnit.mJobType = UnitModel->GetUnitJobType();
		OwnedUnit.mLevel = UnitModel->GetPlayerLevel();

		if (const UAttributeSetComponentModel* Attributes =
			UnitModel->GetAttributeComponentModel())
		{
			FShopRestUnitUI RestUnit;
			RestUnit.mUnitIndex = UnitIndex;
			RestUnit.mJobType = OwnedUnit.mJobType;
			RestUnit.mHP = Attributes->GetAttributeCurrentValue(
				UPlayerUnitAttributeSet::GetHPAttribute());
			RestUnit.mMaxHP = Attributes->GetAttributeCurrentValue(
				UPlayerUnitAttributeSet::GetMaxHPAttribute());
			RestUnit.mAP = Attributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetActionPointAttribute());
			RestUnit.mMaxAP = Attributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetRechargeActionPointAttribute());
			ShopUIData.mRest.mUnits.Add(RestUnit);
		}

		// 빈 슬롯도 담아 배열 위치 = 스킬 슬롯 index 유지 (구매/교체 대상 지정용)
		if (const USkillComponentModel* SkillModel = UnitModel->GetSkillComponentModel())
		{
			const TArray<FSkillEntry>& Skills = SkillModel->GetSkills();
			for (const FSkillEntry& Entry : Skills)
			{
				FShopOwnedSkillSlotUI SkillSlot;
				if (const UStaticSkillData* Data = Entry.mData)
				{
					SkillSlot.mIsEmpty = false;
					SkillSlot.mName = Data->mName;
					SkillSlot.mDescription = Data->mDescription;
					SkillSlot.mIcon = Data->mIcon.LoadSynchronous();
				}
				OwnedUnit.mSkillSlots.Add(SkillSlot);
			}
		}

		ShopUIData.mOwnedUnits.Add(OwnedUnit);
	}
}

/**
 * @brief 한 칸을 샀다 (파티 공용 상품 전용)
 * @details
 * 지급이 먼저다 -- 실패하면 과금하지 않는다.
 * 스킬은 대상 유닛이 필요해 별도 의도(OnBuySkillRequested)로 온다.
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

	// 슬롯이 가리키는 실제 상품 찾기
	const FShopSlotSource* Source = mSlotSources.Find(SlotIndex);
	if (Source == nullptr || Source->mKind != EShopItemKind::Artifact)
	{
		return;
	}

	// 지급 (아티펙트는 파티 공용이라 대상 선택이 없다)
	UPartyModel* PartyModel = GetPartyModel();
	UPartyArtifactComponentModel* ArtifactModel = PartyModel != nullptr
		? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	if (ArtifactModel == nullptr || ArtifactModel->AddArtifact(Source->mItemId) == false)
	{
		return;
	}

	// 과금 + 품절 처리
	SpendPartyGold(Item->mPrice);
	mSoldSlots.Add(SlotIndex);
	PushShopUIData();
}

/**
 * @brief 스킬 한 칸을 샀다 (지급 대상 유닛+스킬 슬롯 지정)
 * @details
 * 선지급 후 직업이 불일치하면 과금하지 않음.
 * 실제로는 UI 단계에서 필터링 되겠지만 로직에서도 검사 추가해서 정합성 유지.
 * @param SlotIndex 화면이 돌려보낸 판매 자리 번호
 * @param UnitIndex 지급 대상 파티 유닛 슬롯 index
 * @param SkillSlotIndex 지급 대상 유닛 내 스킬 슬롯 index
 */
void AShopGameMode::HandleBuySkillRequested(int32 SlotIndex, int32 UnitIndex, int32 SkillSlotIndex)
{
	if (mShopUIModel == nullptr || mSoldSlots.Contains(SlotIndex) == true)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 뷰모델 없음 또는 품절 (Slot=%d)"), SlotIndex);
		return;
	}

	const TArray<FShopItemUI>& Items = mShopUIModel->GetShop().mItems;
	const FShopItemUI* Item = Items.FindByPredicate(
		[SlotIndex](const FShopItemUI& Candidate)
		{ return Candidate.mSlotIndex == SlotIndex; });
	if (Item == nullptr || Item->mIsAffordable == false)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 슬롯 없음 또는 골드 부족 (Slot=%d)"), SlotIndex);
		return;
	}

	// 슬롯이 가리키는 실제 상품 찾기
	const FShopSlotSource* Source = mSlotSources.Find(SlotIndex);
	if (Source == nullptr || Source->mKind != EShopItemKind::Skill)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 스킬 슬롯이 아님 (Slot=%d)"), SlotIndex);
		return;
	}

	// 파티원 인덱스와 유효성을 미리 검사
	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 파티 모델 없음"));
		return;
	}
	const TArray<TObjectPtr<UPlayerUnitModel>>& UnitModels = PartyModel->GetPlayerUnitModels();
	if (UnitModels.IsValidIndex(UnitIndex) == false || UnitModels[UnitIndex] == nullptr)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 유닛 없음 (Unit=%d)"), UnitIndex);
		return;
	}
	USkillComponentModel* SkillModel = UnitModels[UnitIndex]->GetSkillComponentModel();
	if (SkillModel == nullptr)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 스킬 컴포넌트 없음 (Unit=%d)"), UnitIndex);
		return;
	}

	// 지정 슬롯 검사. 빈 슬롯이면 장착, 찬 슬롯이면 교체
	const TArray<FSkillEntry>& Skills = SkillModel->GetSkills();
	const bool bReplaceableSkillSlot = SkillSlotIndex >= ReplaceableSkillStartIndex
		&& SkillSlotIndex < ReplaceableSkillStartIndex + ReplaceableSkillSlotCount;
	if (!bReplaceableSkillSlot || Skills.IsValidIndex(SkillSlotIndex) == false)
	{
		UE_LOG(LogRD, Log,
			TEXT("스킬 구매 거절: 교체 가능한 스킬 슬롯 범위 밖 (Unit=%d, SkillSlot=%d)"),
			UnitIndex, SkillSlotIndex);
		return;
	}
	const bool bIsReplace = Skills[SkillSlotIndex].mData != nullptr;

	// 스킬 데이터 로드 (판매 목록 표시 때 이미 로드된 에셋)
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	UStaticSkillData* SkillData = AssetManager != nullptr
		? AssetManager->GetPrimaryAssetObject<UStaticSkillData>(Source->mItemId) : nullptr;
	if (SkillData == nullptr)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 스킬 데이터 로드 실패 (Slot=%d)"), SlotIndex);
		return;
	}

	// 지급. SetSkill의 직업 검사(IsAcquirableSkill)가 거부하면 과금하지 않음
	if (SkillModel->SetSkill(SkillSlotIndex, SkillData) == false)
	{
		UE_LOG(LogRD, Log, TEXT("스킬 구매 거절: 습득 불가 스킬 (Slot=%d, Unit=%d, 직업 불일치 등)"), SlotIndex, UnitIndex);
		return;
	}

	UE_LOG(LogRD, Log, TEXT("스킬 구매 성공(%s): Slot=%d -> Unit=%d 스킬슬롯=%d"),
		bIsReplace ? TEXT("교체") : TEXT("장착"), SlotIndex, UnitIndex, SkillSlotIndex);

	// 과금 + 품절 처리
	SpendPartyGold(Item->mPrice);
	mSoldSlots.Add(SlotIndex);
	PushShopUIData();
}

void AShopGameMode::HandleHireMercenaryRequested(
	const int32 SlotIndex, const int32 PartySlotIndex)
{
	if (mShopUIModel == nullptr || mSoldSlots.Contains(SlotIndex))
	{
		return;
	}

	const FShopMercenaryUI* DisplayCandidate =
		mShopUIModel->GetShop().mMercenaries.FindByPredicate(
			[SlotIndex](const FShopMercenaryUI& Candidate)
			{ return Candidate.mSlotIndex == SlotIndex; });
	const FShopSlotSource* Source = mSlotSources.Find(SlotIndex);
	if (DisplayCandidate == nullptr || !DisplayCandidate->mIsAffordable
		|| DisplayCandidate->mIsSoldOut || Source == nullptr
		|| Source->mKind != EShopItemKind::Mercenary)
	{
		return;
	}

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
	if (ShopRoom == nullptr
		|| !ShopRoom->mSaleMercenaryDataCandidates.mCandidates.IsValidIndex(
			Source->mCandidateIndex))
	{
		return;
	}
	const FMercenaryCandidate& Candidate =
		ShopRoom->mSaleMercenaryDataCandidates.mCandidates[Source->mCandidateIndex];

	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr
		|| !PartyModel->GetPlayerUnitModels().IsValidIndex(PartySlotIndex))
	{
		return;
	}
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	UStaticPlayerUnitSpawnData* UnitData = AssetManager != nullptr
		? AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(
			Candidate.mSaleMercenaryId) : nullptr;
	UClass* ModelClass = UnitData != nullptr
		? UnitData->mModelClass.LoadSynchronous() : nullptr;
	if (UnitData == nullptr || ModelClass == nullptr)
	{
		return;
	}

	UPlayerUnitModel* NewUnit =
		GetWorldModelFactory(this)->NewModelDeferred<UPlayerUnitModel>(ModelClass);
	if (NewUnit == nullptr)
	{
		return;
	}
	NewUnit->SetStaticSpawnData(UnitData);
	NewUnit->SetPlayerLevel(FMath::Max(1, Candidate.mLevel));
	NewUnit->FinishCreating();
	// #432 규칙(신규 합류 일괄 장착): 파티 공용 아티팩트를 새 용병에게도 장착한다.
	// 빠뜨리면 상점 고용 용병만 아티팩트 효과 없이 전투에 들어간다.
	if (UPartyArtifactComponentModel* PartyArtifacts =
		PartyModel->GetPartyArtifactComponentModel())
	{
		PartyArtifacts->EquipArtifactsTo(NewUnit);
	}

	UPlayerUnitModel* PreviousUnit =
		PartyModel->GetPlayerUnitModels()[PartySlotIndex].Get();
	PartyModel->SetPlayerUnitModel(PartySlotIndex, NewUnit);

	// 후보에 붙은 추가 스킬은 플레이어 고정 스킬 뒤의 교체 가능 슬롯부터
	// 채운다. 실패한 스킬 하나 때문에 이미 생성된 용병 전체를 무효화하지 않는다.
	if (USkillComponentModel* SkillModel = NewUnit->GetSkillComponentModel())
	{
		for (int32 Index = 0;
			Index < Candidate.mOwingSkillIds.Num()
				&& Index < ReplaceableSkillSlotCount; ++Index)
		{
			if (UStaticSkillData* Skill =
				AssetManager->GetPrimaryAssetObject<UStaticSkillData>(
					Candidate.mOwingSkillIds[Index]))
			{
				SkillModel->SetSkill(ReplaceableSkillStartIndex + Index, Skill);
			}
		}
	}
	if (PreviousUnit != nullptr)
	{
		GetWorldModelFactory(this)->DestroyModel(PreviousUnit);
	}

	SpendPartyGold(DisplayCandidate->mPrice);
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
 * @param UnitIndex 파티 유닛 슬롯 index (FShopOwnedUnitUI가 돌려준 값)
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

/**
 * @brief 100G를 소비해 현재 파티 전원의 HP와 AP를 완전히 회복한다.
 * @details UI 스냅샷 값은 신뢰하지 않고 현재 골드와 파티 모델을 다시 검증한다.
 * HP 변경은 UPlayerUnitPersistData가 구독 중인 기존 Attribute 델리게이트를 통해 즉시 저장값에 반영된다.
 */
void AShopGameMode::HandleRestRequested()
{
	if (mRestUsed || GetPartyGold() < ShopRestPrice)
	{
		return;
	}

	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		return;
	}

	TArray<UAttributeSetComponentModel*> UnitAttributes;
	for (const TObjectPtr<UPlayerUnitModel>& UnitModelPtr
		: PartyModel->GetPlayerUnitModels())
	{
		UPlayerUnitModel* UnitModel = UnitModelPtr.Get();
		if (UnitModel == nullptr)
		{
			continue;
		}

		UAttributeSetComponentModel* Attributes = UnitModel->GetAttributeComponentModel();
		if (Attributes == nullptr)
		{
			// 일부만 회복하고 과금하는 상태를 만들지 않는다.
			return;
		}
		UnitAttributes.Add(Attributes);
	}

	if (UnitAttributes.IsEmpty())
	{
		return;
	}

	for (UAttributeSetComponentModel* Attributes : UnitAttributes)
	{
		const float MaxHP = Attributes->GetAttributeCurrentValue(
			UPlayerUnitAttributeSet::GetMaxHPAttribute());
		const float MaxAP = Attributes->GetAttributeCurrentValue(
			UUnitAttributeSet::GetRechargeActionPointAttribute());
		Attributes->ApplyModToAttribute(
			UPlayerUnitAttributeSet::GetHPAttribute(), ETacticalModOp::Override, MaxHP);
		Attributes->ApplyModToAttribute(
			UUnitAttributeSet::GetActionPointAttribute(), ETacticalModOp::Override, MaxAP);
	}

	SpendPartyGold(ShopRestPrice);
	mRestUsed = true;
	PushShopUIData();
}

/** @brief 상점 HUD를 닫은 뒤 다음 방을 고르는 공용 월드맵을 연다. */
void AShopGameMode::HandleLeaveRequested()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("상점 나가기 실패: WorldWidgetSubsystem 없음"));
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
		UE_LOG(LogRD, Warning, TEXT("상점 나가기 실패: WorldMap 위젯 미설정"));
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

/** @brief 파티 골드 소비. 0 이하 가격은 무시한다. */
void AShopGameMode::SpendPartyGold(int32 Price)
{
	if (Price <= 0)
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
				-StaticCast<float>(Price));
		}
	}
}

#if !UE_BUILD_SHIPPING
/** @brief 개발 치트: 파티 골드를 더하고 화면을 갱신한다. */
void AShopGameMode::AddPartyGoldDev(int32 Amount)
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

	PushShopUIData();
}
#endif

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

/**
 * @brief 판매 슬롯 상세 요청 → 종류에 맞는 상세 DTO를 조립해 모델로 되민다.
 *
 * @details
 * 위젯은 GetAuthGameMode로 이쪽을 직접 부르지 않는다 -- 의도(Request)만 보내고
 * 응답은 Set*Detail 푸시로 받는다 (SetShop과 같은 계약). 조립 실패(로드 실패 등)
 * 시에도 빈 DTO를 밀어 위젯이 플레인 폴백을 열 수 있게 한다.
 */
void AShopGameMode::HandleItemDetailRequested(int32 SlotIndex)
{
	if (mShopUIModel == nullptr)
	{
		return;
	}

	const FShopSlotSource* Source = mSlotSources.Find(SlotIndex);
	if (Source != nullptr && Source->mKind == EShopItemKind::Artifact)
	{
		FCombatArtifactUI Detail;
		BuildShopItemArtifactDetailUI(SlotIndex, OUT Detail);
		mShopUIModel->SetArtifactDetail(Detail);
		return;
	}

	FSkillDetailUI Detail;
	BuildShopItemSkillDetailUI(SlotIndex, OUT Detail);
	mShopUIModel->SetSkillDetail(Detail);
}

/** @brief 소지 용병 스킬 상세 요청 → 파티 생산 API 결과를 모델로 되민다. */
void AShopGameMode::HandleOwnedSkillDetailRequested(int32 UnitIndex, int32 SkillSlotIndex)
{
	if (mShopUIModel == nullptr)
	{
		return;
	}

	FSkillDetailUI Detail;
	BuildPartyUnitSkillDetailUI(UnitIndex, SkillSlotIndex, OUT Detail);
	mShopUIModel->SetSkillDetail(Detail);
}

bool AShopGameMode::BuildShopItemSkillDetailUI(int32 SlotIndex,
	FSkillDetailUI& OutDetail) const
{
	OutDetail = FSkillDetailUI();

	const FShopSlotSource* Source = mSlotSources.Find(SlotIndex);
	if (Source == nullptr || Source->mKind != EShopItemKind::Skill)
	{
		return false;
	}
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	const UStaticSkillData* SkillData = AssetManager != nullptr
		? AssetManager->GetPrimaryAssetObject<UStaticSkillData>(Source->mItemId)
		: nullptr;
	if (SkillData == nullptr)
	{
		return false;
	}

	SkillDetailUIBuilder::FillFromSkillData(SkillData, OutDetail);
	return true;
}

bool AShopGameMode::BuildShopItemArtifactDetailUI(int32 SlotIndex,
	FCombatArtifactUI& OutDetail) const
{
	OutDetail = FCombatArtifactUI();

	const FShopSlotSource* Source = mSlotSources.Find(SlotIndex);
	if (Source == nullptr || Source->mKind != EShopItemKind::Artifact)
	{
		return false;
	}
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	const UStaticArtifactData* ArtifactData = AssetManager != nullptr
		? AssetManager->GetPrimaryAssetObject<UStaticArtifactData>(Source->mItemId)
		: nullptr;
	if (ArtifactData == nullptr)
	{
		return false;
	}

	SkillDetailUIBuilder::FillFromArtifactData(ArtifactData, OutDetail);
	return true;
}
