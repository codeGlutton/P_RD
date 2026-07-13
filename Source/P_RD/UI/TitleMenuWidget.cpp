/**
 * @file TitleMenuWidget.cpp
 * @brief 타이틀 메인 메뉴 위젯 구현. WBP 바인딩 연결, 메뉴 텍스트 동기화, 배경 영상 시작/종료/뷰포트 핏을 담당한다.
 * @details
 *  - 타이틀 아트/레이아웃은 WBP_TitleMenu가 보유하고, 이 C++ 클래스는 버튼 이벤트 배선과 화면 전환만 담당한다.
 *  - 배경 영상(루프 mp4) 관련 핏 로직은 *_Background.cpp 파일에 분리되어 있다.
 * @author 박용수
 * @date 2026-06-26
 */
#include "UI/TitleMenuWidget.h"
#include "UI/TitleMenuWidgetPrivate.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "UI/SettingsPanelWidget.h"

using namespace RDTitleMenu;

namespace
{
	constexpr float TitleLayoutLogViewportThreshold = 12.0f;

	void SetProfileText(const UUserWidget* Owner, const TCHAR* BaseName, const FName ProfileName, const FText& Text)
	{
		if (Owner == nullptr)
		{
			return;
		}

		if (UTextBlock* TextBlock = Cast<UTextBlock>(const_cast<UUserWidget*>(Owner)->GetWidgetFromName(MakeProfileWidgetName(BaseName, ProfileName))))
		{
			TextBlock->SetText(Text);
		}
	}

	FString FormatVec2(const FVector2D& Value)
	{
		return FString::Printf(TEXT("%.1f,%.1f"), Value.X, Value.Y);
	}

	FString DescribeWidgetGeometry(const UWidget* Widget)
	{
		if (Widget == nullptr)
		{
			return TEXT("missing");
		}

		const FGeometry Geometry = Widget->GetCachedGeometry();
		const FVector2D AbsolutePosition = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
		return FString::Printf(
			TEXT("%s local=%s abs=%s desired=%s"),
			*Widget->GetName(),
			*FormatVec2(Geometry.GetLocalSize()),
			*FormatVec2(AbsolutePosition),
			*FormatVec2(Widget->GetDesiredSize()));
	}
}

/**
 * @brief 타이틀 메인 메뉴 UI 위젯.
 * @details
 *  방 진입 직후 보이는 메인 화면으로, START/CONTINUE/SETTING 버튼과 캐릭터 선택 화면 전환,
 *  배경 영상(UMediaTexture) 표시를 담당한다. 화면 전환(메인↔캐릭터)과 하위 위젯의 "뒤로 가기"
 *  요청 처리 책임이 이 클래스에 모여 있다.
 */
/**
 * @brief 생성자. 버튼/타이틀 텍스트의 기본 문구와 기본 Visibility를 설정한다.
 * @param ObjectInitializer UObject 생성 초기화 인자(Super로 전달).
 */
UTitleMenuWidget::UTitleMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, mTitleText(NSLOCTEXT("TitleMenuWidget", "TitleText", "Rogue The Dice"))
	, mStartButtonText(NSLOCTEXT("TitleMenuWidget", "StartText", "START"))
	, mNewStartButtonText(NSLOCTEXT("TitleMenuWidget", "NewStartText", "NEW START"))
	, mContinueButtonText(NSLOCTEXT("TitleMenuWidget", "ContinueText", "CONTINUE"))
	, mSettingsButtonText(NSLOCTEXT("TitleMenuWidget", "SettingsText", "SETTINGS"))
	, mMainOnlyStatusText(NSLOCTEXT("TitleMenuWidget", "MainOnlyStatusText", "Title main screen only"))
	, mLastLoggedTitleLayoutViewportSize(FVector2D::ZeroVector)
{
	/*
	 * 타이틀 HUD는 방 진입 직후 바로 보이는 메인 UI다.
	 * 기본 Visibility를 Visible로 맞춰두되, 실제 AddToViewport/표시 생명주기는 RDUserWidget::OpenUI()가 처리한다.
	 */
	SetVisibility(ESlateVisibility::Visible);

	// 타이틀 아트(로고/버튼 텍스처)와 배치는 이제 WBP_TitleMenu가 보유한다.
	// C++ 생성자는 버튼/타이틀 텍스트 기본 문구만 정한다(위 초기화 리스트).
}

