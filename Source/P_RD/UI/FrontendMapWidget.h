/*****************************************************************//**
 * @file   FrontendMapWidget.h
 * @brief  방 공용 월드맵 화면 위젯 정의 헤더
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UI/RoomViewTypes.h"
#include "UI/RDUserWidget.h"

#include "FrontendMapWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UFrontendMapLineWidget;
class UFrontendMapNodeWidget;
class UHorizontalBox;
class UImage;
class UVerticalBox;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrontendMapWidgetSimpleEvent);

USTRUCT()
struct FFrontendMapLinePoolEntry
{
	GENERATED_BODY()

	/**
	 * @brief 지도 연결선 WBP 인스턴스
	 *
	 * @details
	 * RefreshMap()마다 새로 만들지 않고 풀에 보관해 재사용한다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UFrontendMapLineWidget> mLineWidget;
};

USTRUCT()
struct FFrontendMapNodePoolEntry
{
	GENERATED_BODY()

	/**
	 * @brief 지도 방 노드 WBP 인스턴스
	 *
	 * @details
	 * 방 개수가 바뀌어도 기존 노드를 재사용하고, 이번 갱신에서 쓰지 않는 노드는 숨긴다.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UFrontendMapNodeWidget> mNodeWidget;
};

/**
 * @brief 월드맵 화면 본체
 *
 * @details
 * 모달 표시/닫기 책임은 바깥 오버레이가 갖고, 이 클래스는 맵 그래프/선택/입장 UI만 관리한다.
 * 지도 데이터는 직접 생성하지 않고 GameMode::GetMapRoomViews()가 내려준 View DTO를 그린다.
 *
 * 왜 조회용/선택용을 같은 위젯으로 처리하는가:
 * MAP 버튼으로 열 때는 현재 런 경로를 보기만 해야 하고, 전투 승리 후에는 다음 방을 실제로 선택해야 한다.
 * 화면 구조는 같지만 허용 입력만 다르므로 mRoomSelectionEnabled로 모드를 나누어 같은 WBP를 재사용한다.
 * 방 선택/입장 판단은 GameMode API로 돌려보내며, 실제 전환은 RoomTransitionSubsystem 흐름을 탄다.
 *
 * 현재 노드 안에 보이는 "행-열 + 룸 종류" 텍스트는 지도 데이터 배치와 RoomType 매핑을 확인하기 위한
 * 디버깅용 표시다. 최종 UI에서는 RoomType별 아이콘/이미지를 WBP 또는 공식 Stage/Room 표시 데이터에서
 * 받아 노드 위젯에 넣고, 이 디버그 텍스트는 제거하는 방향이다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief 지도 팝업의 기본 ZOrder와 기본 문구 캐시를 준비한다.
	 */
	UFrontendMapWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 현재 RunPersistData 기반 지도 View를 다시 가져와 노드/선을 갱신한다.
	 * @return 지도 데이터가 있어 갱신에 성공하면 true
	 *
	 * FStage/FRoom을 직접 만드는 함수가 아니라, GameMode가 변환해준
	 * FMapRoomView 배열을 화면에 배치하는 표시 전용 함수다.
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool RefreshMap();

	/**
	 * @brief 승리 후 지도처럼 다음 방 선택이 허용되는 상황에서만 true로 둔다.
	 *
	 * @details
	 * 같은 월드맵을 단순 조회와 다음 방 선택에 함께 쓰기 때문에, 열린 경로에 따라 노드/입장 버튼 입력을 분리한다.
	 */
	void SetRoomSelectionEnabled(bool bEnabled);

	/**
	 * @brief 현재 월드맵이 방 선택 입력을 허용하는지 확인한다.
	 *
	 * @return 방 선택/입장 입력을 받을 수 있으면 true
	 */
	bool IsRoomSelectionEnabled() const;

	/** @brief 탑바/전환 흐름이 일시적으로 표시할 상태 문구를 지정한다. */
	void SetMapStatusOverride(const FText& InText);

	/**
	 * @brief 외부에서 지정한 상태 문구를 해제하고 현재 캐시된 지도 기본 상태 문구로 되돌린다.
	 */
	void ClearMapStatusOverride();

	UPROPERTY(Category = UI, BlueprintAssignable)
	FFrontendMapWidgetSimpleEvent OnCloseRequested;

	/* UUserWidget 상속 */
