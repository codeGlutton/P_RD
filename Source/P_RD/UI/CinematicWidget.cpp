/**
 * @file CinematicWidget.cpp
 * @brief 인트로/시네마틱 미디어 재생 위젯의 Slate UI 구성과 수명 주기 구현.
 * @author 박용수
 * @date 2026-06-26
 */
// 이 파일: 시네마틱 위젯의 Slate UI 구성(영상 이미지 + 검은 배경 + "로딩중" 대기 레이어)과
//          수명 주기(재생 시작 PlayCinematic / 종료 FinishCinematic / 닫기 애니메이션 진입)를 담당.
#include "UI/CinematicWidget.h"

#include "MediaTexture.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/**
	 * @brief 시네마틱 영상 브러시를 화면 전체에 cover 방식으로 직접 그리는 Slate 리프 위젯.
	 * @details SImage는 할당 영역에 리소스를 그대로 늘려 그리므로 정사각 영상이 4:3에서 찌그러진다.
	 *          이를 피하려고 원본 비율을 유지한 채 화면을 가득 덮는(상하/좌우 크롭) cover rect를
	 *          OnPaint에서 직접 계산해 그린다.
	 */
	class SCinematicCoverImage : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCinematicCoverImage)
			: _Image(nullptr)
		{
		}

			SLATE_ARGUMENT(const FSlateBrush*, Image)        // 그릴 영상 브러시(미디어 텍스처) 포인터
			SLATE_ATTRIBUTE(FVector2D, NativeSize)           // 영상 원본 해상도(비율 계산 기준). 0이면 브러시 ImageSize로 폴백
		SLATE_END_ARGS()

		/**
		 * @brief Slate 인자에서 브러시/원본크기 핸들을 받아 위젯을 초기화한다.
		 * @param InArgs SLATE_ARGUMENT/ATTRIBUTE로 전달된 Image, NativeSize 값.
		 */
		void Construct(const FArguments& InArgs)
		{
			Image = InArgs._Image;
			NativeSize = InArgs._NativeSize;
			SetCanTick(false);                               // 매 프레임 Tick 불필요(정적 그리기) → 성능을 위해 끔
		}

		/**
		 * @brief 부모 레이아웃 영역을 그대로 채우도록 희망 크기를 0으로 보고한다.
		 * @return ZeroVector(상위 Overlay의 Fill 정렬이 실제 크기를 결정).
		 */
		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D::ZeroVector;
		}

		/**
		 * @brief 영상 브러시를 원본 비율 유지 cover 방식으로 할당 영역에 그린다.
		 * @details cover = 화면을 가득 덮도록 가로/세로 스케일 중 큰 값을 채택 → 빈 영역 없이 한 축이 크롭됨.
		 *          넘치는 부분은 ClippingZone으로 잘라낸다.
		 * @param AllottedGeometry 위젯에 배정된 화면 영역(크기/변환).
		 * @param OutDrawElements 그릴 Slate 드로우 엘리먼트 목록(출력).
		 * @param LayerId 현재 페인트 레이어 인덱스.
		 * @param InWidgetStyle 색상/불투명도 틴트.
		 * @param bParentEnabled 부모 활성 여부(비활성 시 Disabled 이펙트 적용).
		 * @return 사용한 마지막 레이어 인덱스.
		 */
		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			// 그릴 브러시가 없거나 그리지 않는 타입이면 아무것도 안 하고 현재 레이어 반환
			if (Image == nullptr || Image->DrawAs == ESlateBrushDrawType::NoDrawType)
			{
				return LayerId;
			}

			const FVector2D ScreenSize = AllottedGeometry.GetLocalSize();   // 위젯이 차지하는 실제 화면 크기
			if (ScreenSize.X <= 0.0f || ScreenSize.Y <= 0.0f)
			{
				return LayerId;                                            // 레이아웃 미확정(크기 0) 프레임은 스킵
			}

			// 비율 계산 기준 = 영상 원본 해상도. 미확보 시 브러시 ImageSize → 최종적으로 1x1로 단계 폴백
			FVector2D VideoSize = NativeSize.Get();
			if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
			{
				VideoSize = Image->ImageSize;
			}
			if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
			{
				VideoSize = FVector2D(1.0f, 1.0f);                        // 0 나눗셈 방지용 안전 폴백
			}

			// cover 스케일: 두 축 비율 중 큰 값을 택해 화면을 빈틈없이 덮음(작은 값을 쓰면 contain=레터박스가 됨)
			const float CoverScale = FMath::Max(ScreenSize.X / VideoSize.X, ScreenSize.Y / VideoSize.Y);
			const FVector2D DrawSize = VideoSize * CoverScale;            // 실제로 그릴 영상 크기(화면보다 한 축 큼)
			const FVector2D DrawOffset = (ScreenSize - DrawSize) * 0.5f;  // 넘치는 양을 절반씩 나눠 중앙 정렬(상하/좌우 균등 크롭)
			const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

			OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));  // 위젯 경계 밖으로 넘친 영상을 잘라냄
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(DrawOffset)),  // 계산한 크기+오프셋으로 페인트 지오메트리 구성
				Image,
				DrawEffects,
				InWidgetStyle.GetColorAndOpacityTint() * Image->GetTint(InWidgetStyle)  // 위젯 틴트와 브러시 틴트를 곱해 최종 색 적용
			);
			OutDrawElements.PopClip();

			return LayerId;
		}

	private:
		const FSlateBrush* Image = nullptr;   // 그릴 영상 브러시(소유하지 않는 약참조 포인터)
		TAttribute<FVector2D> NativeSize;     // 영상 원본 해상도 핸들(매 페인트 시 최신 값을 lazy 조회)
	};
}

