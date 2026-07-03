#include "UI/SettingsPanelWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "UI/ViewportZOrderType.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "P_RD.h"

/**
 * @brief 설정 패널을 팝업 레이어에 표시되도록 초기화한다.
 *
 * @details
 * 설정 패널은 타이틀 메뉴 위나 인게임 상단 메뉴 위에 뜨는 보조 화면이다.
 * 기본 ZOrder를 PopUp으로 두면 OpenUI()로 열릴 때 HUD/월드 위젯보다 앞에 배치되어 입력 우선순위가 자연스럽다.
 * 팝업 계층에 올라와야 Back/Save/Abandon 같은 버튼 입력이 뒤쪽 메뉴나 HUD에 섞이지 않는다.
 */
USettingsPanelWidget::USettingsPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/**
 * @brief WBP 버튼/슬라이더를 이 위젯의 입력 처리 함수에 연결하고 처음 표시 상태를 맞춘다.
 *
 * @details
 * SettingsPanelWidget은 설정 화면의 입력만 받는다. 저장, 런 포기, 타이틀 이동 같은 실제 게임 처리는
 * 직접 하지 않고 OnSaveAndExitRequested, OnAbandonRunConfirmed 같은 이벤트로 바깥에 알려준다.
 *
 * 왜 이렇게 나누는가:
 * 같은 설정 패널을 타이틀과 인게임에서 함께 쓰기 때문이다. 패널 안에 GameMode/저장/전환 로직을 넣으면
 * 타이틀용 설정과 인게임용 설정이 서로의 흐름을 알아야 하므로 재사용하기 어렵다.
 *
 * 구성 순서:
 * 먼저 WBP 바인딩 누락을 로그로 확인하고, 버튼/슬라이더/체크박스 입력을 C++ 핸들러에 연결한다.
 * 그 뒤 기본 문구, 모드별 표시 상태, 런 포기 확인 패널의 초기 상태를 맞춘다.
 */
void USettingsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

	/* 화면 복귀와 런 액션 버튼은 클릭을 외부 요청 이벤트로만 변환한다. */

	if (BackButton != nullptr)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleBackButtonClicked);
	}
	if (SaveAndExitButton != nullptr)
	{
		SaveAndExitButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleSaveAndExitButtonClicked);
	}
	if (AbandonRunButton != nullptr)
	{
		AbandonRunButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleAbandonRunButtonClicked);
	}
	if (ConfirmAbandonButton != nullptr)
	{
		ConfirmAbandonButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleConfirmAbandonButtonClicked);
	}
	if (CancelAbandonButton != nullptr)
	{
		CancelAbandonButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleCancelAbandonButtonClicked);
	}
	if (ResetButton != nullptr)
	{
		ResetButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleResetButtonClicked);
	}

	/* 품질 버튼은 현재 패널의 임시 품질 번호를 외부 설정 정책으로 전달한다. */

	if (LowQualityButton != nullptr)
	{
		LowQualityButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleLowQualityButtonClicked);
	}
	if (MediumQualityButton != nullptr)
	{
		MediumQualityButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleMediumQualityButtonClicked);
	}
	if (HighQualityButton != nullptr)
	{
		HighQualityButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleHighQualityButtonClicked);
	}

	/* 값 입력 위젯은 입력 값을 그대로 이벤트로 올리고, 실제 적용/저장은 외부 시스템에 맡긴다. */

	if (BgmVolumeSlider != nullptr)
	{
		BgmVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleBgmVolumeChanged);
	}
	if (SfxVolumeSlider != nullptr)
	{
		SfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleSfxVolumeChanged);
	}
	if (UiVolumeSlider != nullptr)
	{
		UiVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleUiVolumeChanged);
	}
	if (ScreenShakeCheckBox != nullptr)
	{
		ScreenShakeCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleScreenShakeChanged);
	}
	if (VibrationCheckBox != nullptr)
	{
		VibrationCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleVibrationChanged);
	}

	if (MasterVolumeSlider != nullptr)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleMasterVolumeChanged);
	}

	if (FpsThirtyButton != nullptr)
	{
		FpsThirtyButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleFpsThirtyButtonClicked);
	}

	if (FpsSixtyButton != nullptr)
	{
		FpsSixtyButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleFpsSixtyButtonClicked);
	}

	if (LanguageKoreanButton != nullptr)
	{
		LanguageKoreanButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleLanguageKoreanButtonClicked);
	}

	if (LanguageEnglishButton != nullptr)
	{
		LanguageEnglishButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleLanguageEnglishButtonClicked);
	}

	if (EffectsCheckBox != nullptr)
	{
		EffectsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleEffectsChanged);
	}

	/* WBP가 가진 임시 표시 상태를 C++ 기본 표시 정책으로 정리한다. */

	SyncText();
	ApplyModeVisibility();
	HideAbandonConfirm();
}

