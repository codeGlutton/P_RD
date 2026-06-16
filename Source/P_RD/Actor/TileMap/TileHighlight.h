/*****************************************************************//**
 * @file   TileHighlight.h
 * @brief  SRPG의 타일 강조 표시 정의 헤더
 * @author 이문환
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "TileHighlight.generated.h"

/**
 * @brief  타일 시각 강조 상태 (한 타일이 여러 상태를 동시에 가질 수 있어 비트플래그)
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETileHighlightFlag : uint8
{
	None	= 0			UMETA(Hidden),
	Aim		= 1 << 0	UMETA(ToolTip = "조준 범위"),
	Select	= 1 << 1	UMETA(ToolTip = "선택된 타일"),
	Effect	= 1 << 2	UMETA(ToolTip = "영향 범위"),
};
ENUM_CLASS_FLAGS(ETileHighlightFlag)

/**
 * @brief  타일 강조 상태 1종을 어떻게 그릴지 정의하는 스타일
 * @details
 * 상태(Aim/Select/Effect)마다 1개씩 타일맵이 보유하는 설정값.
 * 모든 강조는 타일 위에 자기 알파로 Mix한다(알파가 1이면 타일이 안 비침).
 * 겹침 처리: Select가 있으면 Select만, 아니면 Effect가 아래 레이어(Aim/타일)와 자기 색 사이를 펄스로 오간다.
 */
USTRUCT(BlueprintType)
struct FTileHighlightStyle
{
	GENERATED_BODY()

	/**
	 * @brief 강조 색 (알파 포함 — 알파로 타일 비침 정도 조절)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TileHighlight", meta = (DisplayName = "Color"))
	FLinearColor mColor = FLinearColor::White;
};