protected:
	/**
	 * @brief WBP 바인딩과 버튼 이벤트를 연결한 뒤 현재 지도 데이터를 그린다.
	 */
	void NativeConstruct() override;

	/**
	 * @brief 버튼/노드 이벤트와 동적 노드/선 풀을 정리한다.
	 */
	void NativeDestruct() override;

	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** @brief 지도 위젯에서 사용하는 기본 문구 캐시를 갱신한다. */
	void RefreshLocalizedTextCache();

	/** @brief WBP_FrontendMap이 필수 바인딩과 노드/선 클래스를 제공하는지 로그로 확인한다. */
	void ValidateDesignerBindings() const;

	/** @brief 닫기/입장 버튼 이벤트를 연결한다. */
	void BindEvents();

	/** @brief 닫기/입장 버튼 이벤트를 해제한다. */
	void UnbindEvents();

	/** @brief 지도 노드 클릭을 GameMode의 방 선택 요청으로 전달한다. */
	void HandleMapRoomClicked(int32 RowIndex, int32 ColumnIndex);

	/** @brief 지도 상태 문구 TextBlock을 갱신한다. */
	void SetMapStatusText(const FText& InText) const;

	/** @brief ENTER 버튼 라벨을 요청 상태에 맞게 갱신한다. */
	void SetEnterButtonText(const FText& InText) const;

	/** @brief 조회용 지도에서는 ENTER 버튼 자체를 숨긴다. */
	void SetEnterButtonVisible(bool bVisible) const;

	/** @brief 선택 방 프리뷰 텍스트를 갱신한다. 현재 WBP에서는 숨김 처리만 유지한다. */
	void SetMapPreviewText(const FText& Title, const FText& Description, const FText& State, const FSlateColor& StateColor) const;

	/** @brief 노드형 지도 WBP에서는 쓰지 않는 텍스트형 지도 영역을 숨긴다. */
	void HideUnusedMapTextSurfaces() const;

	/** @brief 현재 월드에서 실제 방 선택/입장 요청을 처리할 수 있는지 확인한다. */
	bool IsFrontendMapNavigationEnabled() const;

	/** @brief 현재 화면 폭 + 행 수 기반 지도 스크롤 콘텐츠 크기를 계산한다(폭=뷰포트 풀블리드, 높이=행수x행간격). */
	FVector2D GetMapGraphContentSize() const;

	/** @brief 지도 캔버스와 스크롤 영역의 고정 레이아웃 값을 적용한다. */
	void ConfigureMapGraphLayout() const;

	/** @brief 노드 크기(시안 마커 Map_NodeMetrics 폭, 폴백 96). */
	FVector2D GetMapNodeSize() const;

	/** @brief 행 간격(시안 마커 Map_NodeMetrics 높이, 폴백 176) — 그래프 높이의 정본. */
	float GetMapRowPitch() const;

	/** @brief 열 간격 상한(시안 마커 Map_ColPitch 폭, 폴백 240) — 열이 적을 때 중앙으로 모아 간격 과대를 막는다. */
	float GetMapColPitchMax() const;

	/** @brief 노드 배치 경계(시안 마커 Map_NodeArea): 좌우는 앵커 분수, 상하는 px 오프셋. */
	void GetNodeAreaLayout(float& OutLeftFrac, float& OutRightFrac, float& OutTopPx, float& OutBottomPx) const;

	/** @brief Stage row/column을 시안 경계/간격 기반 캔버스 좌표로 변환한다. */
	FVector2D GetMapRoomNodeCenter(const TArray<FMapRoomView>& Rooms, const FMapRoomView& Room, const FVector2D& GraphSize) const;

	/** @brief 양피지/두루마리(WBP 소유)를 동적 그래프 크기에 슬롯만 동기한다. */
	void UpdateGraphDecorLayout(const FVector2D& GraphSize) const;

	/** @brief 현재 위치 마커/선택 글로우(WBP 소유)를 해당 노드 위치로 옮긴다. */
	void UpdateOverlayMarkers(const TMap<FIntPoint, FVector2D>& NodeCenters, const FIntPoint& CurrentCoord, const FIntPoint& SelectedCoord) const;

	/** @brief 좌측 스테이지 정보 패널(WBP 소유)의 진행도 텍스트를 채운다. */
	void UpdateStageInfoText(int32 CurrentRowIndex, int32 RowCount) const;

	/** @brief 지정 인덱스의 연결선 위젯을 풀에서 가져오거나 새로 만든다. */
	FFrontendMapLinePoolEntry* AcquireMapLineWidget(int32 LineIndex);

	/** @brief 지정 인덱스의 방 노드 위젯을 풀에서 가져오거나 새로 만든다. */
	FFrontendMapNodePoolEntry* AcquireMapNodeWidget(int32 NodeIndex);

	/** @brief 이번 RefreshMap()에서 쓰지 않은 노드/선을 숨겨 다음 갱신 때 재사용한다. */
	void HideUnusedMapGraphWidgets(int32 UsedLineCount, int32 UsedNodeCount);

	/** @brief 닫기 버튼 입력을 외부 닫기 요청 이벤트로 전달한다. */
	UFUNCTION()
	void HandleCloseButtonClicked();

	/** @brief 선택된 방 입장을 GameMode에 요청한다. */
	UFUNCTION()
	void HandleEnterRoomButtonClicked();

	/** @brief 노드 위젯 클릭 이벤트를 지도 선택 처리로 연결한다. */
	UFUNCTION()
	void HandleMapNodeClicked(int32 RowIndex, int32 ColumnIndex);

