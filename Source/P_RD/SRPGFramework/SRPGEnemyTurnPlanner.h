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

// 적 플래너의 판단근거 로그 카테고리
DECLARE_LOG_CATEGORY_EXTERN(LogSRPGEnemyPlanner, Log, All);

class UEnemyUnitModel;
class UUnitModel;
class UTileMapModel;
class UBoardActorModel;
class FTacticalTileTable;
struct FTacticalTileInfo;
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
	 * @param Players 표적 후보가 될 플레이어 유닛들 (배열 순서가 곧 타겟 인덱스, null 원소는 제외됨)
	 * @param TileMap 타일맵 모델 (도달/조준/효과 범위 계산)
	 * @param EventStream 타겟 동률/스킬 랜덤 선택용 스트림 (시뮬/라이브 동일 결과 보장을 위해 룸의 이벤트 스트림 사용)
	 * @param LogTag 판단근거 로그의 줄마다 붙는 식별 태그 (예: "R3/T12", 비어있으면 생략)
	 * @return 커맨드 목록
	 */
	static TArray<TInstancedStruct<FSRPGCommand>> PlanTurn(
		UEnemyUnitModel* Enemy,
		const TArray<UUnitModel*>& Players,
		const UTileMapModel* TileMap,
		const FRandomStream& EventStream,
		const FString& LogTag = FString());

private:
	/**
	 * @brief 후보 타겟 중 최근접 타겟 선택
	 * @details 경로 거리가 가장 짧은 타겟, 동률이면 EventStream 랜덤 (시뮬/라이브 동일 결과 보장)
	 * @param Table 전술 타일 테이블
	 * @param CandidateTargets 후보 타겟 인덱스 목록
	 * @param EventStream 동률 추첨용 스트림
	 * @return 선택된 타겟 인덱스 (후보가 비어있으면 INDEX_NONE)
	 */
	static int32 ChooseNearestTarget(
		const FTacticalTileTable& Table,
		const TArray<int32>& CandidateTargets,
		const FRandomStream& EventStream);

	/**
	 * @brief 필터를 통과한 타일 중 이동성향에 맞는 목적지 선택
	 * @details
	 * MoveClose: 기준 타겟과의 거리 최소 -> 이동비용 최소
	 * MoveAway: 최근접 타겟과의 거리 최대 -> 이동비용 최소
	 * HoldRange: 이동비용 최소 -> 최근접 타겟과의 거리 최대
	 * @param Table 전술 타일 테이블
	 * @param Tendency 이동 성향
	 * @param ReferenceTarget 기준 타겟 인덱스 (MoveClose의 거리 기준)
	 * @param Origin 적 타일 (후보가 없을 때 제자리 유지)
	 * @param Filter 후보 타일 자격 판정 (시전가능/조준가능 등 호출부가 결정)
	 */
	static FTileIndex ChooseDestinationByTendency(
		const FTacticalTileTable& Table,
		EMoveTendency Tendency,
		int32 ReferenceTarget,
		const FTileIndex& Origin,
		TFunctionRef<bool(const FTacticalTileInfo&)> Filter);

	/**
	 * @brief 기준 타겟에게 접근하는 목적지 선택 (조준 가능한 타일이 하나도 없을 때의 폴백)
	 * @details 1순위: 기준 타겟과의 거리 최소, 2순위: 이동비용 최소 (동률이면 제자리 우선 -> 의미 없는 이동 방지)
	 * @param Table 전술 타일 테이블
	 * @param ReferenceTarget 기준 타겟 인덱스
	 * @param Origin 적 타일 (후보가 없을 때 제자리 유지)
	 */
	static FTileIndex ChooseApproachDestination(
		const FTacticalTileTable& Table,
		int32 ReferenceTarget,
		const FTileIndex& Origin);
};
