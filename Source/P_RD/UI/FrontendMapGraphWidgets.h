/*****************************************************************//**
 * @file   FrontendMapGraphWidgets.h
 * @brief  월드맵 그래프 반복 요소 WBP 위젯 정의 헤더
 * @author Codex
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

#include "FrontendMapGraphWidgets.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFrontendMapNodeClickedDelegate, int32, RowIndex, int32, ColumnIndex);

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

	int32 RowIndex = INDEX_NONE;
	int32 ColumnIndex = INDEX_NONE;
	bool bClickBound = false;
};

/**
 * @brief 월드맵 방 사이를 잇는 선 하나를 표시하는 WBP 위젯
 *
 * @details
 * 지도 연결선의 위치, 길이, 각도는 맵 데이터에 따라 매번 달라진다.
 * 그래서 부모 UFrontendMapWidget이 CanvasPanelSlot 값은 계산하지만,
 * 선의 실제 모양은 WBP_FrontendMapLine 안에 둔다.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapLineWidget : public UUserWidget
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
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UFrontendMapNodeWidget : public UUserWidget
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
		const FSlateColor& BadgeColor);

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

	int32 RowIndex = INDEX_NONE;
	int32 ColumnIndex = INDEX_NONE;
};
