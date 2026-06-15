#pragma once

/**
 * @file UIRuntimeLayout.h
 * @brief CanvasPanel에 붙은 보조 위젯의 위치와 크기를 맞추는 헬퍼입니다.
 *
 * @details
 * 주 UI는 WBP에서 구성하고, 여기서는 실행 중 붙거나 위치가 갱신되는 보조 위젯의
 * CanvasPanelSlot 값만 정리합니다. Anchor, Size, ZOrder 설정 코드가 여러 UI 클래스에
 * 퍼지지 않게 한곳에 모았습니다.
 *
 * 이 헬퍼의 의도는 레이아웃 정책을 새로 만드는 것이 아니라, 런타임 구성 UI가 CanvasPanelSlot을
 * 직접 만지는 반복 코드를 줄이는 것입니다. WBP가 이미 관리하는 정적 배치는 그대로 두고,
 * 코드가 생성하거나 재배치하는 보조 패널만 여기 함수를 통해 같은 방식으로 배치합니다.
 */

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

class UWidget;

/**
 * @brief CanvasPanel에 올라간 위젯을 배치하는 공통 함수 모음입니다.
 *
 * @details
 * 위젯이 없거나 CanvasPanel 안에 붙은 위젯이 아니면 그냥 넘어갑니다.
 * 호출하는 쪽에서 매번 null 검사와 슬롯 검사를 반복하지 않게 하기 위해서입니다.
 *
 * @note 이 namespace는 위젯을 부모 패널에 추가하거나 제거하지 않습니다. 이미 CanvasPanel에 붙은
 * 위젯의 slot 속성만 바꾸므로, AddChild/RemoveFromParent 책임은 호출부에 남아 있습니다.
 */
namespace RDUILayout
{
	/**
	 * @brief 위젯에서 CanvasPanelSlot을 꺼냅니다.
	 *
	 * @details
	 * null이면 위젯이 없거나 CanvasPanel 안에 있지 않은 상태입니다.
	 * 그 경우에는 배치를 바꾸지 않고 그대로 둡니다.
	 *
	 * @param Widget CanvasPanel에 붙어 있을 것으로 기대하는 위젯.
	 * @return Widget의 슬롯이 UCanvasPanelSlot이면 해당 포인터, 아니면 nullptr.
	 */
	P_RD_API UCanvasPanelSlot* GetCanvasSlot(UWidget* Widget);

	/**
	 * @brief 위젯을 지정한 Anchor 영역에 꽉 채웁니다.
	 *
	 * @details
	 * 화면 비율에 맞춰 늘어나야 하는 배경, 터치 영역, 큰 패널에 사용합니다.
	 * 이전에 설정된 Alignment 값이 섞이지 않도록 기본값으로 되돌립니다.
	 *
	 * @param Widget 배치할 위젯. CanvasPanelSlot이 아니면 아무 작업도 하지 않습니다.
	 * @param Anchors 채울 Canvas anchor 범위.
	 * @param ZOrder 같은 CanvasPanel 안에서의 그리기/입력 우선순위.
	 *
	 * @note Offset을 0으로 초기화하므로, Anchor 범위 전체를 채우는 용도에 맞습니다.
	 */
	P_RD_API void ApplyAnchoredSlot(UWidget* Widget, const FAnchors& Anchors, int32 ZOrder);

	/**
	 * @brief 위젯을 정해진 위치와 크기로 배치합니다.
	 *
	 * @details
	 * Position, Size, Alignment, Anchor, ZOrder를 같이 설정합니다.
	 * 버튼, 카드, 작은 패널처럼 크기가 정해진 위젯에 사용합니다.
	 *
	 * @param Widget 배치할 위젯. CanvasPanelSlot이 아니면 아무 작업도 하지 않습니다.
	 * @param Anchors 위치 계산 기준이 되는 Canvas anchor.
	 * @param Alignment Position을 해석할 때 사용할 위젯 내부 기준점.
	 * @param Position Anchor 기준 위치.
	 * @param Size 고정 크기.
	 * @param ZOrder 같은 CanvasPanel 안에서의 그리기/입력 우선순위.
	 */
	P_RD_API void ApplyFixedSlot(UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);

	/**
	 * @brief 위젯을 화면 중앙 기준으로 배치합니다.
	 *
	 * @details
	 * 팝업이나 중앙 오버레이처럼 가운데에 놓는 UI에 사용합니다.
	 * 호출부에서 Anchor와 Alignment 값을 다시 쓰지 않아도 됩니다.
	 *
	 * @param Widget 배치할 위젯. CanvasPanelSlot이 아니면 아무 작업도 하지 않습니다.
	 * @param Position 화면 중앙 anchor 기준 위치 보정값.
	 * @param Size 고정 크기.
	 * @param ZOrder 같은 CanvasPanel 안에서의 그리기/입력 우선순위.
	 */
	P_RD_API void ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);
}
