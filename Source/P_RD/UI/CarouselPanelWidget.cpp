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
		/*
		 * DicePanel/SkillPanel은 WBP 디자인에서 CarouselItem_0 같은 이름으로 카드 슬롯을 만든다.
		 * C++이 특정 WBP 클래스를 직접 알지 않고도 같은 이름 규칙으로 슬롯을 찾아 공통 배치/입력을 적용하기 위한 helper다.
		 */
		return WidgetTree != nullptr ? WidgetTree->FindWidget(FName(*WidgetName)) : nullptr;
	}
}

/**
 * @brief 탑바 팝업보다 살짝 위에 뜨는 캐러셀 패널 기본 ZOrder를 설정한다.
 *
 * @details
 * DicePanel/SkillPanel은 TopMenuBar 버튼으로 열리는 플로팅 패널이다.
 * 탑바는 계속 조작 가능해야 하므로 탑바보다 아래가 아니라 일반 팝업보다 조금 높은 계층에 둔다.
 */
UCarouselPanelWidget::UCarouselPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp) + 2;
}

/**
 * @brief WBP에 배치된 카드/버튼을 찾아 캐시하고 초기 목록 배치를 적용한다.
 *
 * @details
 * Construct 시점에는 WidgetTree가 완성되어 있어 이름 기반 검색이 가능하다.
 * 처음 열렸을 때는 원형 캐러셀이 아니라 가로 목록 상태로 보여주고,
 * 사용자가 하나를 선택하면 캐러셀 상태로 전환한다.
 */
void UCarouselPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheCarouselWidgets();
	BindCarouselEvents();
	ApplyCarouselLayout();
}

/**
 * @brief Construct에서 연결한 버튼 이벤트를 해제한다.
 *
 * @details
 * 팝업 위젯은 열고 닫히는 과정에서 다시 Construct될 수 있다.
 * 이벤트가 남아 있으면 한 번 클릭에 여러 선택 처리가 들어갈 수 있어 Destruct에서 정리한다.
 */
void UCarouselPanelWidget::NativeDestruct()
{
	UnbindCarouselEvents();

	Super::NativeDestruct();
}

/**
 * @brief OpenUI()로 패널이 열릴 때 선택/입력 상태를 초기화하고 목록 배치를 다시 적용한다.
 *
 * @details
 * 이전에 열었을 때 선택했던 카드나 누른 위치가 남아 있으면 새로 연 패널이 이미 선택된 상태처럼 보인다.
 * 패널을 열 때마다 "아직 카드 선택 전" 상태로 돌려, DICE/SKILL 모두 같은 시작 화면을 갖게 한다.
 */
void UCarouselPanelWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	ResetCarouselState();
	ApplyCarouselLayout();
}

/**
 * @brief 에디터/PC 환경의 마우스 누름을 캐러셀 항목 선택 시작으로 처리한다.
 *
 * @details
 * 버튼이 없는 카드 영역도 선택할 수 있게 위젯 Geometry 기준으로 눌린 항목을 찾는다.
 * 눌린 항목이 있으면 마우스를 캡처해 ButtonUp까지 같은 위젯에서 받는다.
 */
