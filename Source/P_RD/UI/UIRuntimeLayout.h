#pragma once

/**
 * @file UIRuntimeLayout.h
 * @brief 코드로 만든 UI의 위치와 크기를 맞추는 헬퍼입니다.
 *
 * 런타임에 위젯을 만들면 Anchor, Size, ZOrder 같은 값을 매번 직접 넣어야 합니다.
 * 같은 설정 코드가 여러 UI 클래스에 퍼지지 않게 여기서 한 번에 처리합니다.
 */

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

class UWidget;

/**
 * @brief CanvasPanel에 올라간 위젯을 배치하는 공통 함수 모음입니다.
 *
 * 위젯이 없거나 CanvasPanel 위젯이 아니면 그냥 넘어갑니다.
 * 호출하는 쪽에서 매번 null 검사와 슬롯 검사를 반복하지 않게 하기 위해서입니다.
 */
namespace RDUILayout
{
	/**
	 * @brief 위젯에서 CanvasPanelSlot을 꺼냅니다.
	 *
	 * null이면 위젯이 없거나 CanvasPanel 안에 있지 않은 상태입니다.
	 * 그 경우에는 배치를 바꾸지 않고 그대로 둡니다.
	 */
	P_RD_API UCanvasPanelSlot* GetCanvasSlot(UWidget* Widget);

	/**
	 * @brief 위젯을 지정한 Anchor 영역에 꽉 채웁니다.
	 *
	 * 화면 비율에 맞춰 늘어나야 하는 배경, 터치 영역, 큰 패널에 사용합니다.
	 * 이전에 설정된 Alignment 값이 섞이지 않도록 기본값으로 되돌립니다.
	 */
	P_RD_API void ApplyAnchoredSlot(UWidget* Widget, const FAnchors& Anchors, int32 ZOrder);

	/**
	 * @brief 위젯을 정해진 위치와 크기로 배치합니다.
	 *
	 * Position, Size, Alignment, Anchor, ZOrder를 같이 설정합니다.
	 * 버튼, 카드, 작은 패널처럼 크기가 정해진 위젯에 사용합니다.
	 */
	P_RD_API void ApplyFixedSlot(UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);

	/**
	 * @brief 위젯을 화면 중앙 기준으로 배치합니다.
	 *
	 * 팝업이나 중앙 오버레이처럼 가운데에 놓는 UI에 사용합니다.
	 * 호출부에서 Anchor와 Alignment 값을 다시 쓰지 않아도 됩니다.
	 */
	P_RD_API void ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);
}
