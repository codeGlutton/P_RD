#include "UI/CarouselPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "UI/ViewportZOrderType.h"

namespace
{
	constexpr int32 MaxCarouselItemCount = 8;
	const FVector2D CarouselOrigin(0.0f, 36.0f);
	const FVector2D CarouselBaseSize(180.0f, 244.0f);
	const FVector2D LinearListOrigin(0.0f, 36.0f);
	constexpr float CarouselRadiusX = 360.0f;
	constexpr float CarouselRadiusY = 58.0f;
	constexpr float LinearListMaxWidth = 920.0f;
	constexpr float LinearListPreferredSpacing = 168.0f;
	constexpr float LinearListScale = 0.74f;
	constexpr float MinCarouselScale = 0.44f;
	constexpr float MaxCarouselScale = 1.18f;
	constexpr float MinCarouselOpacity = 0.30f;
	constexpr float MaxCarouselOpacity = 1.00f;
	constexpr float CarouselTapDistanceThreshold = 28.0f;

	UWidget* FindWidgetByName(const UWidgetTree* WidgetTree, const FString& WidgetName)
	{
		return WidgetTree != nullptr ? WidgetTree->FindWidget(FName(*WidgetName)) : nullptr;
	}
}

UCarouselPanelWidget::UCarouselPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp) + 2;
}

void UCarouselPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheCarouselWidgets();
	BindCarouselEvents();
	ApplyCarouselLayout();
}

void UCarouselPanelWidget::NativeDestruct()
{
	UnbindCarouselEvents();

	Super::NativeDestruct();
}

void UCarouselPanelWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	ResetCarouselState();
	ApplyCarouselLayout();
}

FReply UCarouselPanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BeginCarouselPress(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCarouselPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && mPressedCarouselIndex != INDEX_NONE)
	{
		FinishCarouselPress(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCarouselPanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (BeginCarouselPress(InGestureEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

FReply UCarouselPanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mPressedCarouselIndex != INDEX_NONE)
	{
		FinishCarouselPress(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

bool UCarouselPanelWidget::IsCarouselActivated() const
{
	return mCarouselActivated;
}

int32 UCarouselPanelWidget::GetSelectedCarouselIndex() const
{
	return mSelectedCarouselIndex;
}

UWidget* UCarouselPanelWidget::GetSelectedCarouselItem() const
{
	return mCarouselItems.IsValidIndex(mSelectedCarouselIndex) ? mCarouselItems[mSelectedCarouselIndex].Get() : nullptr;
}

bool UCarouselPanelWidget::IsPointerOverSelectedCarouselItem(const FVector2D& ScreenPosition) const
{
	const UWidget* SelectedItem = GetSelectedCarouselItem();
	return SelectedItem != nullptr && SelectedItem->GetCachedGeometry().IsUnderLocation(ScreenPosition);
}

float UCarouselPanelWidget::GetCarouselItemAngle(int32 ItemIndex) const
{
	return 0.0f;
}

void UCarouselPanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
}

void UCarouselPanelWidget::HandleCloseButtonClicked()
{
	CloseUI();
}

void UCarouselPanelWidget::HandleCarouselButton0Clicked()
{
	SelectCarouselItem(0);
}

void UCarouselPanelWidget::HandleCarouselButton1Clicked()
{
	SelectCarouselItem(1);
}

void UCarouselPanelWidget::HandleCarouselButton2Clicked()
{
	SelectCarouselItem(2);
}

void UCarouselPanelWidget::HandleCarouselButton3Clicked()
{
	SelectCarouselItem(3);
}

void UCarouselPanelWidget::HandleCarouselButton4Clicked()
{
	SelectCarouselItem(4);
}

void UCarouselPanelWidget::HandleCarouselButton5Clicked()
{
	SelectCarouselItem(5);
}

void UCarouselPanelWidget::HandleCarouselButton6Clicked()
{
	SelectCarouselItem(6);
}

void UCarouselPanelWidget::HandleCarouselButton7Clicked()
{
	SelectCarouselItem(7);
}

void UCarouselPanelWidget::CacheCarouselWidgets()
{
	mCarouselItems.Reset();
	mCarouselButtons.Reset();

	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCloseButtonClicked);
	}
	if (CloseButtonText != nullptr)
	{
		CloseButtonText->SetText(NSLOCTEXT("CarouselPanelWidget", "CloseButtonText", "X"));
	}

	for (int32 Index = 0; Index < MaxCarouselItemCount; ++Index)
	{
		const FString ItemName = FString::Printf(TEXT("CarouselItem_%d"), Index);
		UWidget* Item = FindWidgetByName(WidgetTree, ItemName);
		const FString ButtonName = FString::Printf(TEXT("CarouselButton_%d"), Index);
		UButton* Button = Cast<UButton>(FindWidgetByName(WidgetTree, ButtonName));
		if (Item == nullptr)
		{
			Item = Button;
		}
		if (Item == nullptr)
		{
			continue;
		}

		if (Button != nullptr)
		{
			mCarouselButtons.Add(Button);
		}
		mCarouselItems.Add(Item);
	}

	mSelectedCarouselIndex = mCarouselItems.IsEmpty() ? INDEX_NONE : FMath::Clamp(mSelectedCarouselIndex, 0, mCarouselItems.Num() - 1);
}

void UCarouselPanelWidget::BindCarouselEvents()
{
	for (int32 Index = 0; Index < mCarouselButtons.Num(); ++Index)
	{
		BindCarouselButton(mCarouselButtons[Index], Index);
	}
}

void UCarouselPanelWidget::UnbindCarouselEvents()
{
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCloseButtonClicked);
	}

	for (int32 Index = 0; Index < mCarouselButtons.Num(); ++Index)
	{
		UnbindCarouselButton(mCarouselButtons[Index], Index);
	}
}

