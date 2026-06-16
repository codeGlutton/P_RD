// 이 파일: 카드 한 장이 화면 어디에·얼마 크기·기울기·투명도로 놓일지를 "계산만" 한다.
//          위젯을 직접 건드리지 않는 순수 계산(상태 없음)이라 테스트·재사용이 쉽다.
//          두 모드: ① 비활성 = 가로 리스트  ② 활성 = 선택 카드 중심의 원형(타원) 캐러셀.
#include "UI/CarouselLayoutPolicy.h"

namespace
{
	const FVector2D CarouselOrigin(0.0f, 36.0f);      // 원형 캐러셀 중심
	const FVector2D CarouselBaseSize(180.0f, 244.0f); // 카드 기본 크기
	const FVector2D LinearListOrigin(0.0f, 36.0f);    // 가로 리스트 중심
	constexpr float CarouselRadiusX = 360.0f;         // 타원 가로 반지름(좌우로 벌어지는 정도)
	constexpr float CarouselRadiusY = 58.0f;          // 타원 세로 반지름(앞/뒤 깊이로 살짝 위아래)
	constexpr float LinearListMaxWidth = 920.0f;      // 리스트 전체 최대 폭(넘으면 간격 좁힘)
	constexpr float LinearListPreferredSpacing = 168.0f; // 리스트 카드 간 기본 간격
	constexpr float LinearListScale = 0.74f;          // 리스트 모드 카드 축소율
	constexpr float MinCarouselScale = 0.44f;         // 가장 뒤 카드 크기
	constexpr float MaxCarouselScale = 1.18f;         // 가장 앞(선택) 카드 크기
	constexpr float MinCarouselOpacity = 0.30f;       // 가장 뒤 카드 투명도
	constexpr float MaxCarouselOpacity = 1.00f;       // 가장 앞 카드 투명도

	// 비활성 모드: 카드들을 가로로 균등 배치. 개수가 많아 폭을 넘기면 간격을 줄여 한 줄에 맞춘다.
	void BuildLinearListLayouts(int32 VisibleItemCount, TArray<FCarouselItemLayout>& OutLayouts)
	{
		const float Spacing = VisibleItemCount > 1
			? FMath::Min(LinearListPreferredSpacing, LinearListMaxWidth / StaticCast<float>(VisibleItemCount - 1))
			: 0.0f;
		const float StartX = -Spacing * StaticCast<float>(VisibleItemCount - 1) * 0.5f;

		for (int32 Index = 0; Index < VisibleItemCount; ++Index)
		{
			FCarouselItemLayout& Layout = OutLayouts[Index];
			Layout.mVisible = true;
			Layout.mPosition = LinearListOrigin + FVector2D(StartX + Spacing * Index, 0.0f);
			Layout.mSize = CarouselBaseSize;
			Layout.mScale = FVector2D(LinearListScale, LinearListScale);
			Layout.mAngle = 0.0f;
			Layout.mOpacity = 1.0f;
			Layout.mZOrder = 100 + Index;
		}
	}

	// 활성 모드: 카드들을 타원 위에 빙 둘러 놓고, 선택 카드를 맨 앞 중앙에 둔다.
	// 핵심 아이디어 — 선택 카드 기준 상대 위치를 각도(Angle)로 환산하고,
	//   그 각도의 cos를 "깊이(Depth)"로 본다(앞=+1, 옆=0, 뒤=-1).
	//   깊이로 크기·투명도·ZOrder를 동시에 정해 "앞 카드는 크고 또렷, 뒤 카드는 작고 흐릿"하게 만든다.
	void BuildActivatedCarouselLayouts(
		int32 VisibleItemCount,
		int32 SelectedItemIndex,
		const TArray<float>& ItemAngles,
		TArray<FCarouselItemLayout>& OutLayouts)
	{
		const int32 SelectedIndex = (SelectedItemIndex < 0 || SelectedItemIndex >= VisibleItemCount) ? 0 : SelectedItemIndex;
		for (int32 Index = 0; Index < VisibleItemCount; ++Index)
		{
			// 선택 카드를 0번(앞)으로 두는 상대 순번 → 0~2π 각도로 균등 분배.
			const int32 RelativeIndex = (Index - SelectedIndex + VisibleItemCount) % VisibleItemCount;
			const float Angle = 2.0f * PI * StaticCast<float>(RelativeIndex) / StaticCast<float>(VisibleItemCount);
			const float Depth = FMath::Cos(Angle);            // 앞(+1)~뒤(-1)
			const float NormalizedDepth = (Depth + 1.0f) * 0.5f; // 0(뒤)~1(앞)로 정규화
			const float X = FMath::Sin(Angle) * CarouselRadiusX; // 좌우 위치(sin)
			const float Y = Depth * CarouselRadiusY;            // 앞일수록 살짝 아래로(타원 깊이)
			const float Scale = FMath::Lerp(MinCarouselScale, MaxCarouselScale, NormalizedDepth);     // 앞일수록 큼
			const float Opacity = FMath::Lerp(MinCarouselOpacity, MaxCarouselOpacity, NormalizedDepth); // 앞일수록 또렷
			const int32 ZOrder = FMath::RoundToInt(100.0f + Depth * 100.0f);                          // 앞일수록 위로 겹침

			FCarouselItemLayout& Layout = OutLayouts[Index];
			Layout.mVisible = true;
			Layout.mPosition = CarouselOrigin + FVector2D(X, Y);
			Layout.mSize = CarouselBaseSize;
			Layout.mScale = FVector2D(Scale, Scale);
			Layout.mAngle = ItemAngles.IsValidIndex(Index) ? ItemAngles[Index] : 0.0f;
			Layout.mOpacity = Opacity;
			Layout.mZOrder = ZOrder;
		}
	}
}

