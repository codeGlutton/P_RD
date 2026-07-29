#include "UI/Inventory/InventoryUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameMode/RoomGameModeBase.h"
#include "UI/Inventory/InventoryUIModel.h"
#include "UI/RoomViewTypes.h"
#include "UI/ViewportZOrderType.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "InventoryUIWidgetBase"

namespace
{
	constexpr float InventoryDesignWidth = 1672.f;
	constexpr float InventoryDesignHeight = 941.f;

	const FLinearColor InventoryBackgroundColor(0.008f, 0.016f, 0.027f, 1.f);
	const FLinearColor InventoryNavyCardColor(0.018f, 0.047f, 0.088f, 0.76f);
	const FLinearColor InventoryParchmentCardColor(0.94f, 0.82f, 0.59f, 0.28f);
	const FLinearColor InventoryGoldColor(0.96f, 0.72f, 0.24f, 1.f);
	const FLinearColor InventoryCreamColor(0.96f, 0.90f, 0.77f, 1.f);
	const FLinearColor InventoryNavyTextColor(0.035f, 0.090f, 0.16f, 1.f);
	const FLinearColor InventoryParchmentMutedColor(0.16f, 0.23f, 0.29f, 0.95f);
	const FLinearColor InventoryMutedTextColor(0.68f, 0.76f, 0.79f, 1.f);
	const FLinearColor InventoryExpColor(0.12f, 0.61f, 0.88f, 1.f);

	void SetTextStyle(UTextBlock* Text, int32 Size, const FLinearColor& Color)
	{
		if (Text == nullptr)
		{
			return;
		}

		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
	}

	void SetHorizontalFill(UHorizontalBoxSlot* Slot)
	{
		if (Slot == nullptr)
		{
			return;
		}

		FSlateChildSize Size;
		Size.SizeRule = ESlateSizeRule::Fill;
		Size.Value = 1.f;
		Slot->SetSize(Size);
	}

	void PlaceOnDesignCanvas(
		UCanvasPanel* Canvas,
		UWidget* Widget,
		const FVector2D& Position,
		const FVector2D& Size,
		int32 ZOrder = 1)
	{
		if (Canvas == nullptr || Widget == nullptr)
		{
			return;
		}

		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
		{
			Slot->SetAnchors(FAnchors(0.f, 0.f));
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(ZOrder);
		}
	}

	FInventoryItemUI MakeInventoryItem(
		const FInventoryRowView& Row,
		EInventoryItemKind Kind)
	{
		FInventoryItemUI Item;
		Item.mKind = Kind;
		Item.mItemIndex = Row.mIndex;
		Item.mName = Row.mName;
		Item.mIcon = Row.mIcon;
		Item.mRarityColor = Row.mRarityColor;
		Item.mDetailText = Row.mDetail;
		return Item;
	}
}

