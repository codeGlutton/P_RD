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
 * @brief  타일 강조 색을 아래 누적색과 합성하는 방식
 */
UENUM(BlueprintType)
enum class ETileHighlightBlend : uint8
{
	Mix			UMETA(ToolTip = "아래 누적색과 섞음"),
	Overwrite	UMETA(ToolTip = "아래를 무시하고 덮어씀"),
};

/**
 * @brief  타일 강조 상태 1종을 어떻게 그릴지 정의하는 스타일
 * @details
 * 상태(Aim/Select/Effect)마다 1개씩 타일맵이 보유하는 설정값.
 * 한 타일에 여러 상태가 겹치면 Priority 오름차순(낮음=바닥)으로 쌓고,
 * 각 겹의 BlendMode대로 누적해 최종 색을 만든다.
 */
USTRUCT(BlueprintType)
struct FTileHighlightStyle
{
	GENERATED_BODY()

	// @brief 강조 색 (알파 포함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TileHighlight", meta = (DisplayName = "Color"))
	FLinearColor mColor = FLinearColor::White;

	// @brief 아래 누적색과의 합성 방식
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TileHighlight", meta = (DisplayName = "Blend Mode"))
	ETileHighlightBlend mBlendMode = ETileHighlightBlend::Mix;

	// @brief 합성 순서 (낮을수록 먼저 깔리고, 높을수록 위에 얹혀 최종을 지배)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TileHighlight", meta = (DisplayName = "Priority"))
	int32 mPriority = 0;
};
