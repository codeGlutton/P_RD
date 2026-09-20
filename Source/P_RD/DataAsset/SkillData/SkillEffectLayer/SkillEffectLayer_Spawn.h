/*****************************************************************//**
 * @file   SkillEffectLayer_Spawn.h
 * @brief  하나의 스킬 모션 내에서 적용하는 동적 보드 액터 스폰 효과 레이어 구현 헤더
 * @author 모호재
 * @date   2026-09-07
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"
#include "DataAsset/ObstacleSpawnData/StaticObstacleSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "SkillEffectLayer_Spawn.generated.h"

/**
 * @brief  동적 보드 액터 배치 정보 구조체
 * @details
 * UStaticObstacleSpawnData 기반 에셋을 받아 유닛(UStaticUnitSpawnData/UStaticEnemyUnitSpawnData) 여부에 따라
 * FEnemyUnitPlacementData 또는 FObstaclePlacementData를 생성하여 반환합니다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FDynamicPlacementData
{
	GENERATED_BODY()

public:
	/**
	 * @brief 설정된 스폰 데이터가 유닛(UStaticUnitSpawnData 계열)인지 여부 반환
	 * @return 유닛(UStaticUnitSpawnData 계열)인지 여부
	 */
	bool IsUnit() const;

	FEnemyUnitPlacementData MakeEnemyUnitPlacementData(const FTileTransform& TargetTileTransform) const;
	FObstaclePlacementData MakeObstaclePlacementData(const FTileTransform& TargetTileTransform) const;

public:
	// @brief 스폰할 장애물 또는 유닛의 정적 스폰 데이터 에셋
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SpawnData", AssetBundles = "PAD"))
	TSoftObjectPtr<UStaticObstacleSpawnData> mSpawnData;

	// @brief 적 유닛일 경우 난이도 설정
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 0;

	// @brief 적 유닛일 경우 기본 스피드 포인트 오프셋
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultSpeedPoint"))
	int32 mDefaultSpeedPoint = 0;
};

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 동적 보드 액터 스폰 효과 레이어
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Spawn : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;
	FText MakeDescription() const override;

public:
	// @brief 동적 배치 설정 정보 Array
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SpawnDatas"))
	TArray<FDynamicPlacementData> mSpawnDatas;
};