/** @brief 외부(예: GameMode)에서 타이틀 진입 시 배경 영상을 미리 시작시키기 위한 진입점. */
// 실제 시작 로직은 StartTitleBackgroundVideo()(*_Background.cpp)에 위임해 핏/재생 책임을 한곳에 둔다.
void UTitleMenuWidget::PrimeTitleBackgroundVideo()
{
	StartTitleBackgroundVideo();
}

void UTitleMenuWidget::ApplyCloseUI()
{
	StopTitleBackgroundVideo();
	Super::ApplyCloseUI();
}

/** @brief WBP 바인딩을 검증하고 타이틀 화면에서 필요한 버튼/하위 위젯 이벤트를 연결한다. */
// WBP_TitleMenu는 START/CONTINUE/SETTING 버튼과 배경 영상을 가진 타이틀 HUD다.
// 캐릭터 선택 화면은 GameMode가 여는 별도 WorldWidget이므로, 이 위젯은 START 요청만 GameMode에 전달한다.
void UTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();
	StartTitleBackgroundVideo();

	BindMainMenuButtons();

	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->OnBackRequested.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsPanelBackRequested);
	}

	SyncMainText();
	AlignMainMenuTextBlocks();
	RefreshMainMenuState();
	RefreshResponsiveTitleLayout(GetCachedGeometry().GetLocalSize());
	SetStatusText(FText::GetEmpty());
}

/**
 * @brief 매 프레임 배경 영상 브러시를 현재 뷰포트 비율에 맞춰 재계산한다.
 * @param MyGeometry 위젯의 현재 지오메트리.
 * @param InDeltaTime 직전 프레임과의 시간 간격(초).
 */
// 창 크기/해상도 변경에 즉시 반응하도록 Tick마다 좌우맞춤+상하크롭 핏을 갱신한다(*_Background.cpp).
void UTitleMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	FitTitleBackgroundVideoToViewport();
	RefreshResponsiveTitleLayout(MyGeometry.GetLocalSize());
}

/** @brief Construct에서 붙인 이벤트를 제거해 재Construct 시 중복 호출을 막는다. */
// UUserWidget은 OpenUI/CloseUI나 레벨 전환 과정에서 다시 Construct될 수 있다.
// AddUniqueDynamic을 사용하더라도 명시적으로 해제해두면 WBP 교체/하위 위젯 재생성 시 이벤트 잔류를 피할 수 있다.
void UTitleMenuWidget::NativeDestruct()
{
	StopTitleBackgroundVideo();

	UnbindMainMenuButtons();

	if (USettingsPanelWidget* TitleSettingsPanel = GetTitleSettingsPanel())
	{
		TitleSettingsPanel->OnBackRequested.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsPanelBackRequested);
	}

	Super::NativeDestruct();
}

/** @brief 생성자/에디터 기본값으로 준비된 문구를 실제 WBP TextBlock에 반영한다. */
// WBP는 레이아웃과 폰트/색을 담당하고, C++은 버튼 의미에 맞는 텍스트만 넣는다.
// 텍스트 동기화를 한 함수로 모아두면 저장 슬롯/불러오기 버튼이 추가될 때 문구 갱신 지점이 분산되지 않는다.
void UTitleMenuWidget::SyncMainText()
{
	// 타이틀 문구는 생성자에서 NSLOCTEXT로 초기화되어 현재 컬처(en/ko)에 맞춰 자동 번역된다.
	// 여기서는 그 문구를 실제 WBP TextBlock에 밀어넣기만 하며, 언어 판별/스위치는 로컬라이제이션 시스템이 담당한다.
	if (TitleText != nullptr)
	{
		TitleText->SetText(mTitleText);
	}

	if (StartButtonText != nullptr)
	{
		StartButtonText->SetText(mStartButtonText);
	}

	if (ContinueButtonText != nullptr)
	{
		ContinueButtonText->SetText(mContinueButtonText);
	}

	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(mSettingsButtonText);
	}

	const FText ExitButtonLabel = NSLOCTEXT("TitleMenuWidget", "ExitText", "EXIT");
	for (const FName ProfileName : TitleLayoutProfiles)
	{
		SetProfileText(this, TEXT("StartButtonText"), ProfileName, mStartButtonText);
		SetProfileText(this, TEXT("ContinueButtonText"), ProfileName, mContinueButtonText);
		SetProfileText(this, TEXT("SettingsButtonText"), ProfileName, mSettingsButtonText);
		SetProfileText(this, TEXT("ExitButtonText"), ProfileName, ExitButtonLabel);
	}

	if (UTextBlock* ExitButtonText = Cast<UTextBlock>(GetWidgetFromName(TEXT("ExitButtonText"))))
	{
		ExitButtonText->SetText(ExitButtonLabel);
	}
}

