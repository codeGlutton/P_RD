/*****************************************************************//**
 * @file   StaticAttackSkillData.h
 * @brief  공격 스킬 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "StaticAttackSkillData.generated.h"

/**
 * @brief  공격 스킬 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticAttackSkillData : public UStaticSkillData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(SkillPrimaryAssetTypes::GetAttackType(mRarityType), GetFName());
	}
};
