/*****************************************************************//**
 * @file   PrimaryAssetType.h
 * @brief  사용되는 Primary Asset Type 매크로를 모아둔 파일
 * @author 모호재
 * @date   2026-05-12
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "DataAsset/RarityType.h"

#define DECLARE_PRIMARY_ASSET_TYPE(TypeName)													\
inline static FPrimaryAssetType Get##TypeName##Type()											\
{																								\
	return FPrimaryAssetType(TEXT(#TypeName));													\
}

 /**
  * @brief 스테이지 Primary Asset Type들을 정의한 namespace 영역
  */
namespace StagePrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(Stage);
}

/**
 * @brief 방 Primary Asset Type들을 정의한 namespace 영역
 */
namespace RoomPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(FrontendRoom);

	DECLARE_PRIMARY_ASSET_TYPE(MonsterRoom);
	DECLARE_PRIMARY_ASSET_TYPE(EliteMonsterRoom);
	DECLARE_PRIMARY_ASSET_TYPE(BossMonsterRoom);
	DECLARE_PRIMARY_ASSET_TYPE(ShopRoom);
	DECLARE_PRIMARY_ASSET_TYPE(TreasureRoom);
}

/**
 * @brief 장애물 Primary Asset Type들을 정의한 namespace 영역
 */
namespace ObstaclePrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(Obstacle);
	DECLARE_PRIMARY_ASSET_TYPE(CombatTargetObstacle);
}

/**
 * @brief 유닛 Primary Asset Type들을 정의한 namespace 영역
 */
namespace UnitPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(PlayerUnit);
	DECLARE_PRIMARY_ASSET_TYPE(EnemyUnit);
}

/**
 * @brief 장비 Primary Asset Type들을 정의한 namespace 영역
 */
namespace EquipmentPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(Equipment);
}

/**
 * @brief 아티펙트 Primary Asset Type들을 정의한 namespace 영역
 */
namespace ArtifactPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(Artifact);
}

/**
 * @brief 스킬 Primary Asset Type들을 정의한 namespace 영역
 * @details
 * 스킬 타입과 희귀도 별로 등장하는 방을 나누어 설계
 */
namespace SkillPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(Passive);
	DECLARE_PRIMARY_ASSET_TYPE(Active);
	DECLARE_PRIMARY_ASSET_TYPE(ObstacleActive);
}

/**
 * @brief 주사위 Primary Asset Type들을 정의한 namespace 영역
 * @details
 * 희귀도 별로 등장하는 방을 나누어 설계
 */
namespace DicePrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(Dice);
}