/** @brief 화면비에 따라 WBP 안의 프로필별 레이아웃 캔버스 중 하나만 활성화한다. */
void UTitleMenuWidget::RefreshResponsiveTitleLayout(const FVector2D& ViewportSize)
{
	if (TitleLayoutSwitcher == nullptr)
	{
		return;
	}

	const FName DesiredProfileName = SelectTitleLayoutProfile(ViewportSize);
	if (DesiredProfileName.IsNone() || DesiredProfileName == mActiveTitleLayoutProfileName)
	{
		const bool bViewportChangedEnoughForLog =
			FMath::Abs(ViewportSize.X - mLastLoggedTitleLayoutViewportSize.X) >= TitleLayoutLogViewportThreshold ||
			FMath::Abs(ViewportSize.Y - mLastLoggedTitleLayoutViewportSize.Y) >= TitleLayoutLogViewportThreshold;
		const bool bProfileChangedForLog = DesiredProfileName != mLastLoggedTitleLayoutProfileName;
		if (DesiredProfileName.IsNone() || (bViewportChangedEnoughForLog == false && bProfileChangedForLog == false))
		{
			return;
		}

		LogResponsiveTitleLayoutMetrics(
			ViewportSize,
			DesiredProfileName,
			TitleLayoutSwitcher->GetActiveWidget(),
			TitleLayoutSwitcher->GetActiveWidgetIndex());
		mLastLoggedTitleLayoutProfileName = DesiredProfileName;
		mLastLoggedTitleLayoutViewportSize = ViewportSize;
		return;
	}

	const FName DesiredScaleBoxWidgetName(*FString::Printf(TEXT("TitleLayoutScaleBox_%s"), *DesiredProfileName.ToString()));
	const FName LegacyLayoutWidgetName(*FString::Printf(TEXT("TitleLayout_%s"), *DesiredProfileName.ToString()));
	for (int32 WidgetIndex = 0; WidgetIndex < TitleLayoutSwitcher->GetNumWidgets(); ++WidgetIndex)
	{
		const UWidget* LayoutWidget = TitleLayoutSwitcher->GetWidgetAtIndex(WidgetIndex);
		if (LayoutWidget != nullptr && LayoutWidget->GetFName() == DesiredScaleBoxWidgetName)
		{
			TitleLayoutSwitcher->SetActiveWidgetIndex(WidgetIndex);
			mActiveTitleLayoutProfileName = DesiredProfileName;
			LogResponsiveTitleLayoutMetrics(ViewportSize, DesiredProfileName, LayoutWidget, WidgetIndex);
			mLastLoggedTitleLayoutProfileName = DesiredProfileName;
			mLastLoggedTitleLayoutViewportSize = ViewportSize;
			return;
		}
	}

	for (int32 WidgetIndex = 0; WidgetIndex < TitleLayoutSwitcher->GetNumWidgets(); ++WidgetIndex)
	{
		const UWidget* LayoutWidget = TitleLayoutSwitcher->GetWidgetAtIndex(WidgetIndex);
		if (LayoutWidget != nullptr && LayoutWidget->GetFName() == LegacyLayoutWidgetName)
		{
			TitleLayoutSwitcher->SetActiveWidgetIndex(WidgetIndex);
			mActiveTitleLayoutProfileName = DesiredProfileName;
			LogResponsiveTitleLayoutMetrics(ViewportSize, DesiredProfileName, LayoutWidget, WidgetIndex);
			mLastLoggedTitleLayoutProfileName = DesiredProfileName;
			mLastLoggedTitleLayoutViewportSize = ViewportSize;
			return;
		}
	}

	UE_LOG(
		LogRD,
		Warning,
		TEXT("TitleMenuWidget LayoutMetrics: no layout widget for profile=%s viewport=%s aspect=%.3f expected=%s legacy=%s switcherChildren=%d"),
		*DesiredProfileName.ToString(),
		*FormatVec2(ViewportSize),
		ViewportSize.Y > 0.0f ? ViewportSize.X / ViewportSize.Y : 0.0f,
		*DesiredScaleBoxWidgetName.ToString(),
		*LegacyLayoutWidgetName.ToString(),
		TitleLayoutSwitcher->GetNumWidgets());
}

