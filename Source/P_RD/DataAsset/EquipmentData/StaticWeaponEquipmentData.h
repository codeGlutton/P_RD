/*****************************************************************//**
 * @file   StaticWeaponEquipmentData.h
 * @brief  무기 장비 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/EquipmentData/StaticEquipmentData.h"
#include "StaticWeaponEquipmentData.generated.h"

/**
 * @brief  무기 장비 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticWeaponEquipmentData : public UStaticEquipmentData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(EquipmentPrimaryAssetTypes::GetWeaponType(mRarityType), GetFName());
	}

	virtual EEquipmentType GetEquipmentType() const override { return EEquipmentType::Weapon; }
};