/**
 * @brief NativeConstruct()에서 연결한 WBP 입력 이벤트를 해제한다.
 *
 * @details
 * UMG 위젯은 화면에서 빠졌다가 다시 붙거나, 에디터에서 재생을 반복하면서 Construct/Destruct를 여러 번 거칠 수 있다.
 * Construct에서 AddUniqueDynamic을 사용해 중복 연결을 줄이고, Destruct에서 RemoveDynamic으로 정리해 생명주기 끝에 입력 연결이 남지 않게 한다.
 *
 * 왜 모든 입력을 다시 풀어주는가:
 * 설정 패널은 OpenUI/CloseUI 흐름으로 여러 번 열릴 수 있다. 이전 인스턴스의 Delegate가 남아 있으면 버튼 한 번에
 * 같은 요청이 여러 번 Broadcast될 수 있으므로, Construct에서 연결한 항목은 Destruct에서 같은 목록으로 해제한다.
 */

void USettingsPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ApplyFoldScaleVariant(MyGeometry.GetLocalSize());
	SyncSliderFillBars();

	// [진단] 뷰포트가 바뀌면 8프레임 뒤(geometry 안정 후) 설정 레이아웃 전수 로그를 1회 남긴다.
	const FVector2D MetricsViewport = MyGeometry.GetLocalSize();
	if (MetricsViewport.X > 1.0f && MetricsViewport.Y > 1.0f
		&& (FMath::Abs(MetricsViewport.X - mMetricsLastViewport.X) >= 1.0f
			|| FMath::Abs(MetricsViewport.Y - mMetricsLastViewport.Y) >= 1.0f))
	{
		mMetricsLastViewport = MetricsViewport;
		mMetricsCountdown = 8;
	}
	if (mMetricsCountdown > 0 && --mMetricsCountdown == 0)
	{
		LogSettingsMetrics(MetricsViewport);
	}
}