void FCarouselLayoutPolicy::BuildLayouts(
	int32 TotalItemCount,
	int32 VisibleItemCount,
	bool bActivated,
	int32 SelectedItemIndex,
	const TArray<float>& ItemAngles,
	TArray<FCarouselItemLayout>& OutLayouts)
{
	OutLayouts.Reset();
	OutLayouts.SetNum(FMath::Max(0, TotalItemCount));

	const int32 ClampedVisibleItemCount = FMath::Clamp(VisibleItemCount, 0, OutLayouts.Num());
	if (ClampedVisibleItemCount <= 0)
	{
		return;
	}

	if (bActivated == false)
	{
		BuildLinearListLayouts(ClampedVisibleItemCount, OutLayouts);
		return;
	}

	BuildActivatedCarouselLayouts(ClampedVisibleItemCount, SelectedItemIndex, ItemAngles, OutLayouts);
}

float FCarouselLayoutPolicy::EaseOutCubic(float RawAlpha)
{
	const float ClampedAlpha = FMath::Clamp(RawAlpha, 0.0f, 1.0f);
	const float InverseAlpha = 1.0f - ClampedAlpha;
	return 1.0f - InverseAlpha * InverseAlpha * InverseAlpha;
}

FCarouselItemLayout FCarouselLayoutPolicy::InterpolateLayout(
	const FCarouselItemLayout& StartLayout,
	const FCarouselItemLayout& TargetLayout,
	float Alpha,
	float RawAlpha)
{
	FCarouselItemLayout ResolvedStartLayout = StartLayout;
	FCarouselItemLayout ResolvedTargetLayout = TargetLayout;

	if (ResolvedStartLayout.mVisible == false && ResolvedTargetLayout.mVisible == true)
	{
		ResolvedStartLayout = ResolvedTargetLayout;
		ResolvedStartLayout.mOpacity = 0.0f;
	}
	else if (ResolvedStartLayout.mVisible == true && ResolvedTargetLayout.mVisible == false)
	{
		ResolvedTargetLayout = ResolvedStartLayout;
		ResolvedTargetLayout.mOpacity = 0.0f;
	}

	FCarouselItemLayout CurrentLayout;
	CurrentLayout.mVisible = ResolvedStartLayout.mVisible || ResolvedTargetLayout.mVisible;
	CurrentLayout.mPosition = ResolvedStartLayout.mPosition + (ResolvedTargetLayout.mPosition - ResolvedStartLayout.mPosition) * Alpha;
	CurrentLayout.mSize = ResolvedStartLayout.mSize + (ResolvedTargetLayout.mSize - ResolvedStartLayout.mSize) * Alpha;
	CurrentLayout.mScale = ResolvedStartLayout.mScale + (ResolvedTargetLayout.mScale - ResolvedStartLayout.mScale) * Alpha;
	CurrentLayout.mAngle = FMath::Lerp(ResolvedStartLayout.mAngle, ResolvedTargetLayout.mAngle, Alpha);
	CurrentLayout.mOpacity = FMath::Lerp(ResolvedStartLayout.mOpacity, ResolvedTargetLayout.mOpacity, Alpha);
	CurrentLayout.mZOrder = RawAlpha < 0.5f ? ResolvedStartLayout.mZOrder : ResolvedTargetLayout.mZOrder;
	return CurrentLayout;
}