/**
 * @brief 시네마틱 위젯 생성자. 닫힐 때 부모에서 자동 제거되도록 설정한다.
 * @param ObjectInitializer 언리얼 오브젝트 초기화 컨텍스트.
 */
UCinematicWidget::UCinematicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mRemoveFromParentOnClose = true;   // 닫기 애니메이션 종료 후 위젯을 뷰포트에서 자동 제거
}

/**
 * @brief Slate 위젯 트리를 구성한다(검은 배경 + cover 영상 이미지 + 로딩 대기 레이어).
 * @details 미디어 오브젝트를 확보하고 영상 원본 해상도를 읽어 브러시를 세팅한 뒤,
 *          3겹 Overlay(맨 아래 검은 배경 / 가운데 영상 / 맨 위 로딩 대기)를 쌓아 반환한다.
 * @return 구성된 루트 SWidget 참조.
 */
TSharedRef<SWidget> UCinematicWidget::RebuildWidget()
{
	EnsureCinematicMediaObjects();   // 미디어 플레이어/텍스처 등 재생에 필요한 오브젝트를 선확보

	// 영상 출력용 브러시를 매 Rebuild마다 초기화하고 이미지 드로우로 설정
	mCinematicVideoBrush = FSlateBrush();
	mCinematicVideoBrush.DrawAs = ESlateBrushDrawType::Image;
	mCinematicVideoNativeSize = ReadCinematicVideoFileDimensions(ResolveCinematicVideoPath());   // 파일 헤더에서 원본 해상도 파싱(비율 계산 기준)
	// 원본 해상도를 못 읽으면 640x640 정사각으로 폴백(브러시 기본 크기)
	mCinematicVideoBrush.ImageSize = mCinematicVideoNativeSize.X > 0.0f && mCinematicVideoNativeSize.Y > 0.0f
		? mCinematicVideoNativeSize
		: FVector2D(640.0f, 640.0f);
	mCinematicVideoBrush.SetResourceObject(mCinematicMediaTexture);   // 루프 재생되는 미디어 텍스처를 브러시 리소스로 연결

	return SNew(SOverlay)
		// [레이어 1] 맨 아래 검은 배경 — 영상이 못 채우는 영역/로딩 전 화면을 검게 덮음
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
		]
		// [레이어 2] 가운데 cover 영상 이미지
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			// 화면 전체에 직접 그리되 원본 비율을 유지한 cover rect를 계산한다.
			// SImage는 할당 영역에 리소스를 그대로 늘려 그릴 수 있어 Android에서 정사각 영상이 4:3으로 찌그러졌다.
			SAssignNew(mCinematicVideoImage, SCinematicCoverImage)
			.Image(&mCinematicVideoBrush)
			.NativeSize(TAttribute<FVector2D>::CreateLambda([this]()
			{
				return mCinematicVideoNativeSize;
			}))
			.Clipping(EWidgetClipping::ClipToBounds)
		]
		// [레이어 3] 맨 위 로딩 대기 레이어 — 페이드 아웃 후 다음 씬 로딩을 가리는 검은 막. 평소엔 Collapsed
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(mLoadingWaitLayer, SBorder)
			.Visibility(EVisibility::Collapsed)   // 초기엔 숨김(FadeToLoadingWaitScreen 시 표시)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
			[
				SNew(SVerticalBox)
				// 상단 여백을 채워 텍스트를 화면 하단 쪽으로 밀어냄
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNullWidget::NullWidget
				]
				// 하단 중앙 "로딩중" 텍스트(아래 96px 패딩으로 바닥에서 살짝 띄움)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 96.0f))
				[
					SAssignNew(mLoadingWaitText, STextBlock)
					.Text(NSLOCTEXT("CinematicWidget", "LoadingWaitText", "로딩중"))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 32))
					.ColorAndOpacity(FLinearColor::Transparent)   // 처음엔 투명 → 페이드 진행도에 맞춰 점점 보이게 함
					.Justification(ETextJustify::Center)
				]
			]
		];
}