void USettingsPanelWidget::ApplyFoldScaleVariant(const FVector2D& ViewportSize)
{
	if (ViewportSize.X <= 1.0f || ViewportSize.Y <= 1.0f || WidgetTree == nullptr)
	{
		return;
	}
	// 레터박스 WBP(SettingsModalCanvas 존재)면 ScaleBox가 전 화면비를 전담한다 — 폴드 스케일 이중 적용 금지.
	if (WidgetTree->FindWidget(TEXT("SettingsModalCanvas")) != nullptr)
	{
		return;
	}
	// 컷은 concept/전투 HUD와 동기(1.68). 상태가 안 바뀌면 아무것도 하지 않는다.
	constexpr float FoldAspectCut = 1.68f;
	const bool bFold = (ViewportSize.X / ViewportSize.Y) < FoldAspectCut;
	if (bFold == mFoldScaleActive)
	{
		return;
	}

	// 스케일 비율 = 폴드 마커 폭 / 기준 마커 폭 (디자이너 데이터). 마커 없는 구 WBP는 변형 생략.
	auto GetMarkerWidth = [this](const TCHAR* MarkerName) -> float
	{
		UWidget* Marker = WidgetTree->FindWidget(FName(MarkerName));
		UCanvasPanelSlot* MarkerSlot = Marker != nullptr ? Cast<UCanvasPanelSlot>(Marker->Slot) : nullptr;
		return MarkerSlot != nullptr ? MarkerSlot->GetSize().X : 0.0f;
	};
	const float BaseWidth = GetMarkerWidth(TEXT("Set_grid_base"));
	const float FoldWidth = GetMarkerWidth(TEXT("Set_grid_fold"));
	if (BaseWidth <= 0.0f || FoldWidth <= 0.0f)
	{
		return;
	}
	const float Scale = FoldWidth / BaseWidth;

	// 중앙(0.5,0.5) 점앵커 = 시안 모달 그리드 콘텐츠. 화면 중앙 기준 균일 스케일을 렌더 트랜스폼으로 얹는다.
	// 슬롯은 불변이라 복원은 항등 트랜스폼이고, 텍스트/아트가 함께 스케일되며 히트테스트도 Slate가 변환을 반영한다.
	WidgetTree->ForEachWidget([Scale, bFold](UWidget* Widget)
	{
		UCanvasPanelSlot* CanvasSlot = Widget != nullptr ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (CanvasSlot == nullptr)
		{
			return;
		}
		const FAnchors SlotAnchors = CanvasSlot->GetAnchors();
		if (SlotAnchors.Minimum != FVector2D(0.5f, 0.5f) || SlotAnchors.Maximum != FVector2D(0.5f, 0.5f))
		{
			return;
		}
		if (bFold)
		{
			const FVector2D CenterOffset = CanvasSlot->GetPosition();
			Widget->SetRenderTransformPivot(FVector2D::ZeroVector);
			Widget->SetRenderTransform(FWidgetTransform(
				FVector2D(CenterOffset.X * (Scale - 1.0f), CenterOffset.Y * (Scale - 1.0f)),
				FVector2D(Scale, Scale), FVector2D::ZeroVector, 0.0f));
		}
		else
		{
			Widget->SetRenderTransform(FWidgetTransform());
			Widget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		}
	});

	mFoldScaleActive = bFold;
}


void USettingsPanelWidget::SyncSliderFillBars()
{
	if (mFillBarsResolved == false && WidgetTree != nullptr)
	{
		mMasterFillBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("Set_slider_fill_master")));
		mBgmFillBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("Set_slider_fill_bgm")));
		mSfxFillBar = Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("Set_slider_fill_sfx")));
		mFillBarsResolved = true;
	}
	// 시안 채움 텍스처(ProgressBar)를 슬라이더 현재 값에 맞춘다 — UMG Slider 스타일엔 채움 슬롯이 없어 오버레이로 구현.
	if (mMasterFillBar.IsValid() && MasterVolumeSlider != nullptr)
	{
		mMasterFillBar->SetPercent(MasterVolumeSlider->GetValue());
	}
	if (mBgmFillBar.IsValid() && BgmVolumeSlider != nullptr)
	{
		mBgmFillBar->SetPercent(BgmVolumeSlider->GetValue());
	}
	if (mSfxFillBar.IsValid() && SfxVolumeSlider != nullptr)
	{
		mSfxFillBar->SetPercent(SfxVolumeSlider->GetValue());
	}
}


