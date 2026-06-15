#include "UI/FrontendMapWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/FrontendMapViewPolicy.h"
#include "UI/ViewportZOrderType.h"

using namespace RDFrontendMap;

/**
 * @brief 월드맵 팝업 ZOrder와 기본 문구 캐시를 초기화한다.
 *
 * @details
 * 선/노드 WBP 클래스는 WBP_FrontendMap Class Defaults에서 지정한다.
 * C++은 특정 WBP 경로를 직접 알지 않고, 런타임에는 지정된 클래스가 없을 때 경고만 남긴다.
 * 월드맵은 탑바에서 OpenUI()로 열리는 팝업이므로 일반 HUD보다 위에 표시한다.
 */
UFrontendMapWidget::UFrontendMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
	RefreshLocalizedTextCache();
}

/**
 * @brief 디자이너 바인딩과 버튼 이벤트를 준비하고 현재 지도 데이터를 그린다.
 */
void UFrontendMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();
	BindEvents();
	ConfigureMapGraphLayout();
	RefreshLocalizedTextCache();
	HideUnusedMapTextSurfaces();
	RefreshMap();
}

/**
 * @brief 버튼/노드 이벤트 연결과 동적 생성한 그래프 풀을 정리한다.
 */
void UFrontendMapWidget::NativeDestruct()
{
	UnbindEvents();
	for (FFrontendMapNodePoolEntry& NodeEntry : mMapNodePool)
	{
		if (NodeEntry.mNodeWidget != nullptr)
		{
			NodeEntry.mNodeWidget->OnMapNodeClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);
		}
	}
	mMapLinePool.Reset();
	mMapNodePool.Reset();
	Super::NativeDestruct();
}

/**
 * @brief 지도 화면에서 사용하는 기본 문구 캐시를 갱신한다.
 */
void UFrontendMapWidget::RefreshLocalizedTextCache()
{
	mMapText = FrontendMapText(TEXT("MapText"));
	mCloseText = FrontendMapText(TEXT("CloseText"));
	mEnterText = FrontendMapText(TEXT("EnterText"));
	mLoadingStatusText = FrontendMapText(TEXT("LoadingStatusText"));
	mMapReadyStatusText = FText::GetEmpty();
	mMapUnavailableStatusText = FrontendMapText(TEXT("MapUnavailableStatusText"));
}

/**
 * @brief 현재 지도에서 방 선택 입력을 허용할지 지정한다.
 *
 * @details
 * 같은 월드맵 위젯을 조회용과 전투 승리 후 선택용으로 공유하기 위해 입력 가능 여부를 외부에서 제어한다.
 */
void UFrontendMapWidget::SetRoomSelectionEnabled(bool bEnabled)
{
	mRoomSelectionEnabled = bEnabled;
}

/**
 * @brief 현재 지도 입력 허용 상태를 반환한다.
 */
bool UFrontendMapWidget::IsRoomSelectionEnabled() const
{
	return mRoomSelectionEnabled;
}

/**
 * @brief 지도 기본 상태 문구 대신 외부 흐름의 안내 문구를 표시한다.
 */
void UFrontendMapWidget::SetMapStatusOverride(const FText& InText)
{
	mStatusOverrideText = InText;
	SetMapStatusText(mStatusOverrideText);
}

/**
 * @brief 외부 상태 문구를 해제하고 현재 캐시된 기본 상태 문구를 즉시 표시한다.
 */
void UFrontendMapWidget::ClearMapStatusOverride()
{
	mStatusOverrideText = FText::GetEmpty();
	SetMapStatusText(mMapReadyStatusText);
}

