/*****************************************************************//**
 * @file   BoardActorModel.h
 * @brief  보드에 올라가는 액터 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/ActorModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileLayer.h"
#include "BoardActorModel.generated.h"

struct FTile;

/**
 * @brief  보드에 올라가는 액터 데이터 모델 클래스
 * @details 보드 위에 올라가는 액터(플레이어, 몬스터 등)의 데이터 모델 베이스 클래스다.
 */
UCLASS(Abstract)
class P_RD_API UBoardActorModel : public UActorModel
{
	GENERATED_BODY()

	// 타일맵 모델이 배치/이동 시 타일 트랜스폼·오버랩 콜백(protected)을 호출
	friend class UTileMapModel;
    // SRPG 전투 모델이 라운드 시작과 종료를 호출
    friend class USRPGCombatModel;
    // 턴 객체가 턴 시작과 종료를 호출
	friend struct FSRPGTurnContext;

public:
	/**
	 * @brief 타일 트랜스폼 반환
	 * @return 타일 트랜스폼
	 */
	const FTileTransform& GetTileTransform() const;

	/**
	 * @brief 액터의 레이어 타입을 반환
	 * @return 레이어 타입
	 */
	ETileLayerFlag GetTileLayerFlags() const;

	/**
	 * @brief 타일 배치 시에 블로킹할 타입들 반환
	 * @return 블로킹할 레이어 타입들
	 */
	ETileLayerFlag GetBlockLayerFlags() const;

	/**
	 * @brief 타일 배치 시에 교체할 타입들 반환
	 * @return 교체할 레이어 타입들
	 */
	ETileLayerFlag GetReplaceLayerFlags() const;

	/**
	 * @brief Overlay 레이어 내 교체 우선순위 반환
	 * @details 같은 교체 대상끼리 우열을 가린다. 진입자 우선순위가 같거나 더 높을 때만 기존 액터를 덮어쓴다.
	 * @return 우선순위 (높을수록 우선)
	 */
	int32 GetOverlayLayerPriority() const;

protected:
	/**
	 * @brief 타일 트랜스폼 설정
	 * @param Transform 설정할 타일 트랜스폼
	 */
	void SetTileTransform(const FTileTransform& Transform);

	/**
	 * @brief 오버랩 시작 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnBeginTileOverlap(FTile* CurTile, UBoardActorModel* Other);
	/**
	 * @brief 오버랩 종료 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnEndTileOverlap(FTile* CurTile, UBoardActorModel* Other);

	/**
	 * @brief 다른 액터에게 교체되어 타일에서 밀려날 때 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 자신을 교체하고 들어오는 액터
	 */
	virtual void OnReplaced(FTile* CurTile, UBoardActorModel* Other);

	/**
	 * @brief 라운드 시작마다 실행될 함수 (라운드 : 고정된 턴 기준으로 한바퀴)
	 */
	virtual void OnBeginRound();
	virtual void OnEndRound();

	/**
	 * @brief 자신의 턴 시작마다 실행될 함수
	 */
	virtual void OnBeginTurn();
	virtual void OnEndTurn();

protected:
	// @brief 타일 트랜스폼(런타임 배치 상태)
	UPROPERTY(Category = "BoardActor", VisibleInstanceOnly, BlueprintReadOnly, Transient, meta = (DisplayName = "TileTransform"))
	FTileTransform mTileTransform = FTileTransform::Invalid;

	// @brief 액터가 속한 레이어 타입
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "TileLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	ETileLayerFlag mTileLayerFlags = ETileLayerFlag::None;

	// @brief 타일 배치 시 블로킹할 레이어 타입들
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "BlockLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	ETileLayerFlag mBlockLayerFlags = ETileLayerFlag::None;

	// @brief 타일 배치 시 교체할 레이어 타입들
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "ReplaceLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	ETileLayerFlag mReplaceLayerFlags = ETileLayerFlag::None;

	// @brief Overlay 레이어 내 교체 우선순위 (높을수록 우선)
	UPROPERTY(Category = "BoardActor", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "OverlayLayerPriority"))
	int32 mOverlayLayerPriority = 0;
};
