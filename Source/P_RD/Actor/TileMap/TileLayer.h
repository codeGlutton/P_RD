/*****************************************************************//**
 * @file   TileLayer.h
 * @brief  SRPG의 타일 레이어 정의 헤더
 * @author 모호재
 * @date   2026-05-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "TileLayer.generated.h"

/**
 * @brief  SRPG의 타일 레이어
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETileLayerFlag : uint8
{
	None = 0				UMETA(Hidden),
	Overlay = 1 << 0		UMETA(ToolTip = "유닛 아래 레이어"),
	Unit = 1 << 1			UMETA(ToolTip = "유닛과 충돌 가능한 레이어"),
	All = Overlay | Unit	UMETA(Hidden),
};
ENUM_CLASS_FLAGS(ETileLayerFlag)

