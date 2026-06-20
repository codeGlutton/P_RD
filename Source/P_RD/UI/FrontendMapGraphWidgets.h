/*****************************************************************//**
 * @file   FrontendMapGraphWidgets.h
 * @brief  월드맵 그래프 반복 요소 WBP 위젯 정의 헤더
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Components/Button.h"
#include "PCGStage/RoomType.h"
#include "UI/RDUserWidget.h"

#include "FrontendMapGraphWidgets.generated.h"

class UBorder;
class UButton;
class UTextBlock;

/**
 * @brief 지도 노드 클릭 시 행/열 좌표만 부모 위젯으로 전달하는 UI delegate
 *
 * 이 이벤트는 룸 데이터나 전환 API를 직접 실행하지 않는다.
 * UMG 노드 버튼이 사용자가 누른 행/열 좌표를 부모 지도 위젯으로 넘기기 위한 표시 계층 이벤트다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFrontendMapNodeClickedDelegate, int32, RowIndex, int32, ColumnIndex);

/**
 * @brief 지도 노드 좌표를 함께 보관하는 전용 Button
 *
 * UButton 자체는 어떤 방 좌표를 의미하는지 모르므로, 지도 노드용 버튼이 행/열 값을 함께 보관한다.
 * 클릭 시 게임 로직을 실행하지 않고 행/열 좌표만 broadcast한다.
 */
UCLASS()
class P_RD_API UFrontendMapNodeButton : public UButton
{
	GENERATED_BODY()

public:
	void SetNodeCoordinates(int32 InRowIndex, int32 InColumnIndex);

	UPROPERTY(Category = "Frontend Map", BlueprintAssignable)
	FFrontendMapNodeClickedDelegate OnMapNodeClicked;

private:
	UFUNCTION()
	void HandleClicked();

	int32 mRowIndex = INDEX_NONE;
	int32 mColumnIndex = INDEX_NONE;
	bool mClickBound = false;
};

/**
 * @brief 월드맵 방 사이를 잇는 선 하나를 표시하는 WBP 위젯
 *
 * @details
 * 지도 연결선의 위치, 길이, 각도는 맵 데이터에 따라 매번 달라진다.
 * 그래서 부모 UFrontendMapWidget이 CanvasPanelSlot 값은 계산하지만,
 * 선의 실제 모양은 WBP_FrontendMapLine 안에 둔다.
 *
 * Stage 연결 정보를 화면 선으로 표현하기 위한 표시 전용 WBP 베이스다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapLineWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 선 색상을 현재 방 상태에 맞게 바꿈 */
	void SetLineColor(const FLinearColor& InColor);

protected:
	void NativeConstruct() override;

private:
	/** @brief WBP 안에서 실제 선 색상을 받는 Border */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LinePanel;
};

/**
 * @brief 월드맵 방 노드 하나를 표시하는 WBP 위젯
 *
 * @details
 * 이 위젯은 방 하나의 버튼/색/문구만 담당한다.
 * 어떤 방으로 이동 가능한지, 선택하면 어떤 런 데이터가 바뀌는지는 부모 UFrontendMapWidget과 GameMode가 처리한다.
 *
 * FRoom을 대체하지 않는다. FMapRoomView 한 개를 화면 노드 하나로 보여주는 UMG 베이스다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapNodeWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief WBP 노드에 표시할 좌표와 시각 상태를 넣음 */
	void SetNodeVisual(
		int32 InRowIndex,
		int32 InColumnIndex,
		const FText& Label,
		const FText& Badge,
		const FLinearColor& PanelColor,
		const FLinearColor& TypeStripeColor,
		const FSlateColor& LabelColor,
		const FSlateColor& BadgeColor,
		ERoomType RoomType = ERoomType::None);

	/** @brief 부모 위젯이 현재 노드의 버튼 활성 여부를 정할 때 사용함 */
	void SetNodeEnabled(bool bEnabled) const;

	/** @brief 노드를 눌렀을 때 부모 맵 위젯으로 행/열 좌표를 보냄 */
	UPROPERTY(Category = "Frontend Map", BlueprintAssignable)
	FFrontendMapNodeClickedDelegate OnMapNodeClicked;

protected:
	void NativeConstruct() override;
	void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleNodeButtonClicked();

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NodeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> NodePanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> NodeTypeStripe;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NodeLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NodeBadgeText;

	int32 mRowIndex = INDEX_NONE;
	int32 mColumnIndex = INDEX_NONE;
};
