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
class UStaticSkillData;
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
	 * @return 커맨드 목록
	 */
	static TArray<TInstancedStruct<FSRPGCommand>> PlanTurn(
		UEnemyUnitModel* Enemy,
		UUnitModel* Player,
		const UTileMapModel* TileMap);

private:
	/**
	 * @brief 이동 성향에 따라 목적지 타일을 선택
	 * @details
	 * 이동가능한 타일들 중에서 플레이어를 조준 가능한 타일을 탐색. (이 타일들을 Feasible이라고 가정)
	 * Feasible 타일들 중에서 이동성향에 따라 최선의 타일 선택.
	 * Feasible 타일이 없으면 이동성향에 맞춰서 플레이어에 접근하고. 스킬 사용은 생략
	 * @note Self는 이동 후 위치를 평가할 때 자기 자신이 시야를 막지 않도록 차폐 예외로 전달
	 */
	static FTileIndex ChooseDestination(
		const FTileIndex& Origin,
		const FTileIndex& PlayerTile,
		int32 MoveRange,
		int32 AimRange,
		const UStaticSkillData* Skill,
		const UTileMapModel* TileMap,
		EMoveTendency Tendency,
		const UBoardActorModel* Self,
		OUT bool& OutCanCast);

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

	// @brief 타일 사이의 거리를 맨해튼 방식으로 계산 (이동은 맨해튼식으로 하니까)
	static int32 TileDistance(const FTileIndex& A, const FTileIndex& B);
};