void UCarouselPanelWidget::SelectCarouselItem(int32 ItemIndex)
{
	if (!mCarouselItems.IsValidIndex(ItemIndex))
	{
		return;
	}

	const int32 PreviousIndex = mSelectedCarouselIndex;
	const bool bWasCarouselActivated = mCarouselActivated;
	mCarouselActivated = true;
	mSelectedCarouselIndex = ItemIndex;
	if (PreviousIndex != mSelectedCarouselIndex || !bWasCarouselActivated)
	{
		HandleCarouselSelectionChanged(PreviousIndex, mSelectedCarouselIndex);
	}
	ApplyCarouselLayout();
}

void UCarouselPanelWidget::ApplyCarouselLayout()
{
	const int32 ItemCount = mCarouselItems.Num();
	if (ItemCount <= 0 || mSelectedCarouselIndex == INDEX_NONE)
	{
		return;
	}

	if (!mCarouselActivated)
	{
		ApplyLinearListLayout();
		return;
	}

	for (int32 Index = 0; Index < ItemCount; ++Index)
	{
		UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr)
		{
			continue;
		}

		const int32 RelativeIndex = (Index - mSelectedCarouselIndex + ItemCount) % ItemCount;
		const float Angle = 2.0f * PI * StaticCast<float>(RelativeIndex) / StaticCast<float>(ItemCount);
		const float Depth = FMath::Cos(Angle);
		const float NormalizedDepth = (Depth + 1.0f) * 0.5f;
		const float X = FMath::Sin(Angle) * CarouselRadiusX;
		const float Y = Depth * CarouselRadiusY;
		const float Scale = FMath::Lerp(MinCarouselScale, MaxCarouselScale, NormalizedDepth);
		const float Opacity = FMath::Lerp(MinCarouselOpacity, MaxCarouselOpacity, NormalizedDepth);
		const int32 ZOrder = FMath::RoundToInt(100.0f + Depth * 100.0f);

		Item->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Item->SetRenderScale(FVector2D(Scale, Scale));
		Item->SetRenderTransformAngle(GetCarouselItemAngle(Index));
		Item->SetRenderOpacity(Opacity);
		Item->SetVisibility(ESlateVisibility::Visible);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(CarouselOrigin + FVector2D(X, Y));
			CanvasSlot->SetSize(CarouselBaseSize);
			CanvasSlot->SetZOrder(ZOrder);
		}
	}
}

void UCarouselPanelWidget::ApplyLinearListLayout()
{
	const int32 ItemCount = mCarouselItems.Num();
	if (ItemCount <= 0)
	{
		return;
	}

	const float Spacing = ItemCount > 1
		? FMath::Min(LinearListPreferredSpacing, LinearListMaxWidth / StaticCast<float>(ItemCount - 1))
		: 0.0f;
	const float StartX = -Spacing * StaticCast<float>(ItemCount - 1) * 0.5f;

	for (int32 Index = 0; Index < ItemCount; ++Index)
	{
		UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr)
		{
			continue;
		}

		Item->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Item->SetRenderScale(FVector2D(LinearListScale, LinearListScale));
		Item->SetRenderTransformAngle(0.0f);
		Item->SetRenderOpacity(1.0f);
		Item->SetVisibility(ESlateVisibility::Visible);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(LinearListOrigin + FVector2D(StartX + Spacing * Index, 0.0f));
			CanvasSlot->SetSize(CarouselBaseSize);
			CanvasSlot->SetZOrder(100 + Index);
		}
	}
}