/** @brief 타이틀 반응형 레이아웃의 실제 선택/스케일/위젯 위치를 비교 가능한 로그로 남긴다. */
void UTitleMenuWidget::LogResponsiveTitleLayoutMetrics(const FVector2D& ViewportSize, const FName ProfileName, const UWidget* ActiveLayoutWidget, const int32 ActiveWidgetIndex) const
{
	const FString ProfileString = ProfileName.ToString();
	const UWidget* MutableSizeBoxWidget = const_cast<UTitleMenuWidget*>(this)->GetWidgetFromName(FName(*FString::Printf(TEXT("TitleLayoutSizeBox_%s"), *ProfileString)));
	const USizeBox* ProfileSizeBox = Cast<USizeBox>(MutableSizeBoxWidget);
	const UWidget* ProfileCanvas = const_cast<UTitleMenuWidget*>(this)->GetWidgetFromName(FName(*FString::Printf(TEXT("TitleLayoutCanvas_%s"), *ProfileString)));
	const UWidget* LogoWidget = const_cast<UTitleMenuWidget*>(this)->GetWidgetFromName(MakeProfileWidgetName(TEXT("TitleLogoImage"), ProfileName));
	const UWidget* StartButtonWidget = const_cast<UTitleMenuWidget*>(this)->GetWidgetFromName(MakeProfileWidgetName(TEXT("StartButton"), ProfileName));
	const UWidget* VersionTextWidget = const_cast<UTitleMenuWidget*>(this)->GetWidgetFromName(MakeProfileWidgetName(TEXT("VersionText"), ProfileName));

	FVector2D DesignSize = FVector2D::ZeroVector;
	if (ProfileSizeBox != nullptr)
	{
		DesignSize.X = ProfileSizeBox->IsWidthOverride() ? ProfileSizeBox->GetWidthOverride() : ProfileSizeBox->GetDesiredSize().X;
		DesignSize.Y = ProfileSizeBox->IsHeightOverride() ? ProfileSizeBox->GetHeightOverride() : ProfileSizeBox->GetDesiredSize().Y;
	}

	float CalculatedScale = 0.0f;
	FVector2D FittedSize = FVector2D::ZeroVector;
	FVector2D LetterboxInset = FVector2D::ZeroVector;
	if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f && DesignSize.X > 0.0f && DesignSize.Y > 0.0f)
	{
		CalculatedScale = FMath::Min(ViewportSize.X / DesignSize.X, ViewportSize.Y / DesignSize.Y);
		FittedSize = DesignSize * CalculatedScale;
		LetterboxInset = (ViewportSize - FittedSize) * 0.5f;
	}

	UE_LOG(
		LogRD,
		Display,
		TEXT("TitleMenuWidget LayoutMetrics: viewport=%s aspect=%.3f profile=%s activeIndex=%d active=%s design=%s scale=%.4f fitted=%s inset=%s switcher=%s activeGeom=%s sizeBox=%s canvas=%s"),
		*FormatVec2(ViewportSize),
		ViewportSize.Y > 0.0f ? ViewportSize.X / ViewportSize.Y : 0.0f,
		*ProfileString,
		ActiveWidgetIndex,
		ActiveLayoutWidget != nullptr ? *ActiveLayoutWidget->GetName() : TEXT("missing"),
		*FormatVec2(DesignSize),
		CalculatedScale,
		*FormatVec2(FittedSize),
		*FormatVec2(LetterboxInset),
		*DescribeWidgetGeometry(TitleLayoutSwitcher),
		*DescribeWidgetGeometry(ActiveLayoutWidget),
		*DescribeWidgetGeometry(ProfileSizeBox),
		*DescribeWidgetGeometry(ProfileCanvas));

	UE_LOG(
		LogRD,
		Display,
		TEXT("TitleMenuWidget LayoutMetrics Widgets: profile=%s logo={%s} startButton={%s} versionText={%s}"),
		*ProfileString,
		*DescribeWidgetGeometry(LogoWidget),
		*DescribeWidgetGeometry(StartButtonWidget),
		*DescribeWidgetGeometry(VersionTextWidget));
}

