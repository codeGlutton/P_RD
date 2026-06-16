// 이 파일: 선택 카드가 바뀔 때 카드들이 "현재 배치 → 목표 배치"로 부드럽게 움직이는
//          프레임별 보간(애니메이션)을 담당한다. 목표 배치 계산은 정책(FCarouselLayoutPolicy),
//          실제 위젯에 값을 쓰는 건 ApplyCarouselLayouts가 맡는다.
#include "UI/CarouselPanelWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "UI/CarouselLayoutPolicy.h"

// 매 프레임: 진행 중인 전환이 있으면 경과 시간으로 0~1 진행도를 구해 카드 배치를 보간 적용.
void UCarouselPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 전환 중이 아니면 매 프레임 일 없음(정적 배치는 Set/Clear 때만 갱신).
	if (mCarouselTransitionActive == false)
	{
		return;
	}

	// 경과/전체 길이 = 선형 진행도(RawAlpha). 여기에 EaseOutCubic을 먹여
	// 처음 빠르게 → 끝에 천천히 멈추는 감속 곡선(Alpha)을 만든다.
	mCarouselTransitionElapsed += InDeltaTime;
	const float RawAlpha = mCarouselTransitionDuration > 0.0f
		? FMath::Clamp(mCarouselTransitionElapsed / mCarouselTransitionDuration, 0.0f, 1.0f)
		: 1.0f;
	const float Alpha = FCarouselLayoutPolicy::EaseOutCubic(RawAlpha);

	TArray<FCarouselItemLayout> CurrentLayouts;
	CurrentLayouts.SetNum(mCarouselTargetLayouts.Num());
	for (int32 Index = 0; Index < mCarouselTargetLayouts.Num(); ++Index)
	{
		const FCarouselItemLayout StartLayout = mCarouselStartLayouts.IsValidIndex(Index)
			? mCarouselStartLayouts[Index]
			: mCarouselTargetLayouts[Index];
		CurrentLayouts[Index] = FCarouselLayoutPolicy::InterpolateLayout(
			StartLayout,
			mCarouselTargetLayouts[Index],
			Alpha,
			RawAlpha);
	}
	ApplyCarouselLayouts(CurrentLayouts);

	// 진행도 100%면 전환 종료 + 부동소수 누적 오차 없이 목표 배치를 정확히 한 번 더 적용.
	if (RawAlpha >= 1.0f)
	{
		mCarouselTransitionActive = false;
		ApplyCarouselLayouts(mCarouselTargetLayouts);
	}
}

// 현재 선택/활성 상태 기준으로 각 카드의 "목표 배치"를 정책에 위임해 계산.
// 활성(원형) 상태일 때만 카드별 추가 회전각(파생 위젯이 정함)을 같이 넘긴다.
void UCarouselPanelWidget::BuildCarouselLayouts(TArray<FCarouselItemLayout>& OutLayouts) const
{
	const int32 ItemCount = GetClampedCarouselItemDisplayCount();
	TArray<float> ItemAngles;
	if (mCarouselActivated == true)
	{
		ItemAngles.SetNum(ItemCount);
		for (int32 Index = 0; Index < ItemCount; ++Index)
		{
			ItemAngles[Index] = GetCarouselItemAngle(Index);
		}
	}

	FCarouselLayoutPolicy::BuildLayouts(
		mCarouselItems.Num(),
		ItemCount,
		mCarouselActivated,
		mSelectedCarouselIndex,
		ItemAngles,
		OutLayouts);
}

// 지금 화면에 적용돼 있는 카드들의 실제 transform(위치/스케일/각도/투명도/ZOrder)을 읽어
// 애니메이션 "시작 배치"로 쓴다. 전환 도중 다시 선택해도 끊김 없이 현재 위치에서 이어지게 함.
void UCarouselPanelWidget::CaptureCurrentCarouselLayouts(TArray<FCarouselItemLayout>& OutLayouts) const
{
	OutLayouts.Reset();
	OutLayouts.SetNum(mCarouselItems.Num());

	for (int32 Index = 0; Index < mCarouselItems.Num(); ++Index)
	{
		const UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr)
		{
			continue;
		}

		FCarouselItemLayout& Layout = OutLayouts[Index];
		Layout.mVisible = Item->GetVisibility() != ESlateVisibility::Collapsed;
		Layout.mScale = Item->GetRenderTransform().Scale;
		Layout.mAngle = Item->GetRenderTransform().Angle;
		Layout.mOpacity = Item->GetRenderOpacity();
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
		{
			Layout.mPosition = CanvasSlot->GetPosition();
			Layout.mSize = CanvasSlot->GetSize();
			Layout.mZOrder = CanvasSlot->GetZOrder();
		}
	}
}

// 계산된 배치 값들을 실제 카드 위젯에 기록한다.
// - 보이는 카드: 렌더 transform(중심 피벗 기준 스케일/회전/투명도) + CanvasSlot(위치/크기/ZOrder) 설정
// - 안 보이는 카드: Collapsed로 숨김(자리도 차지 안 함)
void UCarouselPanelWidget::ApplyCarouselLayouts(const TArray<FCarouselItemLayout>& Layouts)
{
	for (int32 Index = 0; Index < mCarouselItems.Num(); ++Index)
	{
		UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr)
		{
			continue;
		}

		const FCarouselItemLayout Layout = Layouts.IsValidIndex(Index) ? Layouts[Index] : FCarouselItemLayout();
		if (Layout.mVisible == false)
		{
			Item->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		Item->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Item->SetRenderScale(Layout.mScale);
		Item->SetRenderTransformAngle(Layout.mAngle);
		Item->SetRenderOpacity(Layout.mOpacity);
		Item->SetVisibility(ESlateVisibility::Visible);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(Layout.mPosition);
			CanvasSlot->SetSize(Layout.mSize);
			CanvasSlot->SetZOrder(Layout.mZOrder);
		}
	}
}

// 전환 시작: 지금 배치를 시작점으로 캡처 + 목표 배치 저장 + 타이머 리셋해 NativeTick이 보간하게 함.
void UCarouselPanelWidget::StartCarouselLayoutTransition(const TArray<FCarouselItemLayout>& TargetLayouts)
{
	CaptureCurrentCarouselLayouts(mCarouselStartLayouts);
	mCarouselTargetLayouts = TargetLayouts;
	mCarouselTransitionElapsed = 0.0f;
	mCarouselTransitionActive = true;
}
