#include "UI/Shop/ShopUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/ViewportZOrderType.h"

#define LOCTEXT_NAMESPACE "ShopUIWidgetBase"

namespace
{
	/** 상점에서 교체 가능한 네 칸은 두 기본 행동 뒤의 실제 스킬 슬롯 2..5다. */
	constexpr int32 ReplaceableSkillStartIndex = 2;
	constexpr int32 ReplaceableSkillSlotCount = 4;

	/** @brief 상점 항목 종류별 기본 아이콘 텍스처 경로(SVN 임포트). */
	// 종류 구분은 거래 파이프라인(#490)의 것을 따르고, 경로는 실재하는
	// SVN 에셋으로 건다 -- 옛 InSideAsset 경로 3종은 프로젝트에서 지워져
	// 아이콘이 빈칸으로 나왔다(0807 감사).
	const TCHAR* ShopKindIconPath(EShopItemKind Kind)
	{
		switch (Kind)
		{
		case EShopItemKind::Skill:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Whirlwind.T_SkillIcon_Whirlwind");
		case EShopItemKind::Artifact:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice");
		case EShopItemKind::Heal:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Concept02/T_skill_meditation_heal_icon.T_skill_meditation_heal_icon");
		case EShopItemKind::Mercenary:   // 용병 전용 그림이 아직 없다 -- 골드로
		default:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Concept02/T_gold_icon.T_gold_icon");
		}
	}

	UTexture2D* ResolveLegacyShopIcon(const FShopItemUI& Item)
	{
		if (Item.mIcon != nullptr)
		{
			return Item.mIcon;
		}
		return LoadObject<UTexture2D>(nullptr, ShopKindIconPath(Item.mKind));
	}

	/** @brief 스킬 탭 대상 선택기에 사용할 기존 용병 직업 아이콘. */
	const TCHAR* ShopUnitIconPath(EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight");
		case EUnitJobType::Ranger:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Ranger.T_MB_HireIcon_Ranger");
		case EUnitJobType::Mage:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage");
		case EUnitJobType::Barbarian:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Barbarian.T_MB_HireIcon_Barbarian");
		case EUnitJobType::Rogue:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue");
		case EUnitJobType::Druid:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Druid.T_MB_HireIcon_Druid");
		default:
			return nullptr;
		}
	}

	UTexture2D* ResolveLegacyShopUnitIcon(EUnitJobType JobType)
	{
		const TCHAR* Path = ShopUnitIconPath(JobType);
		return Path != nullptr ? LoadObject<UTexture2D>(nullptr, Path) : nullptr;
	}

	UTexture2D* ResolveLegacyShopSelectionPlate(bool bSelected)
	{
		const TCHAR* Path = bSelected
			? TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowSelected.T_MB_HireRowSelected")
			: TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireRowNormal.T_MB_HireRowNormal");
		return LoadObject<UTexture2D>(nullptr, Path);
	}

	void SetShopWidgetShown(UWidget* Widget, bool bShown)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(bShown
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
	}

	void SetShopButtonShown(UButton* Button, bool bShown)
	{
		if (Button != nullptr)
		{
			Button->SetVisibility(bShown
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		}
	}

	void SetShopImage(UImage* Image, UTexture2D* Texture)
	{
		if (Image == nullptr)
		{
			return;
		}

		Image->SetBrushFromTexture(Texture, false);
		Image->SetVisibility(Texture != nullptr
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Hidden);
	}

	// 카드 텍스트 폰트 크기 (UMG 기본 24pt는 카드에 과대)
	constexpr int32 CardHeaderFontSize = 18;
	constexpr int32 CardTextFontSize = 16;

	/** @brief 동적 생성 TextBlock의 폰트 크기만 바꾼다(서체는 기본 유지). */
	void SetCardFontSize(UTextBlock* Text, int32 Size)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
	}
}

UShopUIWidgetBase::UShopUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/** @brief 나가기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void UShopUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	CacheFinalShopWidgets();
	BindFinalShopInputs();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(LOCTEXT("Back", "뒤로"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(LOCTEXT("Shop", "상점"));
	}
	if (mArtifactTabText != nullptr)
	{
		mArtifactTabText->SetText(LOCTEXT("ArtifactTab", "아티팩트"));
	}
	if (mSkillTabText != nullptr)
	{
		mSkillTabText->SetText(LOCTEXT("SkillTab", "스킬"));
	}
	if (mRestTabText != nullptr)
	{
		mRestTabText->SetText(LOCTEXT("RestTab", "휴식"));
	}
	if (mBuyButtonText != nullptr)
	{
		mBuyButtonText->SetText(LOCTEXT("Buy", "구매"));
	}
	if (mRestButtonText != nullptr)
	{
		mRestButtonText->SetText(LOCTEXT("Rest", "휴식하기"));
	}
	if (mInventoryButtonText != nullptr)
	{
		mInventoryButtonText->SetText(LOCTEXT("Inventory", "인벤토리"));
	}

	RefreshView();
}

UTexture2D* UShopUIWidgetBase::ResolveItemIcon(const FShopItemUI& Item) const
{
	return ResolveLegacyShopIcon(Item);
}

UTexture2D* UShopUIWidgetBase::ResolveUnitIcon(const EUnitJobType JobType) const
{
	return ResolveLegacyShopUnitIcon(JobType);
}

UTexture2D* UShopUIWidgetBase::ResolveUnitSelectionPlate(const bool bSelected) const
{
	return ResolveLegacyShopSelectionPlate(bSelected);
}

UTexture2D* UShopUIWidgetBase::ResolveSkillSelectionPlate(const bool bSelected) const
{
	return ResolveLegacyShopSelectionPlate(bSelected);
}

UTexture2D* UShopUIWidgetBase::ResolveOwnedArtifactPlate() const
{
	return ResolveLegacyShopSelectionPlate(false);
}

UTexture2D* UShopUIWidgetBase::ResolveTabPlate(const bool) const
{
	return nullptr;
}

bool UShopUIWidgetBase::HasFinalShopLayout() const
{
	return mArtifactShopPanel != nullptr
		|| mSkillShopPanel != nullptr
		|| mSelectedItemIcon != nullptr
		|| (WidgetTree != nullptr
			&& WidgetTree->FindWidget(TEXT("ShopRailButton_0")) != nullptr);
}

/** @brief 배열형 슬롯은 BindWidget 멤버를 12개 만들지 않고 합의된 이름으로 한 번 찾아 둔다. */
void UShopUIWidgetBase::CacheFinalShopWidgets()
{
	mRailButtons.Reset();
	mRailPlates.Reset();
	mRailIcons.Reset();
	mRailPriceTexts.Reset();
	mUnitSelectButtons.Reset();
	mUnitSelectPlates.Reset();
	mUnitSelectIcons.Reset();
	mSkillSlotButtons.Reset();
	mSkillSlotPlates.Reset();
	mSkillSlotIcons.Reset();
	mRestUnitRowHolders.Reset();
	mRestUnitPlates.Reset();
	mRestUnitIcons.Reset();
	mRestUnitHPBeforeTexts.Reset();
	mRestUnitHPAfterTexts.Reset();
	mRestUnitAPBeforeTexts.Reset();
	mRestUnitAPAfterTexts.Reset();
	mRestUnitHPBeforeFills.Reset();
	mRestUnitHPAfterFills.Reset();
	mRestUnitAPBeforeFills.Reset();
	mRestUnitAPAfterFills.Reset();
	mRailShopItemIndices.Init(INDEX_NONE, 5);

	if (WidgetTree == nullptr)
	{
		return;
	}

	mPreviousHolder = WidgetTree->FindWidget(TEXT("PreviousHolder"));
	mNextHolder = WidgetTree->FindWidget(TEXT("NextHolder"));
	mBuyHolder = WidgetTree->FindWidget(TEXT("BuyHolder"));
	mArtifactTabPlate = Cast<UImage>(WidgetTree->FindWidget(TEXT("ArtifactTabPlate")));
	mSkillTabPlate = Cast<UImage>(WidgetTree->FindWidget(TEXT("SkillTabPlate")));
	mRestTabPlate = Cast<UImage>(WidgetTree->FindWidget(TEXT("RestTabPlate")));
	mSkillSelectionPointer = Cast<UImage>(
		WidgetTree->FindWidget(TEXT("ShopSelectionPointer")));

	for (int32 Index = 0; Index < 5; ++Index)
	{
		mRailButtons.Add(Cast<UButton>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("ShopRailButton_%d"), Index)))));
		mRailPlates.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("ShopRailPlate_%d"), Index)))));
		mRailIcons.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("ShopRailIcon_%d"), Index)))));
		mRailPriceTexts.Add(Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("ShopRailPriceText_%d"), Index)))));
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		mUnitSelectButtons.Add(Cast<UButton>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("mUnitSelectButton_%d"), Index)))));
		mUnitSelectPlates.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("UnitSelectPlate_%d"), Index)))));
		mUnitSelectIcons.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("mUnitSelectIcon_%d"), Index)))));

		mRestUnitRowHolders.Add(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitRowHolder_%d"), Index))));
		mRestUnitPlates.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitPlate_%d"), Index)))));
		mRestUnitIcons.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitIcon_%d"), Index)))));
		mRestUnitHPBeforeTexts.Add(Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitHPBeforeText_%d"), Index)))));
		mRestUnitHPAfterTexts.Add(Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitHPAfterText_%d"), Index)))));
		mRestUnitAPBeforeTexts.Add(Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitAPBeforeText_%d"), Index)))));
		mRestUnitAPAfterTexts.Add(Cast<UTextBlock>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitAPAfterText_%d"), Index)))));
		mRestUnitHPBeforeFills.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitHPBeforeFill_%d"), Index)))));
		mRestUnitHPAfterFills.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitHPAfterFill_%d"), Index)))));
		mRestUnitAPBeforeFills.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitAPBeforeFill_%d"), Index)))));
		mRestUnitAPAfterFills.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("RestUnitAPAfterFill_%d"), Index)))));
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		mSkillSlotButtons.Add(Cast<UButton>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("mSkillSlotButton_%d"), Index)))));
		mSkillSlotPlates.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("SkillSlotPlate_%d"), Index)))));
		mSkillSlotIcons.Add(Cast<UImage>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("mSkillSlotIcon_%d"), Index)))));
	}
}