UInventoryUIWidgetBase::UInventoryUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

	// CDO의 강한 참조로 남겨 WBP가 이 프로퍼티를 따로 설정하지 않아도
	// 인벤토리 배경이 APK cook 대상에 포함된다.
	static ConstructorHelpers::FObjectFinder<UTexture2D> BackgroundFinder(
		TEXT("/Game/UI/Art/RunFlow/T_Inventory_Background_Current.T_Inventory_Background_Current"));
	if (BackgroundFinder.Succeeded())
	{
		mDefaultBackgroundArt = BackgroundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> SkillIconFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_CombatHUD_SkillIcon_Basic.T_CombatHUD_SkillIcon_Basic"));
	if (SkillIconFinder.Succeeded())
	{
		mDefaultSkillIcon = SkillIconFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> EquipmentIconFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common"));
	if (EquipmentIconFinder.Succeeded())
	{
		mDefaultEquipmentIcon = EquipmentIconFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> ArtifactIconFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Treasure.T_MapNode_Treasure"));
	if (ArtifactIconFinder.Succeeded())
	{
		mDefaultArtifactIcon = ArtifactIconFinder.Object;
	}
}

UTexture2D* UInventoryUIWidgetBase::ResolveInventoryIcon(const FInventoryItemUI& Item) const
{
	if (Item.mIcon != nullptr)
	{
		return Item.mIcon;
	}

	switch (Item.mKind)
	{
	case EInventoryItemKind::Skill:
		return mDefaultSkillIcon;
	case EInventoryItemKind::Equipment:
		return mDefaultEquipmentIcon;
	case EInventoryItemKind::Artifact:
		return mDefaultArtifactIcon;
	default:
		return nullptr;
	}
}

/**
 * @brief 전용 아트 WBP가 아직 제목/닫기만 가진 상태여도 코드 fallback을 만든 뒤
 *        버튼과 고정 라벨을 배선한다.
 */
void UInventoryUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureFallbackLayout();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &UInventoryUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(LOCTEXT("Close", "CLOSE"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(LOCTEXT("Inventory", "INVENTORY"));
	}
	if (mPartyLabel != nullptr)
	{
		mPartyLabel->SetText(LOCTEXT("Mercenaries", "MERCENARIES"));
	}
	if (mSkillLabel != nullptr)
	{
		mSkillLabel->SetText(LOCTEXT("AcquiredSkills", "ACQUIRED SKILLS"));
	}
	if (mEquipLabel != nullptr)
	{
		mEquipLabel->SetText(LOCTEXT("AcquiredEquipment", "ACQUIRED EQUIPMENT"));
	}
	if (mArtifactLabel != nullptr)
	{
		mArtifactLabel->SetText(LOCTEXT("Artifacts", "ARTIFACTS"));
	}

	RefreshView();
}

/**
 * @brief AddToViewport로 NativeConstruct가 끝난 뒤 실제 GameMode 값을 읽는다.
 *        닫았다 다시 열 때도 매번 실행되어 직전 전투에서 받은 골드/EXP/보상이 바로 보인다.
 */
void UInventoryUIWidgetBase::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	RefreshFromCurrentRoom();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 스냅샷도 즉시 그린다. */
void UInventoryUIWidgetBase::BindUIModel(UInventoryUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		RefreshView();
		return;
	}

	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UInventoryUIWidgetBase::HandleUIChanged);
		RefreshView();
		OnInventoryRefreshed();
	}
}

void UInventoryUIWidgetBase::HandleCloseClicked()
{
	CloseUI();
}

void UInventoryUIWidgetBase::LongPressItem(EInventoryItemKind Kind, int32 ItemIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestItemLongPress(Kind, ItemIndex);
	}
}

int32 UInventoryUIWidgetBase::GetDisplayedGold() const
{
	return mUIModel != nullptr ? mUIModel->GetInventory().mGold : 0;
}

int32 UInventoryUIWidgetBase::GetMercenaryRowCount() const
{
	return mUIModel != nullptr ? mUIModel->GetInventory().mMercenaries.Num() : 0;
}

int32 UInventoryUIWidgetBase::GetAcquiredItemCount() const
{
	if (mUIModel == nullptr)
	{
		return 0;
	}

	const FInventoryUI& Inventory = mUIModel->GetInventory();
	return Inventory.mSkills.Num() + Inventory.mEquipment.Num() + Inventory.mArtifacts.Num();
}

bool UInventoryUIWidgetBase::HasBackgroundArt() const
{
	return mBackgroundArt.IsNull() == false || mDefaultBackgroundArt != nullptr;
}

bool UInventoryUIWidgetBase::HasFallbackItemIcons() const
{
	return mDefaultSkillIcon != nullptr
		&& mDefaultEquipmentIcon != nullptr
		&& mDefaultArtifactIcon != nullptr;
}

void UInventoryUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UInventoryUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

void UInventoryUIWidgetBase::HandleUIChanged()
{
	RefreshView();
	OnInventoryRefreshed();
}

/**
 * @brief RoomGameMode의 읽기 전용 DTO를 화면 모델로 옮긴다.
 *
 * @details 인벤토리 화면이 AttributeSet이나 RunPersistData를 직접 알지 않게 유지한다.
 *          테스트/에디터 미리보기에서 외부 모델이 이미 바인딩된 경우에는 그 모델을 덮지 않는다.
 */
