/*****************************************************************//**
 * @file   FrontendMapWidget.h
 * @brief  프론트/런 공용 월드맵 화면 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-02
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/FrontendViewTypes.h"

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
 * 방 선택/입장 판단은 GameMode API로 돌려보내며, 실제 전환은 PM 브랜치의 RoomGameModeBase 흐름을 탄다.
 *
 * 현재 노드 안에 보이는 "행-열 + 룸 종류" 텍스트는 지도 데이터 배치와 RoomType 매핑을 확인하기 위한
 * 디버깅용 표시다. 최종 UI에서는 RoomType별 아이콘/이미지를 WBP 또는 공식 Stage/Room 표시 데이터에서
 * 받아 노드 위젯에 넣고, 이 디버그 텍스트는 제거하는 방향이다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapWidget : public UUserWidget
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

	bool bEnterRequested = false;
};