void UShopUIWidgetBase::BindFinalShopInputs()
{
	if (mArtifactTabButton != nullptr)
	{
		mArtifactTabButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleArtifactTabClicked);
	}
	if (mSkillTabButton != nullptr)
	{
		mSkillTabButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleSkillTabClicked);
	}
	if (mRestTabButton != nullptr)
	{
		mRestTabButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleRestTabClicked);
	}
	if (mPreviousButton != nullptr)
	{
		mPreviousButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandlePreviousClicked);
	}
	if (mNextButton != nullptr)
	{
		mNextButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleNextClicked);
	}
	if (mBuyButton != nullptr)
	{
		mBuyButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleBuyClicked);
	}
	if (mRestButton != nullptr)
	{
		mRestButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleRestClicked);
	}
	if (mInventoryButton != nullptr)
	{
		mInventoryButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleInventoryClicked);
	}
	if (mArtifactInventoryCloseButton != nullptr)
	{
		mArtifactInventoryCloseButton->OnClicked.AddUniqueDynamic(
			this, &UShopUIWidgetBase::HandleArtifactInventoryCloseClicked);
	}

	if (mRailButtons.IsValidIndex(0) && mRailButtons[0] != nullptr)
	{
		mRailButtons[0]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleRailClicked0);
	}
	if (mRailButtons.IsValidIndex(1) && mRailButtons[1] != nullptr)
	{
		mRailButtons[1]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleRailClicked1);
	}
	if (mRailButtons.IsValidIndex(2) && mRailButtons[2] != nullptr)
	{
		mRailButtons[2]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleRailClicked2);
	}
	if (mRailButtons.IsValidIndex(3) && mRailButtons[3] != nullptr)
	{
		mRailButtons[3]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleRailClicked3);
	}
	if (mRailButtons.IsValidIndex(4) && mRailButtons[4] != nullptr)
	{
		mRailButtons[4]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleRailClicked4);
	}

	if (mUnitSelectButtons.IsValidIndex(0) && mUnitSelectButtons[0] != nullptr)
	{
		mUnitSelectButtons[0]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleUnitClicked0);
	}
	if (mUnitSelectButtons.IsValidIndex(1) && mUnitSelectButtons[1] != nullptr)
	{
		mUnitSelectButtons[1]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleUnitClicked1);
	}
	if (mUnitSelectButtons.IsValidIndex(2) && mUnitSelectButtons[2] != nullptr)
	{
		mUnitSelectButtons[2]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleUnitClicked2);
	}

	if (mSkillSlotButtons.IsValidIndex(0) && mSkillSlotButtons[0] != nullptr)
	{
		mSkillSlotButtons[0]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleSkillSlotClicked0);
	}
	if (mSkillSlotButtons.IsValidIndex(1) && mSkillSlotButtons[1] != nullptr)
	{
		mSkillSlotButtons[1]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleSkillSlotClicked1);
	}
	if (mSkillSlotButtons.IsValidIndex(2) && mSkillSlotButtons[2] != nullptr)
	{
		mSkillSlotButtons[2]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleSkillSlotClicked2);
	}
	if (mSkillSlotButtons.IsValidIndex(3) && mSkillSlotButtons[3] != nullptr)
	{
		mSkillSlotButtons[3]->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleSkillSlotClicked3);
	}
}

void UShopUIWidgetBase::HandleArtifactTabClicked()
{
	SetActiveItemKind(EShopItemKind::Artifact);
}

void UShopUIWidgetBase::HandleSkillTabClicked()
{
	SetActiveItemKind(EShopItemKind::Skill);
}

void UShopUIWidgetBase::HandleRestTabClicked()
{
	SetActiveItemKind(EShopItemKind::Heal);
}

void UShopUIWidgetBase::HandleRestClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FShopRestUI& Rest = mUIModel->GetShop().mRest;
	if (!Rest.mIsUsed && Rest.mIsAffordable && !Rest.mUnits.IsEmpty())
	{
		mUIModel->RequestRest();
	}
}

void UShopUIWidgetBase::HandleInventoryClicked()
{
	SetArtifactInventoryOpen(true);
}

void UShopUIWidgetBase::HandleArtifactInventoryCloseClicked()
{
	SetArtifactInventoryOpen(false);
}