void UInventoryUIWidgetBase::RefreshFromCurrentRoom()
{
	if (mUIModel != nullptr && mUIModel != mRuntimeUIModel)
	{
		return;
	}

	if (mRuntimeUIModel == nullptr)
	{
		mRuntimeUIModel = NewObject<UInventoryUIModel>(this);
	}
	BindUIModel(mRuntimeUIModel);

	FInventoryUI Inventory;
	const UWorld* World = GetWorld();
	const ARoomGameModeBase* RoomGameMode = World != nullptr
		? Cast<ARoomGameModeBase>(World->GetAuthGameMode()) : nullptr;

	FInventoryView View;
	if (RoomGameMode == nullptr || RoomGameMode->GetInventoryView(View) == false)
	{
		mRuntimeUIModel->SetInventory(Inventory);
		return;
	}

	Inventory.mGold = View.mGold;
	Inventory.mMercenaries.Reserve(View.mMercenaries.Num());
	for (const FInventoryMercenaryView& Source : View.mMercenaries)
	{
		FInventoryMercenaryUI& Target = Inventory.mMercenaries.AddDefaulted_GetRef();
		Target.mPartyIndex = Source.mPartyIndex;
		Target.mName = Source.mName;
		Target.mPortrait = Source.mPortrait;
		Target.mLevel = Source.mLevel;
		Target.mExp = Source.mExp;
		Target.mMaxExp = Source.mMaxExp;
		Target.mHP = Source.mHP;
		Target.mMaxHP = Source.mMaxHP;
	}

	Inventory.mSkills.Reserve(View.mSkills.Num());
	for (const FInventoryRowView& Row : View.mSkills)
	{
		Inventory.mSkills.Add(MakeInventoryItem(Row, EInventoryItemKind::Skill));
	}
	Inventory.mEquipment.Reserve(View.mEquipment.Num());
	for (const FInventoryRowView& Row : View.mEquipment)
	{
		Inventory.mEquipment.Add(MakeInventoryItem(Row, EInventoryItemKind::Equipment));
	}
	Inventory.mArtifacts.Reserve(View.mArtifacts.Num());
	for (const FInventoryRowView& Row : View.mArtifacts)
	{
		Inventory.mArtifacts.Add(MakeInventoryItem(Row, EInventoryItemKind::Artifact));
	}

	mRuntimeUIModel->SetInventory(Inventory);
}

/**
 * @brief 전용 WBP 계약이 갖춰지기 전 사용하는 완전한 모바일용 fallback.
 *
 * @details WBP에 mPartyBox/mSkillBox/...가 생기면 이 함수는 아무 것도 만들지 않는다.
 *          새 배경 아트는 mBackgroundArt에 선택적으로 지정하며, 없으면 단색 배경으로 동작한다.
 */