/** @brief 현재 화면비를 타이틀 전용 레이아웃 프로필로 분류한다. */
FName UTitleMenuWidget::SelectTitleLayoutProfile(const FVector2D& ViewportSize) const
{
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		return NAME_None;
	}

	const float AspectRatio = ViewportSize.X / ViewportSize.Y;
	if (AspectRatio <= 1.35f)
	{
		return TitleLayoutProfileFoldInner;
	}
	if (AspectRatio <= 1.70f)
	{
		return TitleLayoutProfileTablet16x10;
	}
	if (AspectRatio <= 1.95f)
	{
		return TitleLayoutProfileBase16x9;
	}
	if (AspectRatio <= 2.25f)
	{
		return TitleLayoutProfilePhoneWide;
	}

	return TitleLayoutProfilePhoneUltraWide;
}

/** @brief 레거시 단일 버튼과 프로필별 버튼을 같은 입력 핸들러에 연결한다. */
void UTitleMenuWidget::BindMainMenuButtons()
{
	if (StartButton != nullptr)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}
	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}
	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}

	for (const FName ProfileName : TitleLayoutProfiles)
	{
		if (UButton* ProfileStartButton = Cast<UButton>(GetWidgetFromName(MakeProfileWidgetName(TEXT("StartButton"), ProfileName))))
		{
			ProfileStartButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
		}
		if (UButton* ProfileContinueButton = Cast<UButton>(GetWidgetFromName(MakeProfileWidgetName(TEXT("ContinueButton"), ProfileName))))
		{
			ProfileContinueButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
		}
		if (UButton* ProfileSettingsButton = Cast<UButton>(GetWidgetFromName(MakeProfileWidgetName(TEXT("SettingsButton"), ProfileName))))
		{
			ProfileSettingsButton->OnClicked.AddUniqueDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
		}
	}
}

/** @brief Construct에서 연결한 레거시/프로필별 메뉴 버튼 입력을 모두 해제한다. */
void UTitleMenuWidget::UnbindMainMenuButtons()
{
	if (StartButton != nullptr)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
	}
	if (ContinueButton != nullptr)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
	}
	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
	}

	for (const FName ProfileName : TitleLayoutProfiles)
	{
		if (UButton* ProfileStartButton = Cast<UButton>(GetWidgetFromName(MakeProfileWidgetName(TEXT("StartButton"), ProfileName))))
		{
			ProfileStartButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleStartButtonClicked);
		}
		if (UButton* ProfileContinueButton = Cast<UButton>(GetWidgetFromName(MakeProfileWidgetName(TEXT("ContinueButton"), ProfileName))))
		{
			ProfileContinueButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleContinueButtonClicked);
		}
		if (UButton* ProfileSettingsButton = Cast<UButton>(GetWidgetFromName(MakeProfileWidgetName(TEXT("SettingsButton"), ProfileName))))
		{
			ProfileSettingsButton->OnClicked.RemoveDynamic(this, &UTitleMenuWidget::HandleSettingsButtonClicked);
		}
	}
}

/** @brief WBP 생성 단계에서 빠진 버튼 텍스트 가로/세로 중앙 정렬을 런타임에서 보정한다. */
void UTitleMenuWidget::AlignMainMenuTextBlocks()
{
	AlignMenuTextBlock(StartButtonText);
	AlignMenuTextBlock(ContinueButtonText);
	AlignMenuTextBlock(SettingsButtonText);
	AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(TEXT("ExitButtonText"))));
	AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(TEXT("VersionText"))));

	for (const FName ProfileName : TitleLayoutProfiles)
	{
		AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(MakeProfileWidgetName(TEXT("StartButtonText"), ProfileName))));
		AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(MakeProfileWidgetName(TEXT("ContinueButtonText"), ProfileName))));
		AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(MakeProfileWidgetName(TEXT("SettingsButtonText"), ProfileName))));
		AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(MakeProfileWidgetName(TEXT("ExitButtonText"), ProfileName))));
		AlignMenuTextBlock(Cast<UTextBlock>(GetWidgetFromName(MakeProfileWidgetName(TEXT("VersionText"), ProfileName))));
	}
}

