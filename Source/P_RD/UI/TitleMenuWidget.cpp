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
	// 배경 영상만 화면비에 맞춘다.
	//
	// 레이아웃은 ScaleBox 가 알아서 줄인다. 전에는 여기서 매 틱 화면비를 재
	// 다섯 벌 중 하나를 골랐는데, 재 보니 그 다섯이 서로 30~44px 밖에 안 달라
	// 한 벌로 줄였다. 고를 것이 없으면 고르는 코드도 없어야 한다.
	FitTitleBackgroundVideoToViewport();
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
	// 프로필 위젯(StartButton__base_16_9 …)을 쓰는 판인지 본다. 쓰면 접미사
	// 없는 레거시 단추가 비어 있어도 정상이다.
	const bool bUsesProfileLayouts =
		GetWidgetFromName(MakeProfileWidgetName(TEXT("StartButton"),
			TitleLayoutProfileBase16x9)) != nullptr;
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
