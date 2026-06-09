/*****************************************************************//**
 * @file   Tile.h
 * @brief  SRPG의 타일 정의 헤더
 * @author 이문환
 * @date   2026-05-26
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileHighlight.h"
#include "Tile.generated.h"

class ITileActor;

/**
 * @brief  SRPG의 타일
 */
USTRUCT()
struct FTile
{
	GENERATED_BODY()

	// @brief ATileMap이 타일 내부 액터 목록/블로킹 정보에 접근
	friend class ATileMap;

protected:
	// @brief 제한된 레이어
	UPROPERTY(meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	int32 mBlockLayerFlags;

	// @brief 배치된 액터
	TArray<TScriptInterface<ITileActor>> mActors;

	// @brief 강조 표시 상태 (Aim/Select/Effect 비트 조합)
	ETileHighlightFlag mHighlight = ETileHighlightFlag::None;
};