/** @brief CanvasPanel 직계 TextBlock을 같은 위치의 Overlay로 감싸 슬롯 VerticalAlignment를 줄 수 있게 한다. */
void UTitleMenuWidget::AlignMenuTextBlock(UTextBlock* TextBlock)
{
	if (TextBlock == nullptr)
	{
		return;
	}

	TextBlock->SetJustification(ETextJustify::Center);

	if (UOverlaySlot* ExistingOverlaySlot = Cast<UOverlaySlot>(TextBlock->Slot))
	{
		ExistingOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		ExistingOverlaySlot->SetVerticalAlignment(VAlign_Center);
		return;
	}

	UCanvasPanelSlot* SourceCanvasSlot = Cast<UCanvasPanelSlot>(TextBlock->Slot);
	UPanelWidget* SourceParent = TextBlock->GetParent();
	if (WidgetTree == nullptr || SourceCanvasSlot == nullptr || SourceParent == nullptr)
	{
		return;
	}

	const FAnchors Anchors = SourceCanvasSlot->GetAnchors();
	const FMargin Offsets = SourceCanvasSlot->GetOffsets();
	const FVector2D Alignment = SourceCanvasSlot->GetAlignment();
	const int32 ZOrder = SourceCanvasSlot->GetZOrder();
	const bool bAutoSize = SourceCanvasSlot->GetAutoSize();

	const FName OverlayName = MakeUniqueObjectName(this, UOverlay::StaticClass(), *FString::Printf(TEXT("%s_CenterOverlay"), *TextBlock->GetName()));
	UOverlay* CenterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayName);
	if (CenterOverlay == nullptr)
	{
		return;
	}

	SourceParent->RemoveChild(TextBlock);
	if (UCanvasPanel* CanvasParent = Cast<UCanvasPanel>(SourceParent))
	{
		UCanvasPanelSlot* OverlayCanvasSlot = CanvasParent->AddChildToCanvas(CenterOverlay);
		if (OverlayCanvasSlot == nullptr)
		{
			return;
		}

		OverlayCanvasSlot->SetAnchors(Anchors);
		OverlayCanvasSlot->SetOffsets(Offsets);
		OverlayCanvasSlot->SetAlignment(Alignment);
		OverlayCanvasSlot->SetAutoSize(bAutoSize);
		OverlayCanvasSlot->SetZOrder(ZOrder);
	}
	else
	{
		SourceParent->AddChild(CenterOverlay);
	}

	UOverlaySlot* TextOverlaySlot = CenterOverlay->AddChildToOverlay(TextBlock);
	if (TextOverlaySlot != nullptr)
	{
		TextOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		TextOverlaySlot->SetVerticalAlignment(VAlign_Center);
		TextOverlaySlot->SetPadding(FMargin(0.0f));
	}
}

/** @brief 현재 타이틀 화면에서 사용하지 않는 상태 문구 영역을 항상 숨긴다. */
// 새 타이틀 WBP는 별도 상태 텍스트를 사용하지 않는다.
// 기존 C++ 호출부 호환을 위해 함수는 남겨두지만, 어떤 텍스트가 들어와도 화면에는 표시하지 않는다.
void UTitleMenuWidget::SetStatusText(const FText& /*InText*/) const
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/** @brief BindWidget으로 들어와야 할 WBP 하위 위젯/버튼/텍스트가 모두 연결됐는지 검증하고 경고 로그를 남긴다. */
// WBP 디자이너에서 위젯 이름을 잘못 바꾸거나 배치를 빠뜨리면 nullptr가 되므로,
// NativeConstruct 초기에 각 바인딩을 점검해 어떤 위젯이 누락됐는지 명시적으로 로깅한다(배경 영상은 누락 시 스킵).
void UTitleMenuWidget::ValidateDesignerBindings() const
{
	if (TitleLayoutSwitcher == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: TitleLayoutSwitcher is not connected. Legacy single title layout will be used if available."));
	}

	const bool bUsesProfileLayouts = TitleLayoutSwitcher != nullptr;
	if (bUsesProfileLayouts == false && StartButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartButton is not connected."));
	}

	if (bUsesProfileLayouts == false && ContinueButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ContinueButton is not connected."));
	}

	if (bUsesProfileLayouts == false && SettingsButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsButton is not connected."));
	}

	if (bUsesProfileLayouts == false && StartButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: StartButtonText is not connected."));
	}

	if (bUsesProfileLayouts == false && ContinueButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: ContinueButtonText is not connected."));
	}

	if (bUsesProfileLayouts == false && SettingsButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: SettingsButtonText is not connected."));
	}

	if (TitleBackgroundImage == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: TitleBackgroundImage is not connected. Background video will be skipped."));
	}

	if (GetTitleSettingsPanel() == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TitleMenuWidget: InGameSettings world widget is not configured."));
	}

	SetStatusText(FText::GetEmpty());
}