void UInventoryUIWidgetBase::EnsureFallbackLayout()
{
	if (mFallbackLayoutRoot != nullptr
		|| (mMetaText != nullptr
			&& mPartyBox != nullptr
			&& mSkillBox != nullptr
			&& mEquipBox != nullptr
			&& mArtifactBox != nullptr))
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree != nullptr
		? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (RootCanvas == nullptr)
	{
		return;
	}

	// ScaleToFit 바깥 레터박스. 세로 화면에서도 남는 공간을 같은 남색으로 채운다.
	UBorder* ScreenBackdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("InventoryScreenBackdrop"));
	ScreenBackdrop->SetBrushColor(InventoryBackgroundColor);
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(ScreenBackdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackdropSlot->SetOffsets(FMargin(0.f));
		BackdropSlot->SetZOrder(40);
	}

	// 배경과 모든 터치 영역을 같은 1672x941 좌표계에 둔다. 화면 비율이 바뀌어도
	// 그림만 따로 늘어나거나 닫기 버튼이 프레임 밖으로 밀리지 않는다.
	UScaleBox* DesignScale = WidgetTree->ConstructWidget<UScaleBox>(
		UScaleBox::StaticClass(), TEXT("InventoryDesignScale"));
	DesignScale->SetStretch(EStretch::ScaleToFit);
	DesignScale->SetStretchDirection(EStretchDirection::Both);
	if (UCanvasPanelSlot* DesignScaleSlot = RootCanvas->AddChildToCanvas(DesignScale))
	{
		DesignScaleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DesignScaleSlot->SetOffsets(FMargin(0.f));
		DesignScaleSlot->SetZOrder(41);
	}
	mFallbackLayoutRoot = DesignScale;

	USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("InventoryDesignSize"));
	DesignSize->SetWidthOverride(InventoryDesignWidth);
	DesignSize->SetHeightOverride(InventoryDesignHeight);
	DesignScale->AddChild(DesignSize);

	UCanvasPanel* DesignCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("InventoryDesignCanvas"));
	DesignSize->AddChild(DesignCanvas);

	UTexture2D* BackgroundTexture = mBackgroundArt.IsNull() == false
		? mBackgroundArt.LoadSynchronous()
		: mDefaultBackgroundArt.Get();
	if (BackgroundTexture != nullptr)
	{
		UImage* Background = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("InventoryFallbackBackground"));
		Background->SetBrushFromTexture(BackgroundTexture, true);
		Background->SetColorAndOpacity(FLinearColor::White);
		PlaceOnDesignCanvas(
			DesignCanvas,
			Background,
			FVector2D::ZeroVector,
			FVector2D(InventoryDesignWidth, InventoryDesignHeight),
			0);
	}
	else
	{
		UBorder* MissingArtFallback = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("InventoryMissingArtFallback"));
		MissingArtFallback->SetBrushColor(InventoryBackgroundColor);
		PlaceOnDesignCanvas(
			DesignCanvas,
			MissingArtFallback,
			FVector2D::ZeroVector,
			FVector2D(InventoryDesignWidth, InventoryDesignHeight),
			0);
	}

	mTitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimeTitle"));
	SetTextStyle(mTitleText, 38, InventoryNavyTextColor);
	mTitleText->SetJustification(ETextJustify::Center);
	PlaceOnDesignCanvas(
		DesignCanvas, mTitleText,
		FVector2D(360.f, 52.f), FVector2D(970.f, 62.f));

	mMetaText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimeGold"));
	SetTextStyle(mMetaText, 30, InventoryGoldColor);
	mMetaText->SetJustification(ETextJustify::Center);
	PlaceOnDesignCanvas(
		DesignCanvas, mMetaText,
		FVector2D(1280.f, 304.f), FVector2D(285.f, 58.f));

	mCloseButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), TEXT("InventoryRuntimeCloseButton"));
	mCloseButton->SetBackgroundColor(FLinearColor::Transparent);
	mCloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimeCloseText"));
	SetTextStyle(mCloseButtonText, 24, InventoryCreamColor);
	mCloseButtonText->SetJustification(ETextJustify::Center);
	mCloseButton->AddChild(mCloseButtonText);
	// 아트의 우측 하단 나무 버튼 전체를 히트 영역으로 사용한다.
	PlaceOnDesignCanvas(
		DesignCanvas, mCloseButton,
		FVector2D(1275.f, 765.f), FVector2D(305.f, 100.f), 3);

	mPartyLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimePartyLabel"));
	SetTextStyle(mPartyLabel, 22, InventoryNavyTextColor);
	mPartyLabel->SetJustification(ETextJustify::Center);
	PlaceOnDesignCanvas(
		DesignCanvas, mPartyLabel,
		FVector2D(94.f, 159.f), FVector2D(310.f, 38.f));

	mPartyBox = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("InventoryRuntimePartyBox"));
	PlaceOnDesignCanvas(
		DesignCanvas, mPartyBox,
		FVector2D(83.f, 202.f), FVector2D(330.f, 600.f));

	auto AddRewardSection = [this, DesignCanvas](
		const TCHAR* LabelName,
		const TCHAR* BoxName,
		TObjectPtr<UTextBlock>& OutLabel,
		TObjectPtr<UHorizontalBox>& OutBox,
		const FVector2D& LabelPosition,
		const FVector2D& LabelSize,
		const FVector2D& SectionPosition,
		const FVector2D& SectionSize)
	{
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		SetTextStyle(OutLabel, 23, InventoryGoldColor);
		PlaceOnDesignCanvas(DesignCanvas, OutLabel, LabelPosition, LabelSize, 2);

		USizeBox* SectionFrame = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString::Printf(TEXT("%sSize"), BoxName));
		SectionFrame->SetWidthOverride(SectionSize.X);
		SectionFrame->SetHeightOverride(SectionSize.Y);
		PlaceOnDesignCanvas(
			DesignCanvas, SectionFrame,
			SectionPosition, SectionSize, 2);

		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
			UScrollBox::StaticClass(), *FString::Printf(TEXT("%sScroll"), BoxName));
		Scroll->SetOrientation(Orient_Horizontal);
		SectionFrame->AddChild(Scroll);

		OutBox = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), BoxName);
		Scroll->AddChild(OutBox);
	};

	AddRewardSection(
		TEXT("InventoryRuntimeSkillLabel"), TEXT("InventoryRuntimeSkillBox"),
		mSkillLabel, mSkillBox,
		FVector2D(492.f, 204.f), FVector2D(680.f, 38.f),
		FVector2D(480.f, 246.f), FVector2D(705.f, 215.f));
	AddRewardSection(
		TEXT("InventoryRuntimeEquipLabel"), TEXT("InventoryRuntimeEquipBox"),
		mEquipLabel, mEquipBox,
		FVector2D(492.f, 491.f), FVector2D(680.f, 38.f),
		FVector2D(480.f, 533.f), FVector2D(705.f, 245.f));
	AddRewardSection(
		TEXT("InventoryRuntimeArtifactLabel"), TEXT("InventoryRuntimeArtifactBox"),
		mArtifactLabel, mArtifactBox,
		FVector2D(1276.f, 398.f), FVector2D(295.f, 38.f),
		FVector2D(1265.f, 443.f), FVector2D(315.f, 285.f));
}