private:
	/** @brief MAP 제목 문구 캐시 */
	FText mMapText;

	/** @brief 닫기 버튼 문구 캐시 */
	FText mCloseText;

	/** @brief 입장 버튼 기본 문구 캐시 */
	FText mEnterText;

	/** @brief 입장 요청 중 표시할 문구 캐시 */
	FText mLoadingStatusText;

	/** @brief 지도 데이터가 준비되었을 때 표시할 기본 상태 문구 */
	FText mMapReadyStatusText;

	/** @brief 지도 데이터를 가져올 수 없을 때 표시할 상태 문구 */
	FText mMapUnavailableStatusText;

	/**
	 * @brief 탑바/전투 결과 흐름이 지도 기본 상태 문구 대신 임시로 보여줄 문구
	 */
	FText mStatusOverrideText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EnterRoomButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CloseButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EnterButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapStatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapPreviewTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapPreviewDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapPreviewStateText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MapPreviewPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> MapGraphSize;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> MapGraphCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> MapScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MapLegendScroll;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MapLegendList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MapLegendTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MapDimBackground;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MapPaperPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MapPaperShadow;

	/*
	 * 아래 Map_* 위젯들은 concept 시안 빌더(_sync_worldmap_from_concept.py)가 WBP_FrontendMap에 생성/배치한다.
	 * 과거 C++ 런타임 생성(배경/범례)을 WBP 소유로 이관한 것 — C++은 동적 크기/위치 동기만 한다.
	 */

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_ParchmentBody;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_ScrollRodTop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_ScrollRodBottom;

	/** @brief 노드 배치 경계 마커(투명) — 앵커 X 분수 + 오프셋 Top/Bottom px가 정본 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Map_NodeArea;

	/** @brief 노드 크기(W)/행 간격(H) 마커 SizeBox(Collapsed) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Map_NodeMetrics;

	/** @brief 열 간격 상한(W) 마커 SizeBox(Collapsed) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> Map_ColPitch;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_CurrentMarker;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_SelectGlow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Map_StageNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Map_StageProgressText;

	/**
	 * @brief 연결선 하나를 만들 때 사용할 WBP 클래스
	 *
	 * @details
	 * WBP_FrontendMap Class Defaults에서 WBP_FrontendMapLine으로 지정한다.
	 * 부모 맵은 위치/길이/각도만 계산하고, 선 모양은 이 WBP가 가진다.
	 *
	 * 왜 C++ 기본 경로를 두지 않는가:
	 * 선 모양은 WBP 자산의 책임이다. C++이 특정 WBP 경로를 직접 들고 있으면 자산 이름/위치 변경이 코드 수정으로 번진다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<UFrontendMapLineWidget> MapLineWidgetClass;

	/**
	 * @brief 방 노드 하나를 만들 때 사용할 WBP 클래스
	 *
	 * @details
	 * WBP_FrontendMap Class Defaults에서 WBP_FrontendMapNode로 지정한다.
	 * 방 개수가 늘어나도 C++에서 Button/TextBlock을 직접 만들지 않고 이 WBP를 재사용한다.
	 *
	 * 왜 C++ 기본 경로를 두지 않는가:
	 * 노드의 실제 위젯 구성은 WBP에서 바꿀 수 있어야 한다. C++은 선택/배치 데이터만 넘기고 자산 경로는 WBP 설정에 맡긴다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<UFrontendMapNodeWidget> MapNodeWidgetClass;

	UPROPERTY(Transient)
	TArray<FFrontendMapLinePoolEntry> mMapLinePool;

	UPROPERTY(Transient)
	TArray<FFrontendMapNodePoolEntry> mMapNodePool;

	/**
	 * @brief 방 입장 요청 이후 중복 입력을 막기 위한 상태
	 */
	bool mEnterRequested = false;

	/**
	 * @brief 이 지도 화면에서 다음 방 선택을 허용할지 여부
	 *
	 * @details
	 * MAP 버튼으로 연 지도는 조회 전용이고, 전투 승리 후 열린 지도만 다음 방 선택과 입장을 허용한다.
	 *
	 * 왜 별도 플래그가 필요한가:
	 * 노드가 Ready 상태라고 해서 사용자가 항상 선택할 수 있으면, 단순히 경로를 확인하려고 연 MAP 화면에서도
	 * 방 전환 API가 호출될 수 있다. 표시 상태와 입력 허용 상태를 분리해 그런 실수를 막는다.
	 */
	bool mRoomSelectionEnabled = false;

	bool mMapDragScrolling = false;

	FVector2D mMapDragLastScreenPosition = FVector2D::ZeroVector;

	/**
	 * @brief 마지막 RefreshMap이 확인한 지도 행 수(그래프 높이 계산용)
	 *
	 * @details
	 * 그래프 높이는 "행 수 x 행 간격"이 정본이라 데이터 이후에만 정확하다. 데이터 이전(NativeConstruct)에는 폴백 비율을 쓴다.
	 */
	int32 mCachedRowCount = 0;
};