void USettingsPanelWidget::LogSettingsMetrics(const FVector2D& ViewportSize)
{
	if (WidgetTree == nullptr)
	{
		return;
	}
	auto GeomStr = [](UWidget* Widget) -> FString
	{
		if (Widget == nullptr)
		{
			return TEXT("null");
		}
		const FGeometry& Geom = Widget->GetCachedGeometry();
		const FVector2D LocalSize = Geom.GetLocalSize();
		const FVector2D AbsPos = Geom.GetAbsolutePosition();
		const FVector2D AbsSize = Geom.GetAbsoluteSize();
		return FString::Printf(TEXT("local=%.0fx%.0f abs=(%.0f,%.0f %.0fx%.0f)"), LocalSize.X, LocalSize.Y, AbsPos.X, AbsPos.Y, AbsSize.X, AbsSize.Y);
	};
	auto SlotStr = [](UWidget* Widget) -> FString
	{
		UCanvasPanelSlot* CanvasSlot = Widget != nullptr ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (CanvasSlot == nullptr)
		{
			return TEXT("slot=none");
		}
		const FAnchorData Layout = CanvasSlot->GetLayout();
		return FString::Printf(TEXT("anchor=(%.2f,%.2f) off=(%.1f,%.1f,%.1f,%.1f) z=%d auto=%d"),
			Layout.Anchors.Minimum.X, Layout.Anchors.Minimum.Y,
			Layout.Offsets.Left, Layout.Offsets.Top, Layout.Offsets.Right, Layout.Offsets.Bottom,
			CanvasSlot->GetZOrder(), CanvasSlot->GetAutoSize() ? 1 : 0);
	};

	UWidget* ModalCanvas = WidgetTree->FindWidget(TEXT("SettingsModalCanvas"));
	UE_LOG(LogRD, Display, TEXT("SettingsMetrics: viewport=%.1f,%.1f aspect=%.3f letterbox=%s foldScale=%d scaleBox={%s} sizeBox={%s} modalCanvas={%s} root={%s}"),
		ViewportSize.X, ViewportSize.Y, ViewportSize.Y > 0.0f ? ViewportSize.X / ViewportSize.Y : 0.0f,
		ModalCanvas != nullptr ? TEXT("on") : TEXT("off"), mFoldScaleActive ? 1 : 0,
		*GeomStr(WidgetTree->FindWidget(TEXT("SettingsScaleBox"))),
		*GeomStr(WidgetTree->FindWidget(TEXT("SettingsSizeBox"))),
		*GeomStr(ModalCanvas),
		*GeomStr(WidgetTree->FindWidget(TEXT("SettingsPanelRoot"))));

	// 패널 프레임 브러시(9-slice 확인)
	if (UImage* PanelFrame = Cast<UImage>(WidgetTree->FindWidget(TEXT("Set_panel_frame"))))
	{
		const FSlateBrush& Brush = PanelFrame->GetBrush();
		UE_LOG(LogRD, Display, TEXT("SettingsMetrics Frame: %s %s drawAs=%d margin=(%.2f,%.2f,%.2f,%.2f) img=%s"),
			*SlotStr(PanelFrame), *GeomStr(PanelFrame), StaticCast<int32>(Brush.DrawAs),
			Brush.Margin.Left, Brush.Margin.Top, Brush.Margin.Right, Brush.Margin.Bottom,
			Brush.GetResourceObject() != nullptr ? *Brush.GetResourceObject()->GetName() : TEXT("null"));
	}

	static const TCHAR* PlainWidgets[] = {
		TEXT("Set_scrim"), TEXT("SettingsTitleText"), TEXT("Set_btn_close"),
		TEXT("Set_row_fps_plate"), TEXT("Set_seg_fps"), TEXT("Set_row_fps_label"),
		TEXT("Set_row_quality_plate"), TEXT("Set_seg_quality"), TEXT("QualityRow_Label"),
		TEXT("Set_row_language_plate"), TEXT("Set_seg_language"), TEXT("LanguageRow_Label"),
		TEXT("Set_row_master_plate"), TEXT("MasterVolumeSlider"), TEXT("Set_slider_fill_master"),
		TEXT("Set_sec_graphics_banner"), TEXT("Set_sec_graphics_text"),
		TEXT("BackButton"), TEXT("SaveAndExitButton"), TEXT("AbandonRunButton"), TEXT("ResetButton"),
		TEXT("Set_grid_base"), TEXT("Set_grid_fold"),
	};
	for (const TCHAR* WidgetName : PlainWidgets)
	{
		UWidget* Widget = WidgetTree->FindWidget(WidgetName);
		if (Widget != nullptr)
		{
			UE_LOG(LogRD, Display, TEXT("SettingsMetrics W: %s %s %s vis=%d"), WidgetName, *SlotStr(Widget), *GeomStr(Widget), Widget->IsVisible() ? 1 : 0);
		}
		else
		{
			UE_LOG(LogRD, Display, TEXT("SettingsMetrics W: %s MISSING"), WidgetName);
		}
	}

	// 세그 버튼: 슬롯 + 자식 텍스트(폰트/실측 폭/패딩) — 텍스트 넘침의 직접 증거
	static const TCHAR* SegButtons[] = {
		TEXT("FpsThirtyButton"), TEXT("FpsSixtyButton"),
		TEXT("QualityLowButton"), TEXT("QualityMidButton"), TEXT("QualityHighButton"),
		TEXT("LanguageKoreanButton"), TEXT("LanguageEnglishButton"),
	};
	for (const TCHAR* ButtonName : SegButtons)
	{
		UButton* Button = Cast<UButton>(WidgetTree->FindWidget(ButtonName));
		if (Button == nullptr)
		{
			UE_LOG(LogRD, Display, TEXT("SettingsMetrics Btn: %s MISSING"), ButtonName);
			continue;
		}
		FString ChildInfo = TEXT("noChild");
		if (Button->GetChildrenCount() > 0)
		{
			UWidget* Child = Button->GetChildAt(0);
			UButtonSlot* ChildSlot = Child != nullptr ? Cast<UButtonSlot>(Child->Slot) : nullptr;
			const FMargin Pad = ChildSlot != nullptr ? ChildSlot->GetPadding() : FMargin();
			float FontSize = -1.0f;
			FString TextValue;
			if (UTextBlock* TextChild = Cast<UTextBlock>(Child))
			{
				FontSize = TextChild->GetFont().Size;
				TextValue = TextChild->GetText().ToString();
			}
			const FVector2D Desired = Child != nullptr ? Child->GetDesiredSize() : FVector2D::ZeroVector;
			ChildInfo = FString::Printf(TEXT("text='%s' font=%.0f desired=%.0fx%.0f pad=(%.0f,%.0f,%.0f,%.0f) childGeom=%s"),
				*TextValue, FontSize, Desired.X, Desired.Y, Pad.Left, Pad.Top, Pad.Right, Pad.Bottom, *GeomStr(Child));
		}
		UE_LOG(LogRD, Display, TEXT("SettingsMetrics Btn: %s %s %s %s"), ButtonName, *SlotStr(Button), *GeomStr(Button), *ChildInfo);
	}
}