/** @brief 한 획득 보상 섹션을 아이콘 카드로 채운다. */
void UInventoryUIWidgetBase::FillSection(
	UHorizontalBox* Box,
	const TArray<FInventoryItemUI>& Items)
{
	if (Box == nullptr)
	{
		return;
	}
	Box->ClearChildren();

	if (Items.IsEmpty())
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(LOCTEXT("NoAcquiredItems", "No acquired items"));
		SetTextStyle(EmptyText, 15, InventoryMutedTextColor);
		if (UHorizontalBoxSlot* EmptySlot = Box->AddChildToHorizontalBox(EmptyText))
		{
			EmptySlot->SetPadding(FMargin(8.f, 12.f));
		}
		return;
	}

	for (const FInventoryItemUI& Item : Items)
	{
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Card->SetBrushColor(Item.mRarityColor == FLinearColor::White
			? InventoryNavyCardColor
			: FLinearColor(
				Item.mRarityColor.R * 0.20f,
				Item.mRarityColor.G * 0.20f,
				Item.mRarityColor.B * 0.20f,
				0.82f));
		Card->SetPadding(FMargin(12.f, 10.f));

		UHorizontalBox* CardContent = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass());
		Card->SetContent(CardContent);

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(82.f);
		IconSize->SetHeightOverride(82.f);
		CardContent->AddChildToHorizontalBox(IconSize);

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (UTexture2D* Texture = ResolveInventoryIcon(Item))
		{
			Icon->SetBrushFromTexture(Texture, false);
		}
		IconSize->AddChild(Icon);

		UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* CopySlot = CardContent->AddChildToHorizontalBox(Copy))
		{
			CopySlot->SetPadding(FMargin(8.f, 1.f, 8.f, 0.f));
			CopySlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Name->SetText(Item.mName);
		SetTextStyle(Name, 20, InventoryCreamColor);
		Copy->AddChildToVerticalBox(Name);

		if (Item.mDetailText.IsEmpty() == false)
		{
			UTextBlock* Detail = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Detail->SetText(Item.mDetailText);
			SetTextStyle(Detail, 15, InventoryMutedTextColor);
			Copy->AddChildToVerticalBox(Detail);
		}

		if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Card))
		{
			BoxSlot->SetPadding(FMargin(4.f, 2.f));
		}
	}
}

