/*****************************************************************//**
 * @file   TileActor.h
 * @brief  SRPG 타일에 배치 가능한 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "Actor/TileMap/Tile.h"
#include "Actor/TileMap/TileLayer.h"
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

	friend class ATileMap;

public:
	virtual const FTileTransform& GetTileTransform() const PURE_VIRTUAL(ITileActor::GetTileTransform, return FTileTransform::Invalid;)

protected:
	virtual void SetTileTransform(const FTileTransform & Transform) PURE_VIRTUAL(ITileActor::SetTileTransform, return;)

public:
	/**
	 * 액터의 레이어 타입을 반환
	 * @return 레이어 타입
	 */
	virtual ETileLayerFlag GetTileLayerFlag() const PURE_VIRTUAL(ITileActor::GetTileLayer, return ETileLayerFlag::None;)
	/**
	 * 타일 배치 시에 블로킹할 타입들 반환
	 * @return 블로킹할 레이어 타입들
	 */
	virtual ETileLayerFlag GetBlockLayerFlags() const;
	/**
	 * 타일 배치 시에 교체할 타입들 반환
	 * @return 교체할 레이어 타입들
	 */
	virtual ETileLayerFlag GetReplaceLayerFlags() const;

protected:
	/**
	 * 오버랩 시작 시 실행될 함수
	 * @param Other 반대 대상
	 * @param CurTile 현재 위치한 타일 객체
	 */
	virtual void OnBeginTileOverlap(ITileActor* Other, FTile* CurTile);
	/**
	 * 오버랩 종료 시 실행될 함수
	 * @param Other 반대 대상
	 * @param CurTile 현재 위치한 타일 객체
	 */
	virtual void OnEndTileOverlap(ITileActor* Other, FTile* CurTile);

protected:
	/**
	 * 라운드 시작마다 실행될 함수. (라운드 : 고정된 턴 기준으로 한바퀴)
	 * @param CurTile 현재 위치한 타일 객체
	 */
	virtual void OnBeginRound();
	/**
	 * 사이클 시작마다 실행될 함수. (사이클 : 해당 턴 기준으로 한바퀴)
	 * @param CurTile 현재 위치한 타일 객체
	 */
	virtual void OnBeginCycle();
	/**
	 * 자신의 턴 시작마다 실행될 함수.
	 * @param CurTile 현재 위치한 타일 객체
	 */
	virtual void OnBeginTurn();
};



