/*****************************************************************//**
 * @file   SRPGEnemyTurnPlanner.h
 * @brief  적 한 턴의 행동을 계산하는 모델 레이어 플래너 정의 헤더
 * @author 이문환
 * @date   2026-06-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGEnemyTurnPlanner.generated.h"

class UEnemyUnitModel;
class UUnitModel;
class UTileMapModel;
class UBoardActorModel;
enum class EMoveTendency : uint8; // StaticEnemyUnitSpawnData.h

/**
 * @brief 적 한 턴의 행동을 계산하는 플래너
 *
 * @details
 * 플레이어가 쓰는 것과 동일한 최종 커맨드(이동/스킬시전/턴종료)를 순서대로 만들어 반환
 */
UCLASS()
class P_RD_API USRPGEnemyTurnPlanner : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 적 한 턴의 행동 커맨드 목록을 계산
	 * @details 반환 목록은 FSRPGTurnEndCommand로 끝나서 턴이 끝나도록 보장
	 * @param Enemy 행동할 적 유닛
	 * @param Player 표적이 될 플레이어 유닛
	 * @param TileMap 타일맵 모델 (도달/조준/효과 범위 계산)
	 * @param EventStream 스킬 랜덤 선택용 스트림 (시뮬/라이브 동일 결과 보장을 위해 룸의 이벤트 스트림 사용)
	 * @return 커맨드 목록
	 */
	static TArray<TInstancedStruct<FSRPGCommand>> PlanTurn(
		UEnemyUnitModel* Enemy,
		UUnitModel* Player,
		const UTileMapModel* TileMap,
		const FRandomStream& EventStream);

private:
	/**
	 * @brief 타일 목록에서 이동성향에 맞는 최선 타일 선택
	 * @details
	 * MoveClose: 플레이어와 가장 가까운 타일
	 * MoveAway: 플레이어와 가장 먼 타일
	 * HoldRange: 제자리가 목록에 있으면 제자리, 없으면 이동거리 최소 타일
	 * @param Tiles 후보 타일 목록 (시전가능 또는 조준가능 타일들)
	 * @param EnemyTile 적 타일
	 * @param PlayerTile 플레이어 타일
	 * @param Tendency 이동 성향
	 */
	static FTileIndex PickByTendency(
		const TArray<FTileIndex>& Tiles,
		const FTileIndex& EnemyTile,
		const FTileIndex& PlayerTile,
		EMoveTendency Tendency);

	/**
	 * @brief 타일 목록 중 플레이어와의 거리가 이동성향에 맞는 타일 선택
	 * @details
	 * 1순위: 플레이어와의 거리(Closest면 최소, 아니면 최대)
	 * 2순위: 이동거리 최소
	 * @param Tiles 도달 가능한 타일 목록
	 * @param Origin 적 타일
	 * @param PlayerTile 플레이어 타일
	 * @param Closest true면 플레이어에 가까운 쪽, false면 먼 쪽을 선호
	 */
	static FTileIndex PickByPlayerDistance(
		const TArray<FTileIndex>& Tiles,
		const FTileIndex& Origin,
		const FTileIndex& PlayerTile,
		bool Closest);

	// @brief 타일 목록 중 Origin에서 이동거리(맨해튼)가 가장 작은 타일 선택
	static FTileIndex PickByMoveCost(
		const TArray<FTileIndex>& Tiles,
		const FTileIndex& Origin);

	/**
	 * @brief 조준 가능한 타일이 없을 때 플레이어에게 실제로 가까워지는 타일 선택
	 * @details
	 * 기존 거리 계산 방식은 다른 유닛을 우회하는 비용이 안 잡혀서,
	 * 앞이 막히면 제자리가 비용이 가장 작으니까 이동을 안 하는 문제가 있음.
	 * 
	 * 플레이어 기준으로 거리표(GetDistanceField)를 작성해서 다음과 같이 판단.
	 * 1순위: 경로 거리 최소
	 * 2순위: 이동 거리 최소
	 * @param Tiles 도달 가능한 타일 목록 (Origin 포함)
	 * @param Origin 적 타일
	 * @param PlayerTile 플레이어 타일
	 * @param TileMap 타일맵 모델
	 * @param Self 거리장 통과 판정에서 제외할 자기 자신 (자리를 비울 예정이므로)
	 */
	static FTileIndex PickApproachTile(
		const TArray<FTileIndex>& Tiles,
		const FTileIndex& Origin,
		const FTileIndex& PlayerTile,
		const UTileMapModel* TileMap,
		const UBoardActorModel* Self);

	// @brief 타일 사이의 거리를 맨해튼 방식으로 계산 (이동은 맨해튼식으로 하니까)
	static int32 TileDistance(const FTileIndex& A, const FTileIndex& B);
};
