#pragma once

/**
 * @file UIRuntimeLayout.h
 * @brief 코드로 만든 UI의 위치와 크기를 맞추는 함수들입니다.
 *
 * @details
 * WBP에서 배치한 UI는 그대로 두고, 실행 중에 코드가 위치를 잡는 위젯만 여기서 정리합니다.
 * Anchor, Size, ZOrder 설정을 여러 파일에 반복해서 쓰지 않기 위한 파일입니다.
 */

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

class UWidget;

/**
 * @brief CanvasPanel 위젯 배치 함수 모음입니다.
 *
 * @details
 * 위젯이 없거나 CanvasPanel에 붙은 위젯이 아니면 아무것도 하지 않습니다.
 * 위젯을 추가/삭제하는 일은 호출하는 쪽에서 합니다.
 */
namespace RDUILayout
{
	/**
	 * @brief 위젯의 CanvasPanelSlot을 가져옵니다.
	 *
	 * @details
	 * 위젯이 없거나 CanvasPanel에 붙어 있지 않으면 nullptr를 돌려줍니다.
	 *
	 * @param Widget 확인할 위젯.
	 * @return CanvasPanelSlot이면 해당 포인터, 아니면 nullptr.
	 */
	P_RD_API UCanvasPanelSlot* GetCanvasSlot(UWidget* Widget);

	/**
	 * @brief 위젯을 지정한 Anchor 영역에 꽉 채웁니다.
	 *
	 * @details
	 * 배경이나 큰 패널처럼 화면 크기에 맞춰 늘어나야 하는 위젯에 씁니다.
	 *
	 * @param Widget 배치할 위젯. CanvasPanelSlot이 아니면 아무 작업도 하지 않습니다.
	 * @param Anchors 채울 영역.
	 * @param ZOrder 앞뒤 순서.
	 */
	P_RD_API void ApplyAnchoredSlot(UWidget* Widget, const FAnchors& Anchors, int32 ZOrder);

	/**
	 * @brief 위젯을 정해진 위치와 크기로 배치합니다.
	 *
	 * @details
	 * 버튼, 카드, 작은 패널처럼 크기가 정해진 위젯에 사용합니다.
	 *
	 * @param Widget 배치할 위젯. CanvasPanelSlot이 아니면 아무 작업도 하지 않습니다.
	 * @param Anchors 위치 기준.
	 * @param Alignment 위젯 안에서 어느 지점을 기준으로 둘지.
	 * @param Position 위치.
	 * @param Size 크기.
	 * @param ZOrder 앞뒤 순서.
	 */
	P_RD_API void ApplyFixedSlot(UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);

	/**
	 * @brief 위젯을 화면 중앙 기준으로 배치합니다.
	 *
	 * @details
	 * 팝업이나 중앙 오버레이처럼 가운데에 놓는 UI에 사용합니다.
	 *
	 * @param Widget 배치할 위젯. CanvasPanelSlot이 아니면 아무 작업도 하지 않습니다.
	 * @param Position 화면 중앙 기준 위치.
	 * @param Size 크기.
	 * @param ZOrder 앞뒤 순서.
	 */
	P_RD_API void ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);
}