void UFrontendMapWidget::ValidateDesignerBindings() const
{
	/*
	 * 지도 위젯은 WBP_FrontendMap 안에 Graph Canvas와 ScrollBox, 버튼, 노드/선 클래스가 모두 연결되어야 동작한다.
	 * 여기서 즉시 크래시하지 않고 경고만 남기는 이유는 UI 담당자가 WBP 배치를 고치는 중에도 에디터에서 화면을 열어 확인할 수 있게 하기 위해서다.
	 */
	if (MapGraphCanvas == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapGraphCanvas is not connected. WBP_FrontendMap must provide the graph canvas."));
	}
	if (MapGraphSize == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapGraphSize is not connected. WBP_FrontendMap should own the graph size box."));
	}
	if (MapScrollBox == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapScrollBox is not connected. WBP_FrontendMap should own the scroll box."));
	}
	if (CloseButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: CloseButton is not connected."));
	}
	if (EnterRoomButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: EnterRoomButton is not connected."));
	}
	if (MapLineWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapLineWidgetClass is not set. Set WBP_FrontendMapLine in WBP_FrontendMap Class Defaults."));
	}
	if (MapNodeWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapNodeWidgetClass is not set. Set WBP_FrontendMapNode in WBP_FrontendMap Class Defaults."));
	}
}

void UFrontendMapWidget::BindEvents()
{
	/*
	 * 버튼 클릭은 지도 내부 처리로 끝내지 않고, Close는 외부 요청 이벤트로,
	 * Enter는 RoomGameMode 전환 요청으로 연결한다.
	 */
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleCloseButtonClicked);
	}
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->OnClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleEnterRoomButtonClicked);
	}
}

void UFrontendMapWidget::UnbindEvents()
{
	/* NativeConstruct에서 붙인 버튼 델리게이트를 제거해 위젯 재생성 시 중복 호출을 막는다. */
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleCloseButtonClicked);
	}
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->OnClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleEnterRoomButtonClicked);
	}
}

void UFrontendMapWidget::SetMapStatusText(const FText& InText) const
{
	/*
	 * 상태 문구 TextBlock이 WBP에서 빠진 경우도 허용한다.
	 * 지도 핵심 기능은 노드/선 표시이므로, 상태 문구가 없어도 앱이 바로 중단되지 않게 한다.
	 */
	if (MapStatusText != nullptr)
	{
		MapStatusText->SetText(InText);
	}
}

void UFrontendMapWidget::SetEnterButtonText(const FText& InText) const
{
	/* ENTER 버튼 라벨이 연결되어 있을 때만 현재 요청 상태를 표시한다. */
	if (EnterButtonText != nullptr)
	{
		EnterButtonText->SetText(InText);
	}
}

void UFrontendMapWidget::SetMapPreviewText(const FText& Title, const FText& Description, const FText& State, const FSlateColor& StateColor) const
{
	/*
	 * 노드형 WBP에서는 노드 자체 표현을 우선하고, 미리보기 패널 텍스트는 숨긴다.
	 * 함수 시그니처를 유지하는 이유는 이후 프리뷰 디자인이 다시 살아나도 RefreshMap() 호출 구조를 바꾸지 않기 위해서다.
	 */
	(void)Title;
	(void)Description;
	(void)State;
	(void)StateColor;

	HideUnusedMapTextSurfaces();
}

void UFrontendMapWidget::HideUnusedMapTextSurfaces() const
{
	/*
	 * 새 지도 WBP에서는 큰 제목/프리뷰 패널을 쓰지 않는다.
	 * 기존 WBP에 남아 있는 TextBlock이 화면에 중복 노출되지 않도록 비우고 Collapsed 처리한다.
	 */
	if (MapTitleText != nullptr)
	{
		MapTitleText->SetText(FText::GetEmpty());
		MapTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewPanel != nullptr)
	{
		MapPreviewPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewTitleText != nullptr)
	{
		MapPreviewTitleText->SetText(FText::GetEmpty());
		MapPreviewTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewDescriptionText != nullptr)
	{
		MapPreviewDescriptionText->SetText(FText::GetEmpty());
		MapPreviewDescriptionText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewStateText != nullptr)
	{
		MapPreviewStateText->SetText(FText::GetEmpty());
		MapPreviewStateText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CloseButtonText != nullptr)
	{
		CloseButtonText->SetText(mCloseText);
	}
}
