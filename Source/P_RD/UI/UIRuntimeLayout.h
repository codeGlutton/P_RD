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
class UWidgetTree;

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

	/**
	 * @brief WBP 디자이너가 배치한 명명 앵커 위젯에서 정규화(0~1) 영역을 읽습니다.
	 *
	 * @details
	 * 디자인은 코드가 아니라 WBP에 둡니다. WBP에 (0,0) 점앵커 + 디자인좌표 오프셋/크기로 둔
	 * 앵커 위젯(예: HUD_SkillRail)을 찾아, 그 위치/크기를 DesignSize로 나눠 정규화 영역을 돌려줍니다.
	 * 위젯이 없거나 CanvasPanelSlot이 아니면 Fallback을 그대로 돌려줍니다(앵커 없으면 기존 코드 좌표 유지).
	 *
	 * @param WidgetTree 탐색할 위젯 트리.
	 * @param AnchorWidgetName 찾을 앵커 위젯 이름.
	 * @param DesignSize 앵커 위젯 좌표의 기준 캔버스 크기(예: 1920x1080).
	 * @param Fallback 앵커가 없을 때 돌려줄 정규화 영역.
	 * @return 정규화된 (좌,상,우,하) 영역.
	 */
	P_RD_API FAnchors GetDesignerGroupRect(UWidgetTree* WidgetTree, FName AnchorWidgetName, const FVector2D& DesignSize, const FAnchors& Fallback);

	/**
	 * @brief 명명 마커 위젯의 캔버스 슬롯 데이터(앵커+오프셋+정렬)를 그대로 읽습니다.
	 *
	 * @details
	 * 엣지 피닝 스킨의 정본 좌표입니다. 마커가 (ax,ay) 점앵커 + 상대 디자인px 오프셋을 갖고,
	 * 런타임 위젯이 이 데이터를 복사하면 마커와 같은 화면 위치에 붙습니다(레거시 (0,0) 점앵커에도 동일 동작).
	 *
	 * @return 마커가 없거나 CanvasPanelSlot이 아니면 false(OutSlotData 불변).
	 */
	P_RD_API bool GetDesignerSlotData(UWidgetTree* WidgetTree, FName MarkerName, FAnchorData& OutSlotData);

	/**
	 * @brief 정규화(0~1) 영역을 (0,0) 점앵커 + 디자인px 오프셋 슬롯으로 변환합니다.
	 *
	 * @details
	 * 마커가 없을 때 스킨 경로의 fallback으로 씁니다. 좌상단 고정이라 16:9에서 기존과 동일하며,
	 * 와이드에서도 화면 안에 남습니다(레거시 뷰포트 정규화를 fill 캔버스에 그대로 쓰면 안 됨).
	 */
	P_RD_API FAnchorData NormalizedToDesignPointSlot(const FAnchors& Normalized, const FVector2D& DesignSize);

	/** @brief 마커 슬롯 데이터를 읽되, 없으면 정규화 fallback을 디자인px 점앵커 슬롯으로 변환해 돌려줍니다. */
	P_RD_API FAnchorData GetDesignerSlotDataOr(UWidgetTree* WidgetTree, FName MarkerName, const FAnchors& FallbackNormalized, const FVector2D& DesignSize);

	/** @brief 슬롯 데이터(앵커/오프셋/정렬)를 런타임 위젯에 그대로 적용합니다(ZOrder 별도). */
	P_RD_API void ApplyDesignerSlotData(UWidget* Widget, const FAnchorData& SlotData, int32 ZOrder);

	/**
	 * @brief 점앵커 그룹 슬롯 안에서 세로 Index/Count 분배 셀 슬롯을 만듭니다.
	 *
	 * @details
	 * 오프셋은 디자인px(1080 세로 기준)이라 뷰포트와 무관합니다 — 리사이즈 재계산이 필요 없습니다.
	 * 셀 높이 = (그룹높이 - (Count-1)*GapPx) / Count. 앵커/가로/정렬은 그룹 슬롯을 그대로 상속합니다.
	 */
	P_RD_API FAnchorData MakeVerticalSubSlot(const FAnchorData& GroupSlot, int32 Index, int32 Count, float GapPx);

	/**
	 * @brief 이름 접두사로 캔버스 위젯들의 점앵커 슬롯을 모아 세로(Top) 순으로 반환한다.
	 *
	 * @details
	 * 스킨이 반복 아트(예: 스킬 레일 프레임 6칸)를 개별 위젯으로 굽는 경우, 마커 등분 계산 대신
	 * 프레임 위젯 자신의 슬롯을 정본으로 상속하기 위한 헬퍼다(등분-아트 어긋남 원천 차단).
	 */
	P_RD_API TArray<FAnchorData> CollectPointSlotsByPrefix(UWidgetTree* WidgetTree, const FString& NamePrefix);
}