/**
 * @brief 위젯 소멸 시 타이머와 미디어 재생을 정리한다.
 * @details 기본 시네마틱 타이머와 페이드 타이머를 모두 해제하고 미디어를 정지해 누수를 방지한다.
 */
void UCinematicWidget::NativeDestruct()
{
	ClearDefaultCinematicTimer();   // 영상 없을 때 쓰는 고정 길이 타이머 해제
	ClearCinematicFadeTimer();      // 진행 중인 페이드 타이머 해제

	StopCinematicMedia();           // 미디어 플레이어 정지(리소스 정리)

	Super::NativeDestruct();
}

/**
 * @brief 시네마틱 재생을 시작한다.
 * @details 종료 콜백을 등록하고 상태/타이머/로딩막을 초기화한 뒤 재생 애니메이션을 트리거한다.
 * @param Callback 시네마틱 종료(FinishCinematic) 시 호출될 델리게이트. 내부로 소유권 이동.
 */
void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	if (IsOpened() == false)
	{
		return;                                  // 위젯이 열려 있지 않으면 재생하지 않음
	}

	OnEndCinematicAnimation = MoveTemp(Callback);  // 종료 콜백 등록(불필요 복사 방지 위해 이동)
	mCinematicFinished = false;                    // 종료 플래그 리셋
	ClearDefaultCinematicTimer();
	ClearCinematicFadeTimer();
	HideLoadingWaitScreen();                       // 이전 재생에서 남은 로딩막 제거
	PlayCinematicAnimation();
}

/**
 * @brief 시네마틱을 1회만 종료 처리하고 등록된 종료 콜백을 실행한다.
 * @details 중복 호출을 막기 위해 mCinematicFinished로 가드하며, 콜백 실행 후 바인딩을 해제한다.
 */
void UCinematicWidget::FinishCinematic()
{
	if (mCinematicFinished)
	{
		return;                          // 이미 종료됐으면 재진입 차단(콜백 중복 실행 방지)
	}

	mCinematicFinished = true;
	ClearDefaultCinematicTimer();

	if (OnEndCinematicAnimation.IsBound())
	{
		OnEndCinematicAnimation.Execute(this);
		OnEndCinematicAnimation.Unbind();   // 1회성 콜백이므로 실행 후 즉시 해제
	}
}

/**
 * @brief 로딩 대기 화면(검은 막)으로 전환한다.
 * @details 이미 완전히 검은 상태(opacity>=1)면 막을 바로 표시하고, 아니면 페이드 아웃을 시작한다.
 */
void UCinematicWidget::FadeToLoadingWaitScreen()
{
	if (IsOpened() == false)
	{
		return;
	}

	if (mLoadingWaitLayerOpacity >= 1.0f)
	{
		ShowLoadingWaitScreen();   // 이미 불투명하면 페이드 없이 즉시 표시
		return;
	}

	StartFadeToBlack(ECinematicFadePurpose::LoadingWait);   // 로딩막 목적의 페이드 아웃 시작
}

/**
 * @brief 재생 애니메이션 진입점(BlueprintNativeEvent 구현). 영상 재생을 시도한다.
 * @details 영상 재생에 실패하면 고정 길이 기본 타이머로 폴백해 일정 시간 후 종료되게 한다.
 */
void UCinematicWidget::PlayCinematicAnimation_Implementation()
{
	if (PlayCinematicVideo() == false)
	{
		StartDefaultCinematicTimer(mDefaultCinematicDuration);   // 영상 재생 불가 시 기본 지속시간 타이머로 대체
	}
}

/**
 * @brief 닫기 애니메이션 진입점(BlueprintNativeEvent 구현). 검게 페이드한 뒤 위젯을 닫는다.
 * @details 이미 완전 불투명 상태면 즉시 미디어를 멈추고 닫기를 마무리하고, 아니면 닫기 목적 페이드를 시작한다.
 */
void UCinematicWidget::PlayCloseUIAnimation_Implementation()
{
	if (mLoadingWaitLayerOpacity >= 1.0f)
	{
		StopCinematicMedia();   // 이미 검은 화면이면 페이드 생략하고 바로 정지 후 닫기
		FinishCloseUI();
		return;
	}

	StartFadeToBlack(ECinematicFadePurpose::Close);   // 닫기 목적의 페이드 아웃 시작
}
