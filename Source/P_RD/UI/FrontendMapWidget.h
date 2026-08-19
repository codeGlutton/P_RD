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
class UMaterialInterface;
class URetainerBox;
class UVerticalBox;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;
class URunOptionsRailWidget;

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
	virtual bool RefreshMap();

	/**
	 * @brief 비Shipping 오프스크린 캡처에서 GameMode 전환 없이 복구된 런 View를 그린다.
	 *
	 * 실제 게임 화면은 계속 RoomGameMode의 View만 사용한다. 이 오버라이드는 개발 캡처 명령이
	 * 격리 세이브의 Stage를 읽어 동일한 노드/선 렌더러를 검증할 때만 설정한다.
	 */
	void SetPreviewRoomsForDebug(const TArray<FMapRoomView>& InRooms, bool bInAtStageStart);
	void ClearPreviewRoomsForDebug();

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

#if WITH_DEV_AUTOMATION_TESTS
	/** @brief 지도 생명주기가 공용 설정바 WBP도 만들었는지 자동화에서 확인한다. */
	URunOptionsRailWidget* GetRunOptionsRailForTest() const
	{
		return mRunOptionsRailWidget;
	}
#endif

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
	/** @brief 새 가로형 지도 WBP가 기존 세로 지도와 같은 진행 흐름만 공유할 때 true다. */
	virtual bool IsLandscapeLayout() const { return false; }

	/** @brief 파생 지도 클래스가 자신 전용 노드/선 WBP를 지정한다. */
	void SetMapGraphWidgetClasses(
		TSubclassOf<UFrontendMapLineWidget> InLineWidgetClass,
		TSubclassOf<UFrontendMapNodeWidget> InNodeWidgetClass)
	{
		MapLineWidgetClass = InLineWidgetClass;
		MapNodeWidgetClass = InNodeWidgetClass;
	}

	/** @brief 파생 지도 클래스가 직렬화된 연결선 WBP 기본값을 검증할 때 사용한다. */
	TSubclassOf<UFrontendMapLineWidget> GetMapLineWidgetClass() const
	{
		return MapLineWidgetClass;
	}

	/** @brief 파생 지도 클래스가 직렬화된 노드 WBP 기본값을 검증할 때 사용한다. */
	TSubclassOf<UFrontendMapNodeWidget> GetMapNodeWidgetClass() const
	{
		return MapNodeWidgetClass;
	}

	/**
	 * @brief WBP 바인딩과 버튼 이벤트를 연결한 뒤 현재 지도 데이터를 그린다.
	 */
	void NativeConstruct() override;

	/**
	 * @brief 버튼/노드 이벤트와 동적 노드/선 풀을 정리한다.
	 */
	void NativeDestruct() override;

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** @brief 지도 위젯에서 사용하는 기본 문구 캐시를 갱신한다. */
	void RefreshLocalizedTextCache();

	/**
	 * @brief WBP에 닫기 버튼이 없을 때 화면 우상단에 BACK 버튼을 만든다.
	 *
	 * @details 조회용 지도는 전투 HUD 위의 모달로 열리므로 바깥 MAP 버튼에
	 *          의존해 닫을 수 없다. 디자이너 버튼이 생기면 그 버튼을 그대로
	 *          쓰고, 현재 B안처럼 없는 자산에서만 런타임 안전망을 만든다.
	 */
	void EnsureCloseButton();
	/** @brief 전투 HUD와 동일한 지도·용병·몬스터·설정 레일을 지도 위에 얹는다. */
	void EnsureRunOptionsRail();
	/** @brief 지도 본체가 닫힐 때 별도 Viewport 레일도 함께 숨긴다. */
	void HandleMapVisibilityChanged(ESlateVisibility InVisibility);
	/** @brief 현재 지도 표시 상태를 별도 Viewport 레일에 즉시 반영한다. */
	void SyncRunOptionsRailVisibility(ESlateVisibility MapVisibility) const;

	/** @brief BACK 단추를 오른쪽 아래 구석에 판 그림 비율대로 배치한다. */
	void UpdateCloseButtonLayout() const;

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

	/** @brief 실기 HUD 재질에 맞춘 세로 양피지 보드와 장식 가시성을 적용한다. */
	void ApplyCurrentMapArt() const;

	/**
	 * @brief 스크롤 레이어를 RetainerBox로 감싸 화면 고정 원근 머티리얼을 건다.
	 *
	 * @details
	 * UMG는 affine 변환만 지원해 원근을 못 건다. 이미지에 원근을 구우면
	 * 스크롤 위치마다 착시가 깨지므로(내용에 원근이 고정됨), 지도 내용은
	 * 평평하게 그리고 리테이너 머티리얼(M_MapPerspective)이 화면 고정
	 * 사다리꼴로 왜곡한다. 노드/선/양피지가 한 렌더타겟에서 함께 왜곡된다.
	 */
	void InstallMapPerspectiveRetainer();

	/** @brief 화면(리테이너) UV를 원근 왜곡 이전의 콘텐츠 UV로 역변환한다. 사다리꼴 밖이면 X가 [0,1]을 벗어난다. */
	static FVector2D InverseMapPerspectiveUV(const FVector2D& ScreenUV);

	/** @brief 원근 지도 위 탭을 역변환해 가장 가까운 선택 가능 노드 클릭으로 처리한다. */
	bool TryHandleMapPerspectiveTap(const FVector2D& ScreenPosition);

	/**
	 * @brief 범례 아이콘 이미지가 물고 있는 옛 노드 아이콘을 v2 텍스처로 바꾼다.
	 *
	 * @details
	 * 범례 행은 시안 빌더가 WBP에 구웠고 에디터 파이썬으로는 WidgetTree 접근이
	 * 막혀 있어, 런타임에 옛 경로 키워드를 보고 교체한다. 노드 아이콘은
	 * WBP_FrontendMapNode 클래스 디폴트가 v2 를 직접 참조한다.
	 */
	void ApplyLegendIconsV2() const;

	/** @brief 지도 본문 위 베일(어둡게+채도낮춤)을 한 번 만들어 그래프 캔버스에 얹는다. */
	void EnsureParchmentVeil();

	/**
	 * @brief 톤 맞춘 범례 판 이미지를 WBP 범례 그룹 자리에 놓는다.
	 *
	 * @details
	 * 시안 범례는 프레임 + 아이콘/글자 행을 위젯으로 쌓아 만든 것인데, 새 범례는
	 * 글자까지 그려진 완성 이미지 한 장이다. 둘을 같이 켜면 글자가 겹치므로
	 * 기존 그룹을 접고 이미지로 대체한다.
	 */
	void EnsureLegendPlate();

	/** @brief 범례 판 크기를 화면에 맞춰 다시 잡는다(시안 고정 크기는 폰에서 너무 작다). */
	void UpdateLegendPlateLayout() const;

	/** @brief 범례 여닫기 단추를 왼쪽 아래에 만든다(없을 때만). */
	void EnsureLegendToggleButton();

	/** @brief 범례 펼침 상태를 판/단추 문구에 반영한다. */
	void ApplyLegendShownState() const;

	/** @brief 지도 단추(BACK/범례)의 글자 크기를 단추 높이에 맞춘다. */
	void ApplyMapButtonFontSize(UTextBlock* ButtonText, float ButtonHeight) const;

	/** @brief 범례 여닫기 단추 입력. */
	UFUNCTION()
	void HandleLegendToggleClicked();

	/** @brief 이번 갱신에 쓸 노드 크기. 확정값이 있으면 그것, 없으면 시안 크기. */
	FVector2D GetMapNodeSize() const;

	/** @brief 시안이 정한 노드 크기(마커 x 배율) — 좁은 화면에서 줄어들기 전의 상한. */
	float GetMapNodeDesignSize() const;

	/** @brief 이번 갱신의 노드 크기와 행 간격을 확정한다(겹침 방지 + 적은 층 세로 채움). */
	void ResolveNodeMetrics(const TArray<FMapRoomView>& Rooms);

	/** @brief 행 간격(시안 마커 Map_NodeMetrics 높이, 폴백 176) — 그래프 높이의 정본. */
	float GetMapRowPitch() const;

	/** @brief 열 간격 상한(시안 마커 Map_ColPitch 폭, 폴백 240) — 열이 적을 때 중앙으로 모아 간격 과대를 막는다. */
	float GetMapColPitchMax() const;

	/** @brief 노드 배치 경계(시안 마커 Map_NodeArea): 좌우는 앵커 분수, 상하는 px 오프셋. 화면 폭이 있으면 범례/좌우 장식 안전 여백을 반영한다. */
	void GetNodeAreaLayout(float& OutLeftFrac, float& OutRightFrac, float& OutTopPx, float& OutBottomPx, float InGraphWidth = 0.f) const;

	/** @brief Stage row/column을 시안 경계/간격 기반 캔버스 좌표로 변환한다. */
	FVector2D GetMapRoomNodeCenter(const TArray<FMapRoomView>& Rooms, const FMapRoomView& Room, const FVector2D& GraphSize) const;

	/** @brief 양피지/두루마리(WBP 소유)를 동적 그래프 크기에 슬롯만 동기한다. */
	void UpdateGraphDecorLayout(const FVector2D& GraphSize) const;

	/** @brief 현재 위치 마커/선택 글로우(WBP 소유)를 해당 노드 위치로 옮긴다. */
	void UpdateOverlayMarkers(const TMap<FIntPoint, FVector2D>& NodeCenters, const FIntPoint& CurrentCoord, const FIntPoint& SelectedCoord) const;


	/** @brief 지정 인덱스의 연결선 위젯을 풀에서 가져오거나 새로 만든다. */
	FFrontendMapLinePoolEntry* AcquireMapLineWidget(int32 LineIndex);

	/** @brief 지정 인덱스의 방 노드 위젯을 풀에서 가져오거나 새로 만든다. */
	FFrontendMapNodePoolEntry* AcquireMapNodeWidget(int32 NodeIndex);

	/** @brief 이번 RefreshMap()에서 쓰지 않은 노드/선을 숨겨 다음 갱신 때 재사용한다. */
	void HideUnusedMapGraphWidgets(int32 UsedLineCount, int32 UsedNodeCount);

	/** @brief 풀에 없는데 캔버스에 남아 있는 노드/선을 접는다(지도 두 벌 방지 안전장치). */
	void CollapseOrphanGraphWidgets() const;

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

	/** @brief 풀스크린 스크림(빌더 생성). 탑바 인셋 적용 대상. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_Scrim;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_ParchmentBody;

	/** @brief B안 흑단/황동 프레임과 세로 지형을 합친 스크롤 지도 본문 텍스처. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Map|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mMapParchmentTexture;

	/**
	 * @brief 지도 팝업 뒤 배경(정사각 2048).
	 *
	 * @details
	 * 기기마다 화면 비율이 달라 정사각 그림을 cover-fit(긴 변 기준)으로 깐다.
	 * 세로 화면에서는 좌우가, 가로 화면에서는 상하가 잘린다 — 단순한 그림이라
	 * 어디를 잘라도 성립한다.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Map|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mMapPopupBackgroundTexture;

	/** @brief 양피지 위 어둡게-채도낮춤 베일(런타임 생성). 노드 아래, 지도 위. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mMapParchmentVeil;

	/**
	 * @brief 지도 본문을 눌러 노드 아이콘을 띄우는 베일 색.
	 *
	 * @details
	 * 지도 그림이 노드 아이콘과 채도·명도가 비슷해 아이콘이 묻힌다. 곱하기
	 * 틴트는 어둡게만 할 뿐 채도를 못 낮추므로, 배경과 같은 네이비를 반투명으로
	 * 덮어 어둡게+채도낮춤을 한 번에 한다. 알파를 0으로 두면 원본 색이 된다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	FLinearColor mMapParchmentVeilColor = FLinearColor(0.043f, 0.055f, 0.098f, 0.f);

	/** @brief 지도 UI 버튼 판(BACK 단추 바탕). 비율 2.5:1 통짜 그림. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Map|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mMapButtonPlateTexture;

	/** @brief 톤 맞춘 범례 판(글자까지 그려진 완성 이미지). WBP 범례 행을 대체한다. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Map|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> mMapLegendTexture;

	/** @brief 범례 판 이미지(런타임 생성). Map_LegendGroup 자리에 대신 놓는다. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> mMapLegendPlate;

	/** @brief 범례 여닫기 단추(런타임 생성). 왼쪽 아래 구석. */
	UPROPERTY(Transient)
	TObjectPtr<UButton> mMapLegendToggleButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mMapLegendToggleText;

	UPROPERTY(Transient)
	TObjectPtr<URunOptionsRailWidget> mRunOptionsRailWidget;

	/**
	 * @brief 범례를 펴 놓았는지 여부. 기본은 접힘.
	 *
	 * @details
	 * 범례는 화면 위에 얹히는 판이라 어떤 화면 비율에서는 반드시 노드를 가린다
	 * (특히 정사각형에 가까운 폴드 비율). 자주 보는 정보가 아니므로 기본은 접어
	 * 두고 단추로 펴게 한다 -- 그러면 어떤 비율에서도 지도를 가리지 않는다.
	 */
	bool mMapLegendShown = false;

	/**
	 * @brief 노드 아이콘 크기 배율(폰 가독성) — 이 값은 상한일 뿐이다.
	 *
	 * @details
	 * 기본 크기의 정본은 시안 마커(Map_NodeMetrics)지만, 그 마커는 WBP 안에
	 * 있어 에디터 파이썬으로 건드릴 수 없다(WidgetTree 미노출). 폰에서 아이콘이
	 * 너무 작아 종류가 구분되지 않아 코드 쪽 배율로 키운다.
	 *
	 * 실제 크기는 ResolveNodeMetrics()가 행/열 간격 안에 들어가도록 다시 줄이므로,
	 * 이 값을 키워도 겹치지 않는다. 화면이 넓을 때 얼마나 커질 수 있는지의 상한이다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	float mMapNodeSizeScale = 2.2f;

	/**
	 * @brief 이번 갱신에서 확정한 아이콘 크기(px). 0이면 아직 미확정.
	 *
	 * @details
	 * 열 간격에서 파생한 값이라 방 데이터(최대 열)와 화면 폭을 알아야 정해진다.
	 * RefreshMap이 매번 다시 계산하고, GetMapNodeSize()가 이 값을 돌려준다.
	 */
	UPROPERTY(Transient)
	float mResolvedNodeSize = 0.f;

	/** @brief 이번 갱신에서 확정한 행 간격(px). 0이면 시안 마커 값을 쓴다. */
	UPROPERTY(Transient)
	float mResolvedRowPitch = 0.f;

	/**
	 * @brief 눕힌 원근(리테이너 왜곡)을 쓸지 여부. 기본은 끔.
	 *
	 * @details
	 * 책상 배경을 버리고 팝업으로 바꾸면서 "책상 위에 눕힌 지도"라는 기울임의
	 * 근거가 사라졌다. 끄면 탭 좌표 역변환(InverseMapPerspectiveUV) 경로도 같이
	 * 빠져 입력이 단순해진다. 다시 기울이고 싶으면 이 값만 켜면 된다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	bool mUseMapPerspective = false;

	/** @brief 화면 고정 원근 UI 머티리얼(M_MapPerspective). 리테이너 이펙트로 쓴다. */
	/** @brief 룸 아이콘 v2(범례 교체용). 키는 옛 텍스처 경로에서 찾는 종류 키워드. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> mMapIconV2ByKeyword;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Map|Art", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> mMapPerspectiveMaterial;

	/** @brief 원근 왜곡 리테이너(런타임 생성). 스크롤 레이어를 감싼다. */
	UPROPERTY(Transient)
	TObjectPtr<URetainerBox> mMapPerspectiveRetainer;

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

	/** @brief 가로형 지도에서 현재 파티 초상 뒤에 까는 청색 오라. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_CurrentAura;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Map_SelectGlow;

	/** @brief 범례 그룹(프레임+행, 빌더 생성). 좁은 화면에서 통째로 렌더 스케일 축소한다. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Map_LegendGroup;


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

	/**
	 * @brief 지도 콘텐츠(스크림+스크롤)를 위에서 내리는 인셋(px, 디자인 1080 기준)
	 *
	 * @details
	 * 전투 HUD 탑바(상태바/방 이름/내비 버튼)가 지도 위에 계속 보이게 하기 위한 값이다.
	 * 인셋 영역은 이 위젯이 아무것도 그리지 않아 히트테스트가 통과하고, 탑바 버튼을 그대로 누를 수 있다.
	 * 값은 concept 시안(wbpNativeSpec.topUIInset)이 정본이며 빌더가 WBP Class Defaults로 주입한다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	float mTopUIInset = 0.f;

	/** @brief 범례 축소 기준 폭(시안 legendScale.refWidth). 디자인 폭이 이보다 좁으면 비례 축소. */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	float mLegendRefWidth = 1920.f;

	/** @brief 범례 축소 하한(시안 legendScale.minScale). */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	float mLegendMinScale = 0.5f;

	UPROPERTY(Transient)
	TArray<FFrontendMapLinePoolEntry> mMapLinePool;

	UPROPERTY(Transient)
	TArray<FFrontendMapNodePoolEntry> mMapNodePool;

	/** @brief RD.MapPreview.Data가 공급한 격리 캡처용 View. 일반 게임에서는 비어 있다. */
	UPROPERTY(Transient)
	TArray<FMapRoomView> mDebugPreviewRooms;

	bool mUseDebugPreviewRooms = false;
	bool mDebugPreviewAtStageStart = false;

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

	/** @brief 드래그 시작 위치 — 이동량이 작으면 탭으로 보고 노드 클릭을 시도한다. */
	FVector2D mMapDragStartScreenPosition = FVector2D::ZeroVector;

	/** @brief 탭 역변환용 노드 중심 캐시(좌표, 콘텐츠 px, 선택 가능 여부). RefreshMap마다 갱신. */
	struct FMapTapTarget
	{
		FIntPoint mCoord = FIntPoint(INDEX_NONE, INDEX_NONE);
		FVector2D mCenter = FVector2D::ZeroVector;
		bool mSelectable = false;
	};
	TArray<FMapTapTarget> mMapTapTargets;

	/**
	 * @brief 화면 상단에서의 지도 폭 비율(하단 1.0 기준 사다리꼴).
	 *
	 * @details
	 * M_MapPerspective의 TopWidth 파라미터와 탭 좌표 역변환이 같은 값을
	 * 써야 그림과 입력이 일치한다. C++이 정본이고 MID에 주입한다.
	 */
	static constexpr float MapPerspectiveTopWidth = 0.85f;

	/** @brief 뷰포트 변화 감지용 마지막 크기 — 창 리사이즈/회전 시 그래프를 다시 깐다(라이브 리사이즈 재배치). */
	FVector2D mLastViewportSize = FVector2D::ZeroVector;

	/** @brief 리사이즈 후 레이아웃 안정 대기 프레임 카운트다운(0이 되면 RefreshMap). */
	int32 mResizeRefreshCountdown = 0;

	/**
	 * @brief 마지막 RefreshMap이 확인한 지도 행 수(그래프 높이 계산용)
	 *
	 * @details
	 * 그래프 높이는 "행 수 x 행 간격"이 정본이라 데이터 이후에만 정확하다. 데이터 이전(NativeConstruct)에는 폴백 비율을 쓴다.
	 */
	int32 mCachedRowCount = 0;
};