void UShopUIWidgetBase::HandlePreviousClicked()
{
	if (mFilteredShopItemIndices.Num() > 1)
	{
		mSelectedFilteredIndex = (mSelectedFilteredIndex - 1
			+ mFilteredShopItemIndices.Num()) % mFilteredShopItemIndices.Num();
		RefreshView();
	}
}

void UShopUIWidgetBase::HandleNextClicked()
{
	if (mFilteredShopItemIndices.Num() > 1)
	{
		mSelectedFilteredIndex = (mSelectedFilteredIndex + 1)
			% mFilteredShopItemIndices.Num();
		RefreshView();
	}
}

void UShopUIWidgetBase::HandleBuyClicked()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FShopUI& Shop = mUIModel->GetShop();
	const FShopItemUI* Item = GetSelectedItem(Shop);
	if (Item == nullptr || Item->mIsSoldOut || !Item->mIsAffordable)
	{
		return;
	}

	if (Item->mKind == EShopItemKind::Artifact)
	{
		mUIModel->RequestBuy(Item->mSlotIndex);
		return;
	}

	if (Item->mKind == EShopItemKind::Skill
		&& Shop.mOwnedUnits.IsValidIndex(mSelectedUnitViewIndex))
	{
		const FShopOwnedUnitUI& Unit = Shop.mOwnedUnits[mSelectedUnitViewIndex];
		const int32 ModelSkillSlotIndex = ReplaceableSkillStartIndex
			+ mSelectedSkillSlotIndex;
		if (Unit.mSkillSlots.IsValidIndex(ModelSkillSlotIndex))
		{
			mUIModel->RequestBuySkill(
				Item->mSlotIndex, Unit.mUnitIndex, ModelSkillSlotIndex);
		}
	}
}

void UShopUIWidgetBase::HandleRailClicked0() { SelectRailSlot(0); }
void UShopUIWidgetBase::HandleRailClicked1() { SelectRailSlot(1); }
void UShopUIWidgetBase::HandleRailClicked2() { SelectRailSlot(2); }
void UShopUIWidgetBase::HandleRailClicked3() { SelectRailSlot(3); }
void UShopUIWidgetBase::HandleRailClicked4() { SelectRailSlot(4); }
void UShopUIWidgetBase::HandleUnitClicked0() { SelectUnitSlot(0); }
void UShopUIWidgetBase::HandleUnitClicked1() { SelectUnitSlot(1); }
void UShopUIWidgetBase::HandleUnitClicked2() { SelectUnitSlot(2); }
void UShopUIWidgetBase::HandleSkillSlotClicked0() { SelectSkillSlot(0); }
void UShopUIWidgetBase::HandleSkillSlotClicked1() { SelectSkillSlot(1); }
void UShopUIWidgetBase::HandleSkillSlotClicked2() { SelectSkillSlot(2); }
void UShopUIWidgetBase::HandleSkillSlotClicked3() { SelectSkillSlot(3); }

void UShopUIWidgetBase::SetActiveItemKind(EShopItemKind Kind)
{
	if (Kind != EShopItemKind::Artifact
		&& Kind != EShopItemKind::Skill
		&& Kind != EShopItemKind::Heal)
	{
		return;
	}

	if (mActiveItemKind != Kind)
	{
		mActiveItemKind = Kind;
		mSelectedFilteredIndex = 0;
	}
	if (Kind != EShopItemKind::Artifact)
	{
		mIsArtifactInventoryOpen = false;
	}
	RefreshView();
}

void UShopUIWidgetBase::SetArtifactInventoryOpen(bool bOpen)
{
	const bool bCanOpen = mActiveItemKind == EShopItemKind::Artifact
		&& mArtifactInventoryPanel != nullptr;
	mIsArtifactInventoryOpen = bOpen && bCanOpen;
	RefreshView();
}

void UShopUIWidgetBase::SelectRailSlot(int32 RailSlotIndex)
{
	if (!mRailShopItemIndices.IsValidIndex(RailSlotIndex))
	{
		return;
	}

	const int32 ShopItemIndex = mRailShopItemIndices[RailSlotIndex];
	const int32 FilteredIndex = mFilteredShopItemIndices.Find(ShopItemIndex);
	if (FilteredIndex != INDEX_NONE)
	{
		mSelectedFilteredIndex = FilteredIndex;
		RefreshView();
	}
}

void UShopUIWidgetBase::SelectUnitSlot(int32 UnitViewIndex)
{
	if (mUIModel == nullptr
		|| !mUIModel->GetShop().mOwnedUnits.IsValidIndex(UnitViewIndex))
	{
		return;
	}

	mSelectedUnitViewIndex = UnitViewIndex;
	mSelectedSkillSlotIndex = 0;
	mSelectedFilteredIndex = 0;
	RefreshView();
}

void UShopUIWidgetBase::SelectSkillSlot(int32 SkillSlotIndex)
{
	if (mUIModel == nullptr
		|| !mUIModel->GetShop().mOwnedUnits.IsValidIndex(mSelectedUnitViewIndex)
		|| !mUIModel->GetShop().mOwnedUnits[mSelectedUnitViewIndex]
			.mSkillSlots.IsValidIndex(ReplaceableSkillStartIndex + SkillSlotIndex))
	{
		return;
	}

	mSelectedSkillSlotIndex = SkillSlotIndex;
	RefreshView();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 상점 스냅샷도 즉시 한 번 그린다. */
void UShopUIWidgetBase::BindUIModel(UShopUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
		// 상점 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그린다.
		RefreshView();
		OnShopRefreshed();
	}
}

void UShopUIWidgetBase::HandleCloseClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
	CloseUI();
}

/** @brief WBP의 슬롯 구매 입력을 UIModel의 구매 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::BuyItem(int32 SlotIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestBuy(SlotIndex);
	}
}

/** @brief WBP의 나가기 입력을 UIModel의 나가기 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::Leave()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void UShopUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림 → 거래 도메인이면 뷰 갱신 후 WBP 구현 이벤트로도 전달. */
void UShopUIWidgetBase::HandleUIChanged(EShopUIDomain Domain)
{
	// 이 위젯은 거래 화면 — 프리뷰 등 다른 도메인 알림은 각자의 위젯이 처리
	if (Domain != EShopUIDomain::Trade)
	{
		return;
	}

	RefreshView();
	OnShopRefreshed();
}

void UShopUIWidgetBase::RebuildFilteredItems(const FShopUI& Shop)
{
	mFilteredShopItemIndices.Reset();
	for (int32 ShopItemIndex = 0; ShopItemIndex < Shop.mItems.Num(); ++ShopItemIndex)
	{
		const FShopItemUI& Item = Shop.mItems[ShopItemIndex];
		bool bMatches = Item.mKind == mActiveItemKind;
		if (bMatches && mActiveItemKind == EShopItemKind::Skill
			&& Shop.mOwnedUnits.IsValidIndex(mSelectedUnitViewIndex))
		{
			const EUnitJobType SelectedJob =
				Shop.mOwnedUnits[mSelectedUnitViewIndex].mJobType;
			bMatches = Item.mRequiredJobType == EUnitJobType::Common
				|| Item.mRequiredJobType == SelectedJob;
		}
		if (bMatches)
		{
			mFilteredShopItemIndices.Add(ShopItemIndex);
		}
	}

	if (mFilteredShopItemIndices.IsEmpty())
	{
		mSelectedFilteredIndex = 0;
	}
	else
	{
		mSelectedFilteredIndex = FMath::Clamp(
			mSelectedFilteredIndex, 0, mFilteredShopItemIndices.Num() - 1);
	}
}

