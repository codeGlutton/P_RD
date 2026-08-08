#include "UI/Inventory/InventoryUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
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
	constexpr float ArtifactGridWrapWidth = 1420.f;
	constexpr float ArtifactCardWidth = 250.f;
	constexpr float ArtifactCardHeight = 178.f;

	const FLinearColor InventoryBackgroundColor(0.008f, 0.016f, 0.027f, 1.f);
	const FLinearColor InventoryGoldColor(1.0f, 0.79f, 0.25f, 1.f);
	const FLinearColor InventoryCreamColor(0.98f, 0.92f, 0.79f, 1.f);
	const FLinearColor InventoryMutedTextColor(0.72f, 0.80f, 0.84f, 1.f);
	const FLinearColor InventoryNavyTextColor(0.035f, 0.090f, 0.16f, 1.f);

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

	FInventoryArtifactUI MakeInventoryArtifact(const FInventoryArtifactView& Source)
	{
		FInventoryArtifactUI Artifact;
		Artifact.mArtifactIndex = Source.mArtifactIndex;
		Artifact.mName = Source.mName;
		Artifact.mIcon = Source.mIcon;
		Artifact.mRarityColor = Source.mRarityColor;
		Artifact.mDetailText = Source.mDetail;
		return Artifact;
	}
}

UInventoryUIWidgetBase::UInventoryUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

	static ConstructorHelpers::FObjectFinder<UTexture2D> BackgroundFinder(
		TEXT("/Game/UI/Art/RunFlow/T_Inventory_Background_Current.T_Inventory_Background_Current"));
	if (BackgroundFinder.Succeeded())
	{
		mDefaultBackgroundArt = BackgroundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> ArtifactIconFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Treasure.T_MapNode_Treasure"));
	if (ArtifactIconFinder.Succeeded())
	{
		mDefaultArtifactIcon = ArtifactIconFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> CardFrameFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal.T_KitA_Cell_Normal"));
	if (CardFrameFinder.Succeeded())
	{
		mArtifactCardFrame = CardFrameFinder.Object;
	}
}

void UInventoryUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureFallbackLayout();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(
			this, &UInventoryUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(LOCTEXT("Close", "CLOSE"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(LOCTEXT("Inventory", "SHARED INVENTORY"));
	}
	if (mArtifactLabel != nullptr)
	{
		mArtifactLabel->SetText(LOCTEXT("Artifacts", "ARTIFACTS"));
	}

	RefreshView();
}

void UInventoryUIWidgetBase::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	RefreshFromCurrentRoom();
}

void UInventoryUIWidgetBase::ApplyCloseUI()
{
	// 개발 프리뷰/자동화가 주입한 외부 모델을 닫힐 때 버린다. 다음 정상 열기는
	// 반드시 현재 룸의 공용 골드와 아티팩트를 다시 읽는다.
	if (mUIModel != nullptr && mUIModel != mRuntimeUIModel)
	{
		UnbindUIModel();
	}
	Super::ApplyCloseUI();
}

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
		mUIModel->OnUIChanged.AddDynamic(
			this, &UInventoryUIWidgetBase::HandleUIChanged);
		RefreshView();
		OnInventoryRefreshed();
	}
}

void UInventoryUIWidgetBase::HandleCloseClicked()
{
	CloseUI();
}

int32 UInventoryUIWidgetBase::GetDisplayedGold() const
{
	return mUIModel != nullptr ? mUIModel->GetInventory().mGold : 0;
}

int32 UInventoryUIWidgetBase::GetArtifactCount() const
{
	return mUIModel != nullptr ? mUIModel->GetInventory().mArtifacts.Num() : 0;
}

bool UInventoryUIWidgetBase::HasBackgroundArt() const
{
	return mBackgroundArt.IsNull() == false || mDefaultBackgroundArt != nullptr;
}

bool UInventoryUIWidgetBase::HasFallbackArtifactIcon() const
{
	return mDefaultArtifactIcon != nullptr;
}

bool UInventoryUIWidgetBase::HasArtifactCardFrame() const
{
	return mArtifactCardFrame != nullptr;
}

UTexture2D* UInventoryUIWidgetBase::ResolveArtifactIcon(
	const FInventoryArtifactUI& Artifact) const
{
	return Artifact.mIcon != nullptr ? Artifact.mIcon.Get() : mDefaultArtifactIcon.Get();
}

void UInventoryUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(
			this, &UInventoryUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

void UInventoryUIWidgetBase::HandleUIChanged()
{
	RefreshView();
	OnInventoryRefreshed();
}

void UInventoryUIWidgetBase::RefreshFromCurrentRoom()
{
	// 외부에서 mock/test 모델을 바인딩한 프리뷰는 실제 룸 값으로 덮지 않는다.
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
		? Cast<ARoomGameModeBase>(World->GetAuthGameMode())
		: nullptr;

	FInventoryView View;
	if (RoomGameMode == nullptr || RoomGameMode->GetInventoryView(View) == false)
	{
		mRuntimeUIModel->SetInventory(Inventory);
		return;
	}

	Inventory.mGold = View.mGold;
	Inventory.mArtifacts.Reserve(View.mArtifacts.Num());
	for (const FInventoryArtifactView& Source : View.mArtifacts)
	{
		Inventory.mArtifacts.Add(MakeInventoryArtifact(Source));
	}

	mRuntimeUIModel->SetInventory(Inventory);
}

void UInventoryUIWidgetBase::EnsureFallbackLayout()
{
	if (mFallbackLayoutRoot != nullptr
		|| (mMetaText != nullptr
			&& mArtifactLabel != nullptr
			&& mArtifactBox != nullptr
			&& mCloseButton != nullptr))
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree != nullptr
		? Cast<UCanvasPanel>(WidgetTree->RootWidget)
		: nullptr;
	if (RootCanvas == nullptr)
	{
		return;
	}

	UBorder* ScreenBackdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("InventoryScreenBackdrop"));
	ScreenBackdrop->SetBrushColor(InventoryBackgroundColor);
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(ScreenBackdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackdropSlot->SetOffsets(FMargin(0.f));
		BackdropSlot->SetZOrder(40);
	}

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
		FVector2D(315.f, 54.f), FVector2D(880.f, 72.f), 2);

	mMetaText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimeGold"));
	SetTextStyle(mMetaText, 30, InventoryGoldColor);
	mMetaText->SetJustification(ETextJustify::Right);
	PlaceOnDesignCanvas(
		DesignCanvas, mMetaText,
		FVector2D(1372.f, 72.f), FVector2D(145.f, 55.f), 2);

	mArtifactLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimeArtifactLabel"));
	SetTextStyle(mArtifactLabel, 26, InventoryCreamColor);
	PlaceOnDesignCanvas(
		DesignCanvas, mArtifactLabel,
		FVector2D(125.f, 218.f), FVector2D(500.f, 45.f), 2);

	USizeBox* ScrollFrame = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("InventoryRuntimeArtifactScrollFrame"));
	ScrollFrame->SetWidthOverride(1440.f);
	ScrollFrame->SetHeightOverride(492.f);
	PlaceOnDesignCanvas(
		DesignCanvas, ScrollFrame,
		FVector2D(116.f, 270.f), FVector2D(1440.f, 492.f), 2);

	UScrollBox* ArtifactScroll = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(), TEXT("InventoryRuntimeArtifactScroll"));
	ArtifactScroll->SetOrientation(Orient_Vertical);
	ScrollFrame->AddChild(ArtifactScroll);

	mArtifactBox = WidgetTree->ConstructWidget<UWrapBox>(
		UWrapBox::StaticClass(), TEXT("InventoryRuntimeArtifactGrid"));
	mArtifactBox->SetOrientation(Orient_Horizontal);
	mArtifactBox->SetExplicitWrapSize(true);
	mArtifactBox->SetWrapSize(ArtifactGridWrapWidth);
	mArtifactBox->SetInnerSlotPadding(FVector2D(17.f, 14.f));
	mArtifactBox->SetHorizontalAlignment(HAlign_Center);
	ArtifactScroll->AddChild(mArtifactBox);

	mCloseButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), TEXT("InventoryRuntimeCloseButton"));
	mCloseButton->SetBackgroundColor(FLinearColor::Transparent);
	mCloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("InventoryRuntimeCloseText"));
	SetTextStyle(mCloseButtonText, 24, InventoryCreamColor);
	mCloseButtonText->SetJustification(ETextJustify::Center);
	mCloseButton->AddChild(mCloseButtonText);
	PlaceOnDesignCanvas(
		DesignCanvas, mCloseButton,
		FVector2D(1265.f, 778.f), FVector2D(300.f, 103.f), 3);
}

