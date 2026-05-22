/*****************************************************************//**
 * @file   TileActor.h
 * @brief  SRPG 타일에 배치 가능한 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "TileActor.generated.h"

UINTERFACE(MinimalAPI)
class UTileActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 타일에 배치 가능한 객체 인터페이스
 */
class P_RD_API ITileActor
{
	GENERATED_BODY()

public:
	virtual bool IsBlocking() const;

protected:
	virtual void OnBeginTileOverlap(ITileActor* Other) const;
	virtual void OnEndTileOverlap(ITileActor* Other) const;
};