const FShopItemUI* UShopUIWidgetBase::GetSelectedItem(const FShopUI& Shop) const
{
	if (!mFilteredShopItemIndices.IsValidIndex(mSelectedFilteredIndex))
	{
		return nullptr;
	}

	const int32 ShopItemIndex = mFilteredShopItemIndices[mSelectedFilteredIndex];
	return Shop.mItems.IsValidIndex(ShopItemIndex)
		? &Shop.mItems[ShopItemIndex]
		: nullptr;
}

void UShopUIWidgetBase::RefreshFinalShopView(const FShopUI& Shop)
{
	// 확정 WBP에서는 옛 디버그 카드 생성 박스를 남겨 호환만 하되 화면에는 그리지 않는다.
	for (UPanelWidget* LegacyBox : {
		static_cast<UPanelWidget*>(mItemBox.Get()),
		static_cast<UPanelWidget*>(mSkillItemBox.Get()),
		static_cast<UPanelWidget*>(mArtifactItemBox.Get()),
		static_cast<UPanelWidget*>(mOwnedUnitBox.Get()) })
	{
		if (LegacyBox != nullptr)
		{
			LegacyBox->ClearChildren();
			LegacyBox->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const bool bArtifactMode = mActiveItemKind == EShopItemKind::Artifact;
	const bool bSkillMode = mActiveItemKind == EShopItemKind::Skill;
	const bool bRestMode = mActiveItemKind == EShopItemKind::Heal;
	const bool bInventoryOpen = bArtifactMode && mIsArtifactInventoryOpen;
	SetShopWidgetShown(mArtifactShopPanel, bArtifactMode);
	SetShopWidgetShown(mSkillShopPanel, bSkillMode);
	SetShopWidgetShown(mRestShopPanel, bRestMode);
	SetShopButtonShown(mInventoryButton, bArtifactMode);
	if (mArtifactInventoryPanel != nullptr)
	{
		// Visible로 두어 오버레이 뒤의 구매/탭 입력까지 차단한다.
		mArtifactInventoryPanel->SetVisibility(bInventoryOpen
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
	if (mOwnedArtifactBox != nullptr)
	{
		mOwnedArtifactBox->ClearChildren();
		mOwnedArtifactBox->SetVisibility(bInventoryOpen
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (bInventoryOpen)
	{
		RefreshOwnedView(Shop);
	}

	auto SetTabVisual = [this](UButton* Button, UImage* Plate,
		UTextBlock* Text, bool bSelected)
	{
		if (Button != nullptr)
		{
			Button->SetRenderOpacity(bSelected ? 1.f : 0.72f);
		}
		if (Text != nullptr)
		{
			Text->SetColorAndOpacity(FSlateColor(bSelected
				? FLinearColor(1.f, 0.91f, 0.70f, 1.f)
				: FLinearColor(0.68f, 0.74f, 0.79f, 1.f)));
		}
		if (UTexture2D* TabPlate = ResolveTabPlate(bSelected))
		{
			SetShopImage(Plate, TabPlate);
		}
	};
	SetTabVisual(mArtifactTabButton, mArtifactTabPlate,
		mArtifactTabText, bArtifactMode);
	SetTabVisual(mSkillTabButton, mSkillTabPlate,
		mSkillTabText, bSkillMode);
	SetTabVisual(mRestTabButton, mRestTabPlate,
		mRestTabText, bRestMode);

	RefreshSkillTargetView(Shop);
	RefreshRestView(Shop);
	if (bRestMode)
	{
		mFilteredShopItemIndices.Reset();
		mRailShopItemIndices.Init(INDEX_NONE, 5);
		for (int32 RailSlot = 0; RailSlot < 5; ++RailSlot)
		{
			SetShopButtonShown(mRailButtons.IsValidIndex(RailSlot)
				? mRailButtons[RailSlot].Get() : nullptr, false);
			SetShopWidgetShown(mRailPlates.IsValidIndex(RailSlot)
				? mRailPlates[RailSlot].Get() : nullptr, false);
			SetShopWidgetShown(mRailIcons.IsValidIndex(RailSlot)
				? mRailIcons[RailSlot].Get() : nullptr, false);
			SetShopWidgetShown(mRailPriceTexts.IsValidIndex(RailSlot)
				? mRailPriceTexts[RailSlot].Get() : nullptr, false);
		}
		SetShopButtonShown(mPreviousButton, false);
		SetShopButtonShown(mNextButton, false);
		SetShopButtonShown(mBuyButton, false);
		SetShopWidgetShown(mPreviousHolder, false);
		SetShopWidgetShown(mNextHolder, false);
		SetShopWidgetShown(mBuyHolder, false);
		SetShopWidgetShown(mSelectedItemIcon, false);
		SetShopWidgetShown(mSelectedItemNameText, false);
		SetShopWidgetShown(mSelectedItemDescriptionText, false);
		SetShopWidgetShown(mSelectedItemPriceText, false);
		return;
	}

	SetShopButtonShown(mPreviousButton, true);
	SetShopButtonShown(mNextButton, true);
	SetShopButtonShown(mBuyButton, true);
	SetShopWidgetShown(mPreviousHolder, true);
	SetShopWidgetShown(mNextHolder, true);
	SetShopWidgetShown(mBuyHolder, true);
	SetShopWidgetShown(mSelectedItemNameText, true);
	SetShopWidgetShown(mSelectedItemDescriptionText, true);
	SetShopWidgetShown(mSelectedItemPriceText, true);
	RebuildFilteredItems(Shop);
	RefreshRailView(Shop);
	RefreshSelectedItemView(Shop);
}

/** @brief 선택 상품을 가운데(2번)에 고정하고 앞뒤 상품을 순환 배치한다. */
void UShopUIWidgetBase::RefreshRailView(const FShopUI& Shop)
{
	mRailShopItemIndices.Init(INDEX_NONE, 5);
	const int32 ItemCount = mFilteredShopItemIndices.Num();
	if (ItemCount > 0)
	{
		TSet<int32> AssignedFilteredIndices;
		auto AssignRailSlot = [&](int32 RailSlot, int32 FilteredIndex)
		{
			if (!mRailShopItemIndices.IsValidIndex(RailSlot)
				|| AssignedFilteredIndices.Contains(FilteredIndex))
			{
				return;
			}
			AssignedFilteredIndices.Add(FilteredIndex);
			mRailShopItemIndices[RailSlot] = mFilteredShopItemIndices[FilteredIndex];
		};

		AssignRailSlot(2, mSelectedFilteredIndex);
		for (int32 Distance = 1; Distance <= 2; ++Distance)
		{
			AssignRailSlot(2 - Distance,
				(mSelectedFilteredIndex - Distance + ItemCount) % ItemCount);
			AssignRailSlot(2 + Distance,
				(mSelectedFilteredIndex + Distance) % ItemCount);
		}
	}

	for (int32 RailSlot = 0; RailSlot < 5; ++RailSlot)
	{
		const int32 ShopItemIndex = mRailShopItemIndices.IsValidIndex(RailSlot)
			? mRailShopItemIndices[RailSlot] : INDEX_NONE;
		const bool bHasItem = Shop.mItems.IsValidIndex(ShopItemIndex);
		UButton* Button = mRailButtons.IsValidIndex(RailSlot)
			? mRailButtons[RailSlot] : nullptr;
		UImage* Plate = mRailPlates.IsValidIndex(RailSlot)
			? mRailPlates[RailSlot] : nullptr;
		UImage* Icon = mRailIcons.IsValidIndex(RailSlot)
			? mRailIcons[RailSlot] : nullptr;
		UTextBlock* PriceText = mRailPriceTexts.IsValidIndex(RailSlot)
			? mRailPriceTexts[RailSlot] : nullptr;

		const bool bCenterSlot = RailSlot == 2;
		SetShopButtonShown(Button, bHasItem);
		SetShopWidgetShown(Plate, bHasItem);
		SetShopWidgetShown(Icon, bHasItem && !bCenterSlot);
		SetShopWidgetShown(PriceText, bHasItem && !bCenterSlot);
		if (!bHasItem)
		{
			continue;
		}

		const FShopItemUI& Item = Shop.mItems[ShopItemIndex];
		if (Button != nullptr)
		{
			Button->SetIsEnabled(true);
			Button->SetRenderScale(FVector2D(1.f, 1.f));
			Button->SetRenderOpacity((Item.mIsSoldOut || !Item.mIsAffordable)
				? 0.62f : 1.f);
		}
		if (Plate != nullptr)
		{
			Plate->SetColorAndOpacity(FLinearColor::White);
		}
		if (!bCenterSlot)
		{
			SetShopImage(Icon, ResolveItemIcon(Item));
			if (Icon != nullptr)
			{
				Icon->SetColorAndOpacity((Item.mIsSoldOut || !Item.mIsAffordable)
					? FLinearColor(1.f, 1.f, 1.f, 0.42f)
					: FLinearColor::White);
			}
		}
		if (PriceText != nullptr && !bCenterSlot)
		{
			PriceText->SetText(Item.mIsSoldOut
				? LOCTEXT("SoldOut", "품절")
				: FText::Format(LOCTEXT("RailPrice", "{0} G"),
					FText::AsNumber(Item.mPrice)));
			PriceText->SetColorAndOpacity(FSlateColor(
				Item.mIsAffordable && !Item.mIsSoldOut
					? FLinearColor(1.f, 0.83f, 0.30f, 1.f)
					: FLinearColor(0.85f, 0.42f, 0.38f, 1.f)));
		}
	}

	const bool bCanNavigate = ItemCount > 1;
	if (mPreviousButton != nullptr)
	{
		mPreviousButton->SetIsEnabled(bCanNavigate);
	}
	if (mNextButton != nullptr)
	{
		mNextButton->SetIsEnabled(bCanNavigate);
	}
}

void UShopUIWidgetBase::RefreshSkillTargetView(const FShopUI& Shop)
{
	const bool bSkillMode = mActiveItemKind == EShopItemKind::Skill;
	if (Shop.mOwnedUnits.IsEmpty())
	{
		mSelectedUnitViewIndex = 0;
	}
	else
	{
		mSelectedUnitViewIndex = FMath::Clamp(
			mSelectedUnitViewIndex, 0, Shop.mOwnedUnits.Num() - 1);
	}

	for (int32 UnitViewIndex = 0; UnitViewIndex < 3; ++UnitViewIndex)
	{
		const bool bHasUnit = bSkillMode
			&& Shop.mOwnedUnits.IsValidIndex(UnitViewIndex);
		UButton* Button = mUnitSelectButtons.IsValidIndex(UnitViewIndex)
			? mUnitSelectButtons[UnitViewIndex] : nullptr;
		UImage* Plate = mUnitSelectPlates.IsValidIndex(UnitViewIndex)
			? mUnitSelectPlates[UnitViewIndex] : nullptr;
		UImage* Icon = mUnitSelectIcons.IsValidIndex(UnitViewIndex)
			? mUnitSelectIcons[UnitViewIndex] : nullptr;
		SetShopButtonShown(Button, bHasUnit);
		SetShopWidgetShown(Plate, bHasUnit);
		SetShopWidgetShown(Icon, bHasUnit);
		if (!bHasUnit)
		{
			continue;
		}

		const FShopOwnedUnitUI& Unit = Shop.mOwnedUnits[UnitViewIndex];
		if (Button != nullptr)
		{
			Button->SetIsEnabled(true);
			Button->SetRenderOpacity(UnitViewIndex == mSelectedUnitViewIndex
				? 1.f : 0.62f);
		}
		SetShopImage(Plate, ResolveUnitSelectionPlate(
			UnitViewIndex == mSelectedUnitViewIndex));
		SetShopImage(Icon, ResolveUnitIcon(Unit.mJobType));
	}

	const FShopOwnedUnitUI* SelectedUnit = bSkillMode
		&& Shop.mOwnedUnits.IsValidIndex(mSelectedUnitViewIndex)
		? &Shop.mOwnedUnits[mSelectedUnitViewIndex] : nullptr;
	if (SelectedUnit == nullptr || SelectedUnit->mSkillSlots.IsEmpty())
	{
		mSelectedSkillSlotIndex = 0;
	}
	else
	{
		const int32 AvailableReplaceableSlots = FMath::Clamp(
			SelectedUnit->mSkillSlots.Num() - ReplaceableSkillStartIndex,
			0, ReplaceableSkillSlotCount);
		mSelectedSkillSlotIndex = AvailableReplaceableSlots > 0
			? FMath::Clamp(mSelectedSkillSlotIndex, 0, AvailableReplaceableSlots - 1)
			: 0;
	}

	for (int32 SkillSlotIndex = 0; SkillSlotIndex < ReplaceableSkillSlotCount;
		++SkillSlotIndex)
	{
		const int32 ModelSkillSlotIndex = ReplaceableSkillStartIndex + SkillSlotIndex;
		const bool bHasSlot = SelectedUnit != nullptr
			&& SelectedUnit->mSkillSlots.IsValidIndex(ModelSkillSlotIndex);
		UButton* Button = mSkillSlotButtons.IsValidIndex(SkillSlotIndex)
			? mSkillSlotButtons[SkillSlotIndex] : nullptr;
		UImage* Plate = mSkillSlotPlates.IsValidIndex(SkillSlotIndex)
			? mSkillSlotPlates[SkillSlotIndex] : nullptr;
		UImage* Icon = mSkillSlotIcons.IsValidIndex(SkillSlotIndex)
			? mSkillSlotIcons[SkillSlotIndex] : nullptr;
		SetShopButtonShown(Button, bHasSlot);
		SetShopWidgetShown(Plate, bHasSlot);
		if (!bHasSlot)
		{
			SetShopWidgetShown(Icon, false);
			continue;
		}

		const FShopOwnedSkillSlotUI& SkillSlot =
			SelectedUnit->mSkillSlots[ModelSkillSlotIndex];
		if (Button != nullptr)
		{
			Button->SetIsEnabled(true);
			Button->SetRenderOpacity(SkillSlotIndex == mSelectedSkillSlotIndex
				? 1.f : 0.62f);
		}
		SetShopImage(Plate, ResolveSkillSelectionPlate(
			SkillSlotIndex == mSelectedSkillSlotIndex));
		SetShopImage(Icon, SkillSlot.mIcon);
	}

	const int32 SelectedModelSlotIndex = ReplaceableSkillStartIndex
		+ mSelectedSkillSlotIndex;
	const bool bShowSelectionPointer = SelectedUnit != nullptr
		&& SelectedUnit->mSkillSlots.IsValidIndex(SelectedModelSlotIndex);
	SetShopWidgetShown(mSkillSelectionPointer, bShowSelectionPointer);
	if (mSkillSelectionPointer != nullptr && bShowSelectionPointer)
	{
		mSkillSelectionPointer->SetRenderTranslation(
			FVector2D(142.f * mSelectedSkillSlotIndex, 0.f));
	}
}

/** @brief 휴식 탭의 3인 HP/AP 전·후 값과 바를 갱신한다. */
void UShopUIWidgetBase::RefreshRestView(const FShopUI& Shop)
{
	const bool bRestMode = mActiveItemKind == EShopItemKind::Heal;
	SetShopButtonShown(mRestButton, bRestMode);
	SetShopWidgetShown(RestCostText, bRestMode);
	if (RestCostText != nullptr && bRestMode)
	{
		RestCostText->SetText(FText::Format(LOCTEXT("RestPrice", "{0} G"),
			FText::AsNumber(Shop.mRest.mPrice)));
	}

	auto SetFillRatio = [](UImage* Fill, float Value, float Maximum, bool bShown)
	{
		SetShopWidgetShown(Fill, bShown);
		if (Fill == nullptr || !bShown)
		{
			return;
		}

		const float Ratio = Maximum > 0.f
			? FMath::Clamp(Value / Maximum, 0.f, 1.f) : 0.f;
		Fill->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		Fill->SetRenderScale(FVector2D(Ratio, 1.f));
	};

	auto SetValueText = [](UTextBlock* Text, float Value, float Maximum, bool bShown)
	{
		SetShopWidgetShown(Text, bShown);
		if (Text != nullptr && bShown)
		{
			Text->SetText(FText::FromString(FString::Printf(
				TEXT("%d/%d"), FMath::RoundToInt(Value),
				FMath::RoundToInt(Maximum))));
		}
	};

	for (int32 UnitViewIndex = 0; UnitViewIndex < 3; ++UnitViewIndex)
	{
		const bool bHasUnit = bRestMode
			&& Shop.mRest.mUnits.IsValidIndex(UnitViewIndex);
		UWidget* RowHolder = mRestUnitRowHolders.IsValidIndex(UnitViewIndex)
			? mRestUnitRowHolders[UnitViewIndex].Get() : nullptr;
		UImage* Plate = mRestUnitPlates.IsValidIndex(UnitViewIndex)
			? mRestUnitPlates[UnitViewIndex].Get() : nullptr;
		UImage* Icon = mRestUnitIcons.IsValidIndex(UnitViewIndex)
			? mRestUnitIcons[UnitViewIndex].Get() : nullptr;
		UTextBlock* HPBeforeText = mRestUnitHPBeforeTexts.IsValidIndex(UnitViewIndex)
			? mRestUnitHPBeforeTexts[UnitViewIndex].Get() : nullptr;
		UTextBlock* HPAfterText = mRestUnitHPAfterTexts.IsValidIndex(UnitViewIndex)
			? mRestUnitHPAfterTexts[UnitViewIndex].Get() : nullptr;
		UTextBlock* APBeforeText = mRestUnitAPBeforeTexts.IsValidIndex(UnitViewIndex)
			? mRestUnitAPBeforeTexts[UnitViewIndex].Get() : nullptr;
		UTextBlock* APAfterText = mRestUnitAPAfterTexts.IsValidIndex(UnitViewIndex)
			? mRestUnitAPAfterTexts[UnitViewIndex].Get() : nullptr;
		UImage* HPBeforeFill = mRestUnitHPBeforeFills.IsValidIndex(UnitViewIndex)
			? mRestUnitHPBeforeFills[UnitViewIndex].Get() : nullptr;
		UImage* HPAfterFill = mRestUnitHPAfterFills.IsValidIndex(UnitViewIndex)
			? mRestUnitHPAfterFills[UnitViewIndex].Get() : nullptr;
		UImage* APBeforeFill = mRestUnitAPBeforeFills.IsValidIndex(UnitViewIndex)
			? mRestUnitAPBeforeFills[UnitViewIndex].Get() : nullptr;
		UImage* APAfterFill = mRestUnitAPAfterFills.IsValidIndex(UnitViewIndex)
			? mRestUnitAPAfterFills[UnitViewIndex].Get() : nullptr;

		SetShopWidgetShown(RowHolder, bHasUnit);
		SetShopWidgetShown(Plate, bHasUnit);
		if (!bHasUnit)
		{
			SetShopWidgetShown(Icon, false);
			SetValueText(HPBeforeText, 0.f, 0.f, false);
			SetValueText(HPAfterText, 0.f, 0.f, false);
			SetValueText(APBeforeText, 0.f, 0.f, false);
			SetValueText(APAfterText, 0.f, 0.f, false);
			SetFillRatio(HPBeforeFill, 0.f, 0.f, false);
			SetFillRatio(HPAfterFill, 0.f, 0.f, false);
			SetFillRatio(APBeforeFill, 0.f, 0.f, false);
			SetFillRatio(APAfterFill, 0.f, 0.f, false);
			continue;
		}

		const FShopRestUnitUI& Unit = Shop.mRest.mUnits[UnitViewIndex];
		SetShopImage(Icon, ResolveUnitIcon(Unit.mJobType));
		SetValueText(HPBeforeText, Unit.mHP, Unit.mMaxHP, true);
		SetValueText(HPAfterText, Unit.mMaxHP, Unit.mMaxHP, true);
		SetValueText(APBeforeText, Unit.mAP, Unit.mMaxAP, true);
		SetValueText(APAfterText, Unit.mMaxAP, Unit.mMaxAP, true);
		SetFillRatio(HPBeforeFill, Unit.mHP, Unit.mMaxHP, true);
		SetFillRatio(HPAfterFill, Unit.mMaxHP, Unit.mMaxHP, true);
		SetFillRatio(APBeforeFill, Unit.mAP, Unit.mMaxAP, true);
		SetFillRatio(APAfterFill, Unit.mMaxAP, Unit.mMaxAP, true);
	}

	if (mRestButton == nullptr || !bRestMode)
	{
		return;
	}

	const bool bCanRest = !Shop.mRest.mIsUsed
		&& Shop.mRest.mIsAffordable
		&& !Shop.mRest.mUnits.IsEmpty();
	mRestButton->SetIsEnabled(bCanRest);
	if (mRestButtonText != nullptr)
	{
		if (Shop.mRest.mIsUsed)
		{
			mRestButtonText->SetText(LOCTEXT("RestUsed", "휴식 완료"));
		}
		else if (!Shop.mRest.mIsAffordable)
		{
			mRestButtonText->SetText(LOCTEXT("RestNotAffordable", "골드 부족"));
		}
		else
		{
			mRestButtonText->SetText(LOCTEXT("RestNow", "휴식하기"));
		}
	}
}

void UShopUIWidgetBase::RefreshSelectedItemView(const FShopUI& Shop)
{
	const FShopItemUI* Item = GetSelectedItem(Shop);
	SetShopWidgetShown(mSelectedItemIcon, Item != nullptr);
	if (Item == nullptr)
	{
		if (mSelectedItemNameText != nullptr)
		{
			mSelectedItemNameText->SetText(LOCTEXT("NoShopItems", "판매 상품이 없습니다"));
		}
		if (mSelectedItemDescriptionText != nullptr)
		{
			mSelectedItemDescriptionText->SetText(FText::GetEmpty());
		}
		if (mSelectedItemPriceText != nullptr)
		{
			mSelectedItemPriceText->SetText(LOCTEXT("NoSelectedPrice", "-"));
		}
		if (mBuyButton != nullptr)
		{
			mBuyButton->SetIsEnabled(false);
		}
		if (mBuyButtonText != nullptr)
		{
			mBuyButtonText->SetText(LOCTEXT("Unavailable", "구매 불가"));
		}
		return;
	}

	SetShopImage(mSelectedItemIcon, ResolveItemIcon(*Item));
	if (mSelectedItemIcon != nullptr)
	{
		mSelectedItemIcon->SetColorAndOpacity((Item->mIsSoldOut || !Item->mIsAffordable)
			? FLinearColor(1.f, 1.f, 1.f, 0.48f)
			: FLinearColor::White);
	}
	if (mSelectedItemNameText != nullptr)
	{
		mSelectedItemNameText->SetText(Item->mName);
		mSelectedItemNameText->SetColorAndOpacity(FSlateColor(Item->mRarityColor));
	}
	if (mSelectedItemDescriptionText != nullptr)
	{
		const FText FallbackDescription = Item->mKind == EShopItemKind::Artifact
			? LOCTEXT("ArtifactFallbackDescription", "파티 전체에 적용되는 아티팩트입니다.")
			: LOCTEXT("SkillFallbackDescription", "선택한 용병의 스킬 슬롯에 장착합니다.");
		mSelectedItemDescriptionText->SetText(Item->mDescription.IsEmpty()
			? FallbackDescription : Item->mDescription);
	}
	if (mSelectedItemPriceText != nullptr)
	{
		mSelectedItemPriceText->SetText(Item->mIsSoldOut
			? LOCTEXT("SelectedSoldOut", "품절")
			: FText::Format(LOCTEXT("SelectedPrice", "{0} G"),
				FText::AsNumber(Item->mPrice)));
	}

	bool bHasSkillTarget = true;
	if (Item->mKind == EShopItemKind::Skill)
	{
		bHasSkillTarget = Shop.mOwnedUnits.IsValidIndex(mSelectedUnitViewIndex)
			&& Shop.mOwnedUnits[mSelectedUnitViewIndex]
				.mSkillSlots.IsValidIndex(
					ReplaceableSkillStartIndex + mSelectedSkillSlotIndex);
	}
	const bool bCanBuy = !Item->mIsSoldOut && Item->mIsAffordable && bHasSkillTarget;
	if (mBuyButton != nullptr)
	{
		mBuyButton->SetIsEnabled(bCanBuy);
	}
	if (mBuyButtonText != nullptr)
	{
		if (Item->mIsSoldOut)
		{
			mBuyButtonText->SetText(LOCTEXT("BuySoldOut", "품절"));
		}
		else if (!Item->mIsAffordable)
		{
			mBuyButtonText->SetText(LOCTEXT("BuyNotAffordable", "골드 부족"));
		}
		else if (!bHasSkillTarget)
		{
			mBuyButtonText->SetText(LOCTEXT("BuySelectTarget", "대상 선택"));
		}
		else
		{
			if (Item->mKind == EShopItemKind::Skill)
			{
				const FShopOwnedSkillSlotUI& TargetSlot =
					Shop.mOwnedUnits[mSelectedUnitViewIndex]
						.mSkillSlots[ReplaceableSkillStartIndex
							+ mSelectedSkillSlotIndex];
				mBuyButtonText->SetText(TargetSlot.mIsEmpty
					? LOCTEXT("EquipSelectedSkill", "장착")
					: LOCTEXT("ReplaceSelectedSkill", "교체"));
			}
			else
			{
				mBuyButtonText->SetText(LOCTEXT("BuySelected", "구매"));
			}
		}
	}
}

/** @brief 현재 모델의 골드/판매 슬롯을 BindWidget 위젯(골드 라벨 + 항목 박스)에 반영한다. */
void UShopUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FShopUI& Shop = mUIModel->GetShop();

	if (mGoldText != nullptr)
	{
		mGoldText->SetText(FText::Format(
			LOCTEXT("GoldAmount", "{0} G"),
			FText::AsNumber(Shop.mGold)));
	}

	if (HasFinalShopLayout())
	{
		RefreshFinalShopView(Shop);
		return;
	}

	// 소지품(파티 아티펙트 + 유닛 카드)은 판매 박스 유무와 무관하게 갱신
	RefreshOwnedView(Shop);

	// 종류별 박스가 WBP에 없으면 공용 박스(mItemBox)로 폴백 — WBP를 단계적으로 바꿔도 동작 유지
	if (mItemBox == nullptr && mSkillItemBox == nullptr && mArtifactItemBox == nullptr)
	{
		return;
	}
	if (mItemBox != nullptr)
	{
		mItemBox->ClearChildren();
	}
	if (mSkillItemBox != nullptr)
	{
		mSkillItemBox->ClearChildren();
	}
	if (mArtifactItemBox != nullptr)
	{
		mArtifactItemBox->ClearChildren();
	}

	for (int32 ItemIndex = 0; ItemIndex < Shop.mItems.Num(); ++ItemIndex)
	{
		const FShopItemUI& Item = Shop.mItems[ItemIndex];

		// 카드가 들어갈 박스 결정. 미분류 종류(용병 등 추후 추가분)도 공용 박스로
		UPanelWidget* TargetBox = nullptr;
		switch (Item.mKind)
		{
		case EShopItemKind::Skill:
			TargetBox = mSkillItemBox;
			break;
		case EShopItemKind::Artifact:
			TargetBox = mArtifactItemBox;
			break;
		default:
			break;
		}
		if (TargetBox == nullptr)
		{
			TargetBox = mItemBox;
		}
		if (TargetBox == nullptr)
		{
			continue;
		}

		// 슬롯 카드 = 세로 박스(아이콘 + 이름 + 가격). 종류별 아이콘 자동 매핑.
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (Card == nullptr)
		{
			continue;
		}

		// 슬롯 인덱스 라벨. BuyItem(SlotIndex)에 넣을 모델 인덱스 확인용(테스트 UI)
		if (UTextBlock* IndexLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			IndexLabel->SetText(FText::Format(LOCTEXT("[{0}]", "[{0}]"), FText::AsNumber(ItemIndex)));
			IndexLabel->SetJustification(ETextJustify::Center);
			SetCardFontSize(IndexLabel, CardTextFontSize);
			if (UVerticalBoxSlot* IndexSlot = Card->AddChildToVerticalBox(IndexLabel))
			{
				IndexSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			if (UTexture2D* Tex = ResolveItemIcon(Item))
			{
				Icon->SetBrushFromTexture(Tex, false);
			}
			Icon->SetDesiredSizeOverride(FVector2D(112.f, 112.f));
			// 품절/구매불가는 흐리게.
			Icon->SetColorAndOpacity((Item.mIsSoldOut || !Item.mIsAffordable)
				? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White);
			Card->AddChildToVerticalBox(Icon);
		}

		if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Name->SetText(Item.mName);
			Name->SetJustification(ETextJustify::Center);
			SetCardFontSize(Name, CardHeaderFontSize);
			if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
			{
				NameSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
				NameSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UTextBlock* Price = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			const FText PriceText = Item.mIsSoldOut
				? LOCTEXT("Sold Out", "Sold Out")
				: FText::Format(LOCTEXT("{0} G", "{0} G"), FText::AsNumber(Item.mPrice));
			Price->SetText(PriceText);
			Price->SetJustification(ETextJustify::Center);
			SetCardFontSize(Price, CardTextFontSize);
			Price->SetColorAndOpacity(FSlateColor(Item.mIsAffordable && !Item.mIsSoldOut
				? FLinearColor(0.95f, 0.85f, 0.45f, 1.f) : FLinearColor(0.8f, 0.4f, 0.4f, 1.f)));
			if (UVerticalBoxSlot* PriceSlot = Card->AddChildToVerticalBox(Price))
			{
				PriceSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
				PriceSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// 박스 종류(수평/줄바꿈)에 따라 슬롯 타입이 달라 캐스팅으로 여백 적용
		UPanelSlot* CardSlot = TargetBox->AddChild(Card);
		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(CardSlot))
		{
			HorizontalSlot->SetPadding(FMargin(14.f, 0.f));
		}
		else if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(CardSlot))
		{
			WrapSlot->SetPadding(FMargin(14.f, 8.f));
		}
	}
}

/**
 * @brief 소지 아티펙트(파티 소유)와 파티 유닛 카드를 소지 박스에 반영한다.
 * @details
 * 아티펙트 카드의 [N]은 파티 배열 index — RD.ShopDiscardArtifact payload 그대로.
 * 유닛 카드는 [유닛 index] 직업 Lv 헤더 + 슬롯별 스킬 줄(S슬롯: 이름, 빈 슬롯은 '-') —
 * (유닛, 슬롯) 쌍이 스킬 구매/버리기 명령 payload 그대로.
 */
void UShopUIWidgetBase::RefreshOwnedView(const FShopUI& Shop)
{
	// 소지 아티펙트 — 아이콘 + [N] + 이름(희귀도 색)
	if (mOwnedArtifactBox != nullptr)
	{
		mOwnedArtifactBox->ClearChildren();
		mOwnedArtifactBox->SetInnerSlotPadding(FVector2D(22.f, 20.f));
		mOwnedArtifactBox->SetHorizontalAlignment(HAlign_Center);
		for (const FShopOwnedArtifactUI& Artifact : Shop.mOwnedArtifacts)
		{
			USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass());
			UOverlay* CardFrame = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass());
			UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			if (CardSize == nullptr || CardFrame == nullptr || Card == nullptr)
			{
				continue;
			}
			CardSize->SetWidthOverride(205.f);
			CardSize->SetHeightOverride(230.f);
			CardSize->AddChild(CardFrame);

			if (UImage* Plate = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
			{
				Plate->SetBrushFromTexture(ResolveOwnedArtifactPlate(), false);
				if (UOverlaySlot* PlateSlot = CardFrame->AddChildToOverlay(Plate))
				{
					PlateSlot->SetHorizontalAlignment(HAlign_Fill);
					PlateSlot->SetVerticalAlignment(VAlign_Fill);
				}
			}
			if (UOverlaySlot* ContentSlot = CardFrame->AddChildToOverlay(Card))
			{
				ContentSlot->SetPadding(FMargin(20.f, 18.f));
				ContentSlot->SetHorizontalAlignment(HAlign_Fill);
				ContentSlot->SetVerticalAlignment(VAlign_Fill);
			}

			if (UTextBlock* IndexLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				IndexLabel->SetText(FText::Format(LOCTEXT("[{0}]", "[{0}]"), FText::AsNumber(Artifact.mArtifactIndex)));
				IndexLabel->SetJustification(ETextJustify::Center);
				SetCardFontSize(IndexLabel, CardHeaderFontSize);
				if (UVerticalBoxSlot* IndexSlot = Card->AddChildToVerticalBox(IndexLabel))
				{
					IndexSlot->SetHorizontalAlignment(HAlign_Center);
				}
			}

			if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
			{
				if (Artifact.mIcon != nullptr)
				{
					Icon->SetBrushFromTexture(Artifact.mIcon, false);
				}
				// 판매 카드(112px)와 구분되게 소지품은 작게
				Icon->SetDesiredSizeOverride(FVector2D(142.f, 142.f));
				if (UVerticalBoxSlot* IconSlot = Card->AddChildToVerticalBox(Icon))
				{
					IconSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 4.f));
					IconSlot->SetHorizontalAlignment(HAlign_Center);
					IconSlot->SetVerticalAlignment(VAlign_Center);
					IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}
			}

			if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				Name->SetText(Artifact.mName);
				Name->SetJustification(ETextJustify::Center);
				Name->SetColorAndOpacity(FSlateColor(Artifact.mRarityColor));
				SetCardFontSize(Name, CardHeaderFontSize);
				if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
				{
					NameSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
					NameSlot->SetHorizontalAlignment(HAlign_Center);
				}
			}

			if (UWrapBoxSlot* CardSlot = mOwnedArtifactBox->AddChildToWrapBox(CardSize))
			{
				CardSlot->SetPadding(FMargin(8.f));
			}
		}
	}

	// 파티 유닛 카드 — 헤더 + 슬롯별 스킬 줄
	if (mOwnedUnitBox != nullptr)
	{
		mOwnedUnitBox->ClearChildren();
		for (const FShopOwnedUnitUI& Unit : Shop.mOwnedUnits)
		{
			UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			if (Card == nullptr)
			{
				continue;
			}

			if (UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				Header->SetText(FText::Format(
					LOCTEXT("[{0}] {1} Lv.{2}", "[{0}] {1} Lv.{2}"),
					FText::AsNumber(Unit.mUnitIndex),
					UEnum::GetDisplayValueAsText(Unit.mJobType),
					FText::AsNumber(Unit.mLevel)));
				SetCardFontSize(Header, CardHeaderFontSize);
				Card->AddChildToVerticalBox(Header);
			}

			for (int32 SlotIndex = 0; SlotIndex < Unit.mSkillSlots.Num(); ++SlotIndex)
			{
				const FShopOwnedSkillSlotUI& SkillSlot = Unit.mSkillSlots[SlotIndex];
				if (UTextBlock* SlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
				{
					SlotText->SetText(FText::Format(
						LOCTEXT("S{0}: {1}", "S{0}: {1}"),
						FText::AsNumber(SlotIndex),
						SkillSlot.mIsEmpty ? LOCTEXT("-", "-") : SkillSlot.mName));
					// 빈 슬롯은 흐리게 — 구매로 채울 자리
					SlotText->SetColorAndOpacity(FSlateColor(SkillSlot.mIsEmpty
						? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White));
					SetCardFontSize(SlotText, CardTextFontSize);
					if (UVerticalBoxSlot* TextSlot = Card->AddChildToVerticalBox(SlotText))
					{
						TextSlot->SetPadding(FMargin(12.f, 2.f, 0.f, 0.f));
					}
				}
			}

			if (UWrapBoxSlot* CardSlot = mOwnedUnitBox->AddChildToWrapBox(Card))
			{
				CardSlot->SetPadding(FMargin(10.f, 6.f));
			}
		}
	}
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UShopUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