void UInventoryUIWidgetBase::FillArtifacts(
	UWrapBox* Box,
	const TArray<FInventoryArtifactUI>& Artifacts)
{
	if (Box == nullptr)
	{
		return;
	}
	Box->ClearChildren();

	if (Artifacts.IsEmpty())
	{
		USizeBox* EmptySize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("InventoryRuntimeEmptySize"));
		EmptySize->SetWidthOverride(ArtifactGridWrapWidth);
		EmptySize->SetHeightOverride(150.f);

		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("InventoryRuntimeEmptyText"));
		EmptyText->SetText(LOCTEXT("NoArtifacts", "No artifacts acquired"));
		SetTextStyle(EmptyText, 22, InventoryMutedTextColor);
		EmptyText->SetJustification(ETextJustify::Center);
		EmptySize->AddChild(EmptyText);

		if (UWrapBoxSlot* EmptySlot = Box->AddChildToWrapBox(EmptySize))
		{
			EmptySlot->SetFillEmptySpace(true);
			EmptySlot->SetHorizontalAlignment(HAlign_Fill);
		}
		return;
	}

	for (int32 ArtifactPosition = 0; ArtifactPosition < Artifacts.Num(); ++ArtifactPosition)
	{
		const FInventoryArtifactUI& Artifact = Artifacts[ArtifactPosition];

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("InventoryArtifactCard_%d"), ArtifactPosition));
		CardSize->SetWidthOverride(ArtifactCardWidth);
		CardSize->SetHeightOverride(ArtifactCardHeight);

		UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			*FString::Printf(TEXT("InventoryArtifactOverlay_%d"), ArtifactPosition));
		CardSize->AddChild(Card);

		UImage* Frame = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("InventoryArtifactFrame_%d"), ArtifactPosition));
		if (mArtifactCardFrame != nullptr)
		{
			Frame->SetBrushFromTexture(mArtifactCardFrame, false);
		}
		const FLinearColor RarityTint = Artifact.mRarityColor == FLinearColor::White
			? FLinearColor::White
			: FLinearColor(
				0.72f + Artifact.mRarityColor.R * 0.28f,
				0.72f + Artifact.mRarityColor.G * 0.28f,
				0.72f + Artifact.mRarityColor.B * 0.28f,
				1.f);
		Frame->SetColorAndOpacity(RarityTint);
		if (UOverlaySlot* FrameSlot = Card->AddChildToOverlay(Frame))
		{
			FrameSlot->SetHorizontalAlignment(HAlign_Fill);
			FrameSlot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass());
		IconSize->SetWidthOverride(92.f);
		IconSize->SetHeightOverride(92.f);
		if (UOverlaySlot* IconSizeSlot = Card->AddChildToOverlay(IconSize))
		{
			IconSizeSlot->SetHorizontalAlignment(HAlign_Center);
			IconSizeSlot->SetVerticalAlignment(VAlign_Top);
			IconSizeSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
		}

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass());
		if (UTexture2D* Texture = ResolveArtifactIcon(Artifact))
		{
			Icon->SetBrushFromTexture(Texture, false);
		}
		IconSize->AddChild(Icon);

		UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass());
		if (UOverlaySlot* CopySlot = Card->AddChildToOverlay(Copy))
		{
			CopySlot->SetHorizontalAlignment(HAlign_Fill);
			CopySlot->SetVerticalAlignment(VAlign_Bottom);
			CopySlot->SetPadding(FMargin(20.f, 0.f, 20.f, 18.f));
		}

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass());
		Name->SetText(Artifact.mName);
		SetTextStyle(Name, 18, InventoryCreamColor);
		Name->SetJustification(ETextJustify::Center);
		Copy->AddChildToVerticalBox(Name);

		if (Artifact.mDetailText.IsEmpty() == false)
		{
			UTextBlock* Detail = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass());
			Detail->SetText(Artifact.mDetailText);
			SetTextStyle(Detail, 13, InventoryMutedTextColor);
			Detail->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* DetailSlot = Copy->AddChildToVerticalBox(Detail))
			{
				DetailSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
			}
		}

		if (UWrapBoxSlot* CardSlot = Box->AddChildToWrapBox(CardSize))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Center);
			CardSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UInventoryUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FInventoryUI& Inventory = mUIModel->GetInventory();
	if (mMetaText != nullptr)
	{
		mMetaText->SetText(FText::AsNumber(Inventory.mGold));
	}
	FillArtifacts(mArtifactBox, Inventory.mArtifacts);
}

void UInventoryUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