void USettingsPanelWidget::NativeDestruct()
{
	/* NativeConstruct()에서 연결한 버튼 입력 Delegate를 해제한다. */

	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleBackButtonClicked);
	}
	if (SaveAndExitButton != nullptr)
	{
		SaveAndExitButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleSaveAndExitButtonClicked);
	}
	if (AbandonRunButton != nullptr)
	{
		AbandonRunButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleAbandonRunButtonClicked);
	}
	if (ConfirmAbandonButton != nullptr)
	{
		ConfirmAbandonButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleConfirmAbandonButtonClicked);
	}
	if (CancelAbandonButton != nullptr)
	{
		CancelAbandonButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleCancelAbandonButtonClicked);
	}
	if (ResetButton != nullptr)
	{
		ResetButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleResetButtonClicked);
	}
	if (LowQualityButton != nullptr)
	{
		LowQualityButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleLowQualityButtonClicked);
	}
	if (MediumQualityButton != nullptr)
	{
		MediumQualityButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleMediumQualityButtonClicked);
	}
	if (HighQualityButton != nullptr)
	{
		HighQualityButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleHighQualityButtonClicked);
	}

	/* 슬라이더/체크박스 값 변경 Delegate도 같은 생명주기에서 정리한다. */

	if (BgmVolumeSlider != nullptr)
	{
		BgmVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleBgmVolumeChanged);
	}
	if (SfxVolumeSlider != nullptr)
	{
		SfxVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleSfxVolumeChanged);
	}
	if (UiVolumeSlider != nullptr)
	{
		UiVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleUiVolumeChanged);
	}
	if (ScreenShakeCheckBox != nullptr)
	{
		ScreenShakeCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleScreenShakeChanged);
	}
	if (VibrationCheckBox != nullptr)
	{
		VibrationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleVibrationChanged);
	}

	Super::NativeDestruct();
}

/**
 * @brief Back 버튼 입력을 외부 복귀 요청 이벤트로 전달한다.
 *
 * @details
 * 이 함수는 화면을 닫지 않고 OnBackRequested만 Broadcast한다.
 * 타이틀에서는 타이틀 메뉴로 돌아가고, 인게임에서는 상단 메뉴 흐름으로 돌아가는 식으로 수신자마다 복귀 방식이 다르기 때문이다.
 */
void USettingsPanelWidget::HandleBackButtonClicked()
{
	OnBackRequested.Broadcast();
}
