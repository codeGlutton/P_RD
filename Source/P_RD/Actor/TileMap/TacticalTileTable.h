/*****************************************************************//**
 * @file   TacticalTileTable.h
 * @brief  턴 계획용 전술 타일 테이블 정의 헤더
 * @author 이문환
 * @date   2026-07-25
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"

class UTileMapModel;
class UBoardActorModel;
class UStaticUnitSkillData;

/**
 * @brief 타일 하나의 전술 정보
 */
struct P_RD_API FTacticalTileInfo
{
	// @brief 타일 좌표
	FTileIndex mIndex = FTileIndex::Invalid;

	// @brief 원점에서 이 타일까지의 이동 비용 (경로 거리, 원점 자신은 0)
	int32 mMoveCost = 0;

	// @brief 조준가능한 {스킬,타겟} 정보
	// @details
	// 행: 스킬 슬롯 인덱스
	// 열: 타겟 배열의 인덱스
	// 예) [0,1,0] [0,1,1]: 첫번째 스킬은 두번째 타겟에 조준가능, 두번째 스킬은 두번째+세번째 타겟에 조준가능
	// @note
	// 이 타일까지의 이동비용은 계산하지 않음
	// 순전히 '이 타일에 서 있다면' 조준가능한 스킬과 타겟을 수집하는 기능.
	TArray<bool> mAimableFlags;

	// @brief 시전가능한 {스킬,타겟} 정보
	// @details
	// 행: 스킬 슬롯 인덱스
	// 열: 타겟 배열의 인덱스
	// 예) [0,0,0] [0,1,0]: 첫번째 스킬은 아무도 때릴 수 없음, 두번째 스킬은 두번째 타겟에 시전가능
	// @note
	// 조준가능한 {스킬,타겟} 중에서 행동력을 사용해서 시전까지 가능한 것들만 따로 저장
	TArray<bool> mCastableFlags;

	// @brief 타겟까지의 경로 거리
	// @details
	// 예) [3,4,MAX_int32]: 경로 거리 기준으로 첫번째 타겟은 3, 두번째 타겟은 4, 세번째 타겟은 도달하는 경로가 없음
	TArray<int32> mTargetDistances;
};

/**
 * @brief 턴 계획용 전술 타일 테이블
 * @details
 * 원점(계획 주체 위치)에서 도달 가능한 타일마다 전술 정보를 미리 채워두고,
 * 이후 판단은 전부 저장된 값의 조회로 처리하는 임시 데이터.
 * 보드 상태가 바뀌면 낡은 정보가 되므로 턴 계획 1회 안에서만 사용.
 */
class P_RD_API FTacticalTileTable
{
public:
	/**
	 * @brief 테이블 구성
	 * @param[in] TileMap : 타일맵 모델
	 * @param[in] Self : 계획 주체 (이동으로 자리를 비울 예정이므로 시야 차폐와 통과 판정에서 제외)
	 * @param[in] Origin : 계획 주체의 현재 타일
	 * @param[in] TargetTiles : 타겟들의 타일 (배열 순서가 곧 타겟 인덱스)
	 * @param[in] Skills : 스킬 슬롯 배열 (배열 순서가 곧 슬롯 인덱스, 빈 슬롯은 nullptr)
	 * @param[in] MoveBudget : 이동에 쓸 수 있는 행동력 (도달 범위 기준, 속박 등으로 이동 불가면 0)
	 * @param[in] ActionPoint : 이동과 스킬 시전이 나눠 쓰는 행동력 (시전 예산 판정 기준)
	 */
	void Build(
		const UTileMapModel* TileMap,
		const UBoardActorModel* Self,
		const FTileIndex& Origin,
		const TArray<FTileIndex>& TargetTiles,
		const TArray<const UStaticUnitSkillData*>& Skills,
		int32 MoveBudget,
		int32 ActionPoint);

	// @brief 해당 타일에서 해당 스킬로 해당 타겟을 조준 가능한 지 판정
	bool IsAimable(const FTacticalTileInfo& Tile, int32 SkillSlot, int32 TargetIndex) const;

	// @brief 해당 타일에서 해당 스킬로 해당 타겟에게 시전 가능한 지 판정
	bool IsCastable(const FTacticalTileInfo& Tile, int32 SkillSlot, int32 TargetIndex) const;

	// @brief 해당 타겟에게 시전 가능한 지 판정 (어느 타일에서든, 어떤 스킬로든 가능하면 참)
	bool CanCastToTarget(int32 TargetIndex) const;

	// @brief 해당 타겟에게 시전 가능한 스킬 슬롯 목록 (어느 타일에서든 가능하면 포함)
	TArray<int32> GetCastableSkillSlots(int32 TargetIndex) const;

	// @brief 조준 가능한 {타일, 스킬, 타겟} 조합이 하나라도 있는 지 판정
	bool HasAnyAimable() const;

	// @brief 전술 타일 목록 조회
	const TArray<FTacticalTileInfo>& GetTacticalTiles() const { return mTacticalTiles; }

	// @brief 원점에서 해당 타겟까지의 경로 거리 (도달 불가면 MAX_int32)
	int32 GetDistanceToTarget(int32 TargetIndex) const;

	// @brief 해당 타일에서 가장 가까운 타겟까지의 경로 거리
	int32 GetNearestTargetDistance(const FTacticalTileInfo& Tile) const;

	/**
	 * @brief 필터를 통과한 타일 중 축 값이 가장 작은 타일 선택
	 * @details
	 * 1순위 축이 같으면 2순위 축으로 비교, 그래도 같으면 먼저 나온 타일 유지 (결정성 보장)
	 * 큰 값이 좋은 축(위협 거리 최대화 등)은 호출부에서 부호를 반전해서 전달
	 * @param[in] Filter : 후보 포함 여부 판정
	 * @param[in] PrimaryAxis : 1순위 축 값 (작을수록 좋음)
	 * @param[in] SecondaryAxis : 2순위 축 값 (작을수록 좋음)
	 * @param[in] Fallback : 후보가 하나도 없을 때 반환할 타일
	 * @return FTileIndex : 선택된 타일
	 */
	FTileIndex PickTile(
		TFunctionRef<bool(const FTacticalTileInfo&)> Filter,
		TFunctionRef<int64(const FTacticalTileInfo&)> PrimaryAxis,
		TFunctionRef<int64(const FTacticalTileInfo&)> SecondaryAxis,
		const FTileIndex& Fallback) const;

private:
	// @brief {스킬, 타겟}을 인자로 받아서 플래그 배열의 1차원 인덱스로 변환
	int32 FlagIndex(int32 SkillSlot, int32 TargetIndex) const;

	// @brief 전술 타일 정보 저장소 (도달 가능 타일들 + 원점)
	TArray<FTacticalTileInfo> mTacticalTiles;

	// @brief 원점에서 타겟까지의 경로 거리 (인덱스=타겟, 도달 불가면 MAX_int32)
	TArray<int32> mOriginTargetDistances;

	// @brief 스킬 슬롯 수 (플래그 배열의 행 수)
	int32 mSkillSlotCount = 0;

	// @brief 타겟 수 (플래그 배열의 열 수)
	int32 mTargetCount = 0;
};
