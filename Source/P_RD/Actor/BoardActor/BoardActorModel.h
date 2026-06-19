/*****************************************************************//**
 * @file   BoardActorModel.h
 * @brief  보드에 올라가는 액터 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/ActorModel.h"
#include "Actor/TileMap/Tile.h"
#include "Actor/TileMap/TileLayer.h"
#include "BoardActorModel.generated.h"

/**
 * @brief  보드에 올라가는 액터 데이터 모델 클래스
 * @details 보드 위에 올라가는 액터(플레이어, 몬스터 등)의 데이터 모델 베이스 클래스다.
 */
UCLASS(Abstract)
class P_RD_API UBoardActorModel : public UActorModel
{
	GENERATED_BODY()

	friend class ATileMap;

	friend class USRPGCombatSubsystem;
	friend struct FSRPGTurnContext;

public:
	virtual const FTileTransform& GetTileTransform() const PURE_VIRTUAL(UBoardActorModel::GetTileTransform, return FTileTransform::Invalid;)

protected:
	virtual void SetTileTransform(const FTileTransform & Transform) PURE_VIRTUAL(UBoardActorModel::SetTileTransform, return;)

public:
	/**
	 * @brief 액터의 레이어 타입을 반환
	 * @return 레이어 타입
	 */
	virtual ETileLayerFlag GetTileLayerFlags() const PURE_VIRTUAL(UBoardActorModel::GetTileLayer, return ETileLayerFlag::None;)
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
	/**
	 * Overlay 레이어 내 교체 우선순위 반환
	 * @details 같은 교체 대상끼리 우열을 가린다. 진입자 우선순위가 같거나 더 높을 때만 기존 액터를 덮어쓴다.
	 * @return 우선순위 (높을수록 우선)
	 */
	virtual int32 GetOverlayLayerPriority() const;

protected:
	/**
	 * @brief 오버랩 시작 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnBeginTileOverlap(FTile * CurTile, UBoardActorModel* Other);
	/**
	 * @brief 오버랩 종료 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnEndTileOverlap(FTile * CurTile, UBoardActorModel* Other);

	/**
	 * @brief 다른 액터에게 교체되어 타일에서 밀려날 때 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 자신을 교체하고 들어오는 액터
	 */
	virtual void OnReplaced(FTile * CurTile, UBoardActorModel* Other);

	/**
	 * 라운드 시작마다 실행될 함수. (라운드 : 고정된 턴 기준으로 한바퀴)
	 */
	virtual void OnBeginRound();
	virtual void OnEndRound();

	/**
	 * 자신의 턴 시작마다 실행될 함수.
	 */
	virtual void OnBeginTurn();
	virtual void OnEndTurn();
};
