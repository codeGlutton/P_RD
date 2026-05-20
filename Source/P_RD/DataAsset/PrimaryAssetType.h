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

#define DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(TypeName, SuffixEnumType, SuffixName)			\
DECLARE_PRIMARY_ASSET_TYPE(TypeName)															\
inline static FPrimaryAssetType Get##TypeName##Type(const FString& SuffixName)					\
{																								\
	return FPrimaryAssetType(*FString::Printf(TEXT("%s_%s"), TEXT(#TypeName), *SuffixName));	\
}																								\
inline static FPrimaryAssetType Get##TypeName##Type(SuffixEnumType SuffixName)					\
{																								\
	return Get##TypeName##Type(EnumToString(SuffixName));										\
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
 * @details
 * 방 타입과 스테이지 별로 등장하는 방을 나누어 설계
 */
namespace RoomPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(TitleRoom);

	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(MonsterRoom, EStageLevelType, StageLevel);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(EliteMonsterRoom, EStageLevelType, StageLevel);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(BossMonsterRoom, EStageLevelType, StageLevel);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(ShopRoom, EStageLevelType, StageLevel);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(TreasureRoom, EStageLevelType, StageLevel);
}

/**
 * @brief 유닛 Primary Asset Type들을 정의한 namespace 영역
 * @details
 * 유닛 타입별로 등장하는 방을 나누어 설계
 */
namespace UnitPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE(PlayerUnit);
	DECLARE_PRIMARY_ASSET_TYPE(EnemyUnit);
}

/**
 * @brief 장비 Primary Asset Type들을 정의한 namespace 영역
 * @details
 * 장비 타입과 희귀도 별로 등장하는 방을 나누어 설계
 */
namespace EquipmentPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(Weapon, ERarityType, Rarity);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(Gloves, ERarityType, Rarity);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(Boots, ERarityType, Rarity);
}

/**
 * @brief 스킬 Primary Asset Type들을 정의한 namespace 영역
 * @details
 * 스킬 타입과 희귀도 별로 등장하는 방을 나누어 설계
 */
namespace SkillPrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(Attack, ERarityType, Rarity);
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(Spell, ERarityType, Rarity);
}

/**
 * @brief 주사위 Primary Asset Type들을 정의한 namespace 영역
 * @details
 * 희귀도 별로 등장하는 방을 나누어 설계
 */
namespace DicePrimaryAssetTypes
{
	DECLARE_PRIMARY_ASSET_TYPE_WITH_SUFFIX(Dice, ERarityType, Rarity);
}