/** @brief 용병마다 별도 EXP 진행도를 한 행으로 그린다. */
void UInventoryUIWidgetBase::FillMercenaries(
	UVerticalBox* Box,
	const TArray<FInventoryMercenaryUI>& Mercenaries)
{
	if (Box == nullptr)
	{
		return;
	}
	Box->ClearChildren();

	for (const FInventoryMercenaryUI& Mercenary : Mercenaries)
	{
		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		RowSize->SetHeightOverride(132.f);
		if (UVerticalBoxSlot* RowSizeSlot = Box->AddChildToVerticalBox(RowSize))
		{
			RowSizeSlot->SetPadding(FMargin(0.f, 4.f));
		}

		UBorder* Row = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Row->SetBrushColor(InventoryParchmentCardColor);
		Row->SetPadding(FMargin(10.f, 8.f));
		RowSize->AddChild(Row);

		UHorizontalBox* RowContent = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass());
		Row->SetContent(RowContent);

		USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PortraitSize->SetWidthOverride(74.f);
		PortraitSize->SetHeightOverride(94.f);
		RowContent->AddChildToHorizontalBox(PortraitSize);

		UImage* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (Mercenary.mPortrait != nullptr)
		{
			Portrait->SetBrushFromTexture(Mercenary.mPortrait, false);
		}
		PortraitSize->AddChild(Portrait);

		UVerticalBox* Status = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* StatusSlot = RowContent->AddChildToHorizontalBox(Status))
		{
			SetHorizontalFill(StatusSlot);
			StatusSlot->SetPadding(FMargin(10.f, 0.f, 14.f, 0.f));
			StatusSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UHorizontalBox* NameLine = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* NameLineSlot = Status->AddChildToVerticalBox(NameLine))
		{
			NameLineSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Name->SetText(Mercenary.mName);
		SetTextStyle(Name, 15, InventoryNavyTextColor);
		SetHorizontalFill(NameLine->AddChildToHorizontalBox(Name));

		UTextBlock* Level = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Level->SetText(FText::Format(LOCTEXT("MercenaryLevel", "Lv.{0}"),
			FText::AsNumber(Mercenary.mLevel)));
		SetTextStyle(Level, 13, InventoryNavyTextColor);
		NameLine->AddChildToHorizontalBox(Level);

		USizeBox* ExpBarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ExpBarSize->SetHeightOverride(14.f);
		if (UVerticalBoxSlot* ExpBarSlot = Status->AddChildToVerticalBox(ExpBarSize))
		{
			ExpBarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		}

		UProgressBar* ExpBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		ExpBar->SetPercent(Mercenary.mMaxExp > 0.f
			? FMath::Clamp(Mercenary.mExp / Mercenary.mMaxExp, 0.f, 1.f)
			: 0.f);
		ExpBar->SetFillColorAndOpacity(InventoryExpColor);
		ExpBarSize->AddChild(ExpBar);

		UTextBlock* ExpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ExpText->SetText(Mercenary.mMaxExp > 0.f
			? FText::Format(
				LOCTEXT("MercenaryExpWithMax", "EXP {0} / {1}"),
				FText::AsNumber(FMath::RoundToInt(Mercenary.mExp)),
				FText::AsNumber(FMath::RoundToInt(Mercenary.mMaxExp)))
			: FText::Format(
				LOCTEXT("MercenaryExp", "EXP {0}"),
				FText::AsNumber(FMath::RoundToInt(Mercenary.mExp))));
		SetTextStyle(ExpText, 14, InventoryParchmentMutedColor);
		if (UVerticalBoxSlot* ExpTextSlot = Status->AddChildToVerticalBox(ExpText))
		{
			ExpTextSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
		}

		UTextBlock* HPText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HPText->SetText(FText::Format(
			LOCTEXT("MercenaryHP", "HP {0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Mercenary.mHP)),
			FText::AsNumber(FMath::RoundToInt(Mercenary.mMaxHP))));
		SetTextStyle(HPText, 13, InventoryParchmentMutedColor);
		Status->AddChildToVerticalBox(HPText);
	}

	if (Mercenaries.IsEmpty())
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(LOCTEXT("NoMercenaries", "No mercenaries in the party"));
		SetTextStyle(EmptyText, 17, InventoryNavyTextColor);
		Box->AddChildToVerticalBox(EmptyText);
	}
}

/** @brief 현재 모델의 골드/용병별 EXP/획득 보상을 BindWidget 또는 fallback에 반영한다. */
void UInventoryUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FInventoryUI& Inventory = mUIModel->GetInventory();
	if (mMetaText != nullptr)
	{
		mMetaText->SetText(FText::Format(
			LOCTEXT("GoldValue", "GOLD  {0}"),
			FText::AsNumber(Inventory.mGold)));
	}

	FillMercenaries(mPartyBox, Inventory.mMercenaries);
	FillSection(mSkillBox, Inventory.mSkills);
	FillSection(mEquipBox, Inventory.mEquipment);
	FillSection(mArtifactBox, Inventory.mArtifacts);
}

void UInventoryUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
