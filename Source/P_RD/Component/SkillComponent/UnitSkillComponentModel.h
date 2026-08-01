/*****************************************************************//**
 * @file   UnitSkillComponentModel.h
 * @brief  유닛의 액티브 스킬 컴포넌트 모델 구현 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-06-30
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "UnitSkillComponentModel.generated.h"

/**
 * @brief  유닛의 액티브 스킬 컴포넌트 모델
 */
UCLASS()
class P_RD_API UUnitSkillComponentModel : public USkillComponentModel
{
	GENERATED_BODY()

	/* USkillComponentModel 상속 */
protected:
	bool CanActiveSkill_Internal(int32 SkillIndex) const override;
	void ConsumeResources_Internal(int32 SkillIndex) override;

public:
	bool HasRequiredActionPoint(int32 SkillIndex) const;
	int32 GetRequiredActionPoint(int32 SkillIndex) const;
};