FReply UCarouselPanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && BeginCarouselPress(InMouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

/**
 * @brief 마우스 뗌 위치가 처음 누른 카드와 같은지 확인해 탭 선택을 확정한다.
 *
 * @details
 * 누른 뒤 일정 거리 이상 움직이면 드래그/스크롤 의도로 보고 선택하지 않는다.
 * 모바일 터치와 같은 규칙을 쓰기 위해 FinishCarouselPress()로 공통 처리한다.
 */
FReply UCarouselPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && mPressedCarouselIndex != INDEX_NONE)
	{
		FinishCarouselPress(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

/**
 * @brief 모바일 터치 시작을 캐러셀 항목 선택 시작으로 처리한다.
 */
FReply UCarouselPanelWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (BeginCarouselPress(InGestureEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

/**
 * @brief 모바일 터치 종료를 캐러셀 항목 선택 확정으로 처리한다.
 */
FReply UCarouselPanelWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mPressedCarouselIndex != INDEX_NONE)
	{
		FinishCarouselPress(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}

	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

/**
 * @brief 사용자가 카드 하나를 선택해 원형 캐러셀 상태로 들어갔는지 반환한다.
 */
bool UCarouselPanelWidget::IsCarouselActivated() const
{
	return mCarouselActivated;
}

/**
 * @brief 현재 선택된 카드 인덱스를 반환한다.
 */
int32 UCarouselPanelWidget::GetSelectedCarouselIndex() const
{
	return mSelectedCarouselIndex;
}

/**
 * @brief 현재 선택된 카드 위젯을 반환한다.
 */
UWidget* UCarouselPanelWidget::GetSelectedCarouselItem() const
{
	return mCarouselItems.IsValidIndex(mSelectedCarouselIndex) ? mCarouselItems[mSelectedCarouselIndex].Get() : nullptr;
}

/**
 * @brief 포인터 위치가 현재 선택된 카드 위에 있는지 확인한다.
 *
 * @details
 * DicePanel은 선택된 카드 위에서만 회전 드래그를 시작해야 한다.
 * 캐러셀 공통 베이스가 Geometry 판정을 제공해 파생 패널이 카드 배열 구조를 직접 만지지 않게 한다.
 */
bool UCarouselPanelWidget::IsPointerOverSelectedCarouselItem(const FVector2D& ScreenPosition) const
{
	const UWidget* SelectedItem = GetSelectedCarouselItem();
	return SelectedItem != nullptr && SelectedItem->GetCachedGeometry().IsUnderLocation(ScreenPosition);
}

/**
 * @brief 카드별 추가 회전 각도를 반환한다.
 *
 * @details
 * 기본 캐러셀은 회전을 주지 않는다.
 * DicePanel은 선택된 카드만 드래그 각도로 회전시키기 위해 이 함수를 override한다.
 */
float UCarouselPanelWidget::GetCarouselItemAngle(int32 ItemIndex) const
{
	return 0.0f;
}

/**
 * @brief 선택 카드가 바뀐 뒤 파생 패널이 필요한 상태를 정리할 수 있는 hook이다.
 */
void UCarouselPanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
}

/**
 * @brief 닫기 버튼 클릭을 공통 CloseUI() 생명주기로 연결한다.
 */
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
	/*
	 * WBP마다 카드 개수를 코드로 직접 넘기지 않기 위해 이름 규칙으로 최대 8개까지 찾는다.
	 * CarouselItem_N이 있으면 그 위젯을 카드로 사용하고, 없으면 CarouselButton_N 자체를 카드로 사용한다.
	 * 이렇게 해두면 단순 버튼형 WBP와 카드 안에 버튼이 들어간 WBP를 같은 코드로 처리할 수 있다.
	 */
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
	/*
	 * UButton 델리게이트는 파라미터를 직접 받을 수 없으므로,
	 * 인덱스별 작은 핸들러를 통해 SelectCarouselItem(Index)로 연결한다.
	 */
	for (int32 Index = 0; Index < mCarouselButtons.Num(); ++Index)
	{
		BindCarouselButton(mCarouselButtons[Index], Index);
	}
}

void UCarouselPanelWidget::UnbindCarouselEvents()
{
	/*
	 * 캐시된 버튼 수만큼 Construct에서 붙인 핸들러를 해제한다.
	 * WBP가 재생성되어 이전 버튼 인스턴스가 사라지는 상황에서도 UObject 델리게이트 잔류를 최소화한다.
	 */
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
	/*
	 * 첫 선택 전에는 패널이 가로 목록처럼 보인다.
	 * 사용자가 카드를 선택하면 mCarouselActivated를 켜고, 선택된 카드를 기준으로 원형 캐러셀 배치를 계산한다.
	 */
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
	/*
	 * 선택된 카드를 기준으로 각 항목의 상대 위치를 계산한다.
	 * Depth가 클수록 앞쪽 카드로 보고 크기/불투명도/ZOrder를 키운다.
	 * 실제 카드 디자인은 WBP가 담당하고, C++은 카드 묶음의 배치와 깊이감만 만든다.
	 */
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
	/*
	 * 아직 카드를 선택하지 않은 초기 상태 배치다.
	 * 모든 카드가 같은 크기/불투명도로 가로에 놓여, 사용자가 먼저 어떤 항목을 선택할지 고르게 한다.
	 */
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
	/*
	 * 열릴 때마다 선택/눌림 상태를 초기화한다.
	 * 이전 팝업 세션의 Press 좌표가 남으면 첫 터치가 잘못 선택 처리될 수 있다.
	 */
	mPressedCarouselIndex = INDEX_NONE;
	mCarouselPressPosition = FVector2D::ZeroVector;
	mCarouselActivated = false;
	mSelectedCarouselIndex = mCarouselItems.IsEmpty() ? INDEX_NONE : 0;
}

bool UCarouselPanelWidget::BeginCarouselPress(const FVector2D& ScreenPosition)
{
	/*
	 * 눌림 시작 시점에는 선택을 확정하지 않는다.
	 * 사용자가 손가락을 살짝 움직인 뒤 놓는 경우를 스크롤/드래그로 볼 수 있어,
	 * FinishCarouselPress()에서 같은 카드 위에서 짧게 끝났는지 다시 확인한다.
	 */
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
	/*
	 * Press 시작 카드와 Release 카드가 같고 이동 거리가 작을 때만 탭으로 인정한다.
	 * 터치 스크롤이나 주사위 드래그가 단순 선택으로 오인되지 않게 하기 위한 기준이다.
	 */
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
	/*
	 * 캐러셀에서는 카드들이 겹쳐 보일 수 있다.
	 * 화면 좌표 아래에 여러 카드가 겹치면 ZOrder가 가장 높은, 즉 가장 앞에 있는 카드를 선택한다.
	 */
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
	/*
	 * CanvasPanelSlot이 있으면 실제 배치 ZOrder를 사용한다.
	 * WBP 구조가 바뀌어 CanvasPanelSlot이 아니더라도 선택 판정이 완전히 깨지지 않도록 0을 반환한다.
	 */
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
	/*
	 * Dynamic delegate는 일반 람다 인덱스 캡처가 아니라 UFUNCTION 바인딩을 요구한다.
	 * 현재 WBP 최대 카드 수를 8개로 제한하고, 각 인덱스 전용 핸들러를 연결한다.
	 */
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
	/*
	 * BindCarouselButton()에서 붙인 인덱스별 핸들러를 정확히 같은 함수로 해제한다.
	 * AddUniqueDynamic으로 중복을 막아도 Destruct 해제까지 해두면 위젯 재사용 시 이벤트 상태가 예측 가능하다.
	 */
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