void UCarouselPanelWidget::ResetCarouselState()
{
	mPressedCarouselIndex = INDEX_NONE;
	mCarouselPressPosition = FVector2D::ZeroVector;
	mCarouselActivated = false;
	mSelectedCarouselIndex = mCarouselItems.IsEmpty() ? INDEX_NONE : 0;
}

bool UCarouselPanelWidget::BeginCarouselPress(const FVector2D& ScreenPosition)
{
	const int32 ItemIndex = FindCarouselItemIndexAtPosition(ScreenPosition);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	mPressedCarouselIndex = ItemIndex;
	mCarouselPressPosition = ScreenPosition;
	return true;
}

bool UCarouselPanelWidget::FinishCarouselPress(const FVector2D& ScreenPosition)
{
	const int32 ReleasedItemIndex = FindCarouselItemIndexAtPosition(ScreenPosition);
	const bool bIsTap = mPressedCarouselIndex != INDEX_NONE
		&& ReleasedItemIndex == mPressedCarouselIndex
		&& FVector2D::Distance(mCarouselPressPosition, ScreenPosition) <= CarouselTapDistanceThreshold;

	if (bIsTap)
	{
		SelectCarouselItem(mPressedCarouselIndex);
	}

	mPressedCarouselIndex = INDEX_NONE;
	mCarouselPressPosition = FVector2D::ZeroVector;
	return bIsTap;
}

int32 UCarouselPanelWidget::FindCarouselItemIndexAtPosition(const FVector2D& ScreenPosition) const
{
	int32 BestItemIndex = INDEX_NONE;
	int32 BestZOrder = TNumericLimits<int32>::Lowest();

	for (int32 Index = 0; Index < mCarouselItems.Num(); ++Index)
	{
		const UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr || !Item->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			continue;
		}

		const int32 ZOrder = GetCarouselItemZOrder(Index);
		if (ZOrder >= BestZOrder)
		{
			BestZOrder = ZOrder;
			BestItemIndex = Index;
		}
	}

	return BestItemIndex;
}

int32 UCarouselPanelWidget::GetCarouselItemZOrder(int32 ItemIndex) const
{
	const UWidget* Item = mCarouselItems.IsValidIndex(ItemIndex) ? mCarouselItems[ItemIndex].Get() : nullptr;
	if (Item == nullptr)
	{
		return TNumericLimits<int32>::Lowest();
	}

	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
	{
		return CanvasSlot->GetZOrder();
	}

	return 0;
}

void UCarouselPanelWidget::BindCarouselButton(UButton* Button, int32 ItemIndex)
{
	if (Button == nullptr)
	{
		return;
	}

	switch (ItemIndex)
	{
	case 0:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton0Clicked);
		break;
	case 1:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton1Clicked);
		break;
	case 2:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton2Clicked);
		break;
	case 3:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton3Clicked);
		break;
	case 4:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton4Clicked);
		break;
	case 5:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton5Clicked);
		break;
	case 6:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton6Clicked);
		break;
	case 7:
		Button->OnClicked.AddUniqueDynamic(this, &UCarouselPanelWidget::HandleCarouselButton7Clicked);
		break;
	default:
		break;
	}
}

void UCarouselPanelWidget::UnbindCarouselButton(UButton* Button, int32 ItemIndex)
{
	if (Button == nullptr)
	{
		return;
	}

	switch (ItemIndex)
	{
	case 0:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton0Clicked);
		break;
	case 1:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton1Clicked);
		break;
	case 2:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton2Clicked);
		break;
	case 3:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton3Clicked);
		break;
	case 4:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton4Clicked);
		break;
	case 5:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton5Clicked);
		break;
	case 6:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton6Clicked);
		break;
	case 7:
		Button->OnClicked.RemoveDynamic(this, &UCarouselPanelWidget::HandleCarouselButton7Clicked);
		break;
	default:
		break;
	}
}
