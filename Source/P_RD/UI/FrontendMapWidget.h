/*****************************************************************//**
 * @file   FrontendMapWidget.h
 * @brief  프론트/런 공용 월드맵 화면 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Frontend/FrontendViewTypes.h"
#include "UI/RDUserWidget.h"

#include "FrontendMapWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UFrontendMapLineWidget;
class UFrontendMapNodeWidget;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFrontendMapWidgetSimpleEvent);

USTRUCT()
struct FFrontendMapLinePoolEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UFrontendMapLineWidget> LineWidget;
};

USTRUCT()
struct FFrontendMapNodePoolEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UFrontendMapNodeWidget> NodeWidget;
};

/**
 * @brief 월드맵 화면 본체
 *
 * @details
 * 모달 표시/닫기 책임은 바깥 오버레이가 갖고, 이 클래스는 맵 그래프/선택/입장 UI만 관리한다.
 * 지도 데이터는 직접 생성하지 않고 AFrontendGameMode::GetMapRoomViews()가 내려준 View DTO를 그린다.
 *
 * @note UI 파트 추가 위젯
 * feature/create-srpg-framework-base 브랜치에는 FStage/FRoom과 방 전환 API가 있지만,
 * 이를 타이틀 화면에서 읽기 전용 월드맵으로 그리는 UMG 위젯은 없어서 UI/map 브랜치에서 추가했다.
 * 방 선택/입장 판단은 GameMode API로 돌려보내며, 실제 전환은 PM 브랜치의 RoomTransitionSubsystem 흐름을 탄다.
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
	UFrontendMapWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 현재 RunPersistData 기반 지도 View를 다시 가져와 노드/선을 갱신한다.
	 * @return 지도 데이터가 있어 갱신에 성공하면 true
	 *
	 * @note UI 파트 추가 API
	 * PM 브랜치의 FStage/FRoom을 직접 만드는 함수가 아니라, FrontendGameMode가 변환해준
	 * FFrontendMapRoomView 배열을 화면에 배치하는 표시 전용 함수다.
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool RefreshMap();

	/** @brief 승리 후 지도처럼 다음 방 선택이 허용되는 상황에서만 true로 둔다. */
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
	 * @brief 외부에서 지정한 상태 문구를 해제하고 지도 기본 상태 문구로 되돌린다.
	 */
	void ClearMapStatusOverride();

	UPROPERTY(Category = UI, BlueprintAssignable)
	FFrontendMapWidgetSimpleEvent OnCloseRequested;

	/* UUserWidget 상속 */
protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

private:
	void RefreshLocalizedTextCache();
	void ValidateDesignerBindings() const;
	void BindEvents();
	void UnbindEvents();
	void HandleMapRoomClicked(int32 RowIndex, int32 ColumnIndex);
	void SetMapStatusText(const FText& InText) const;
	void SetEnterButtonText(const FText& InText) const;
	void SetMapPreviewText(const FText& Title, const FText& Description, const FText& State, const FSlateColor& StateColor) const;
	void HideUnusedMapTextSurfaces() const;
	bool IsFrontendMapNavigationEnabled() const;
	void ConfigureMapGraphLayout() const;
	FFrontendMapLinePoolEntry* AcquireMapLineWidget(int32 LineIndex);
	FFrontendMapNodePoolEntry* AcquireMapNodeWidget(int32 NodeIndex);
	void HideUnusedMapGraphWidgets(int32 UsedLineCount, int32 UsedNodeCount);

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleEnterRoomButtonClicked();

	UFUNCTION()
	void HandleMapNodeClicked(int32 RowIndex, int32 ColumnIndex);

private:
	FText mMapText;
	FText mCloseText;
	FText mEnterText;
	FText mLoadingStatusText;
	FText mMapReadyStatusText;
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

	/**
	 * @brief 연결선 하나를 만들 때 사용할 WBP 클래스
	 *
	 * @details
	 * WBP_FrontendMap Class Defaults에서 WBP_FrontendMapLine으로 지정한다.
	 * 부모 맵은 위치/길이/각도만 계산하고, 선 모양은 이 WBP가 가진다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<UFrontendMapLineWidget> MapLineWidgetClass;

	/**
	 * @brief 방 노드 하나를 만들 때 사용할 WBP 클래스
	 *
	 * @details
	 * WBP_FrontendMap Class Defaults에서 WBP_FrontendMapNode로 지정한다.
	 * 방 개수가 늘어나도 C++에서 Button/TextBlock을 직접 만들지 않고 이 WBP를 재사용한다.
	 */
	UPROPERTY(Category = "Frontend Map", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<UFrontendMapNodeWidget> MapNodeWidgetClass;

	UPROPERTY(Transient)
	TArray<FFrontendMapLinePoolEntry> MapLinePool;

	UPROPERTY(Transient)
	TArray<FFrontendMapNodePoolEntry> MapNodePool;

	/**
	 * @brief 방 입장 요청 이후 중복 입력을 막기 위한 상태
	 */
	bool bEnterRequested = false;

	/**
	 * @brief 이 지도 화면에서 다음 방 선택을 허용할지 여부
	 *
	 * @details
	 * MAP 버튼으로 연 지도는 조회 전용이고, 전투 승리 후 열린 지도만 다음 방 선택과 입장을 허용한다.
	 */
	bool bRoomSelectionEnabled = false;
};
