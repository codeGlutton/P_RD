/*****************************************************************//**
 * @file   SkillAnimationMetaData.h
 * @brief  스킬 애니메이션의 메타 데이터 정의 헤더
 * @author 모호재
 * @date   2026-07-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Animation/BoardActorAnimType.h"
#include "SkillAnimationMetaData.generated.h"

class USkillComponentModel;

/**
 * @brief  스킬 애니메이션의 메타 데이터
 */
USTRUCT()
struct FSkillAnimationMetaData : public FBoardActorAnimationMetaData
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<USkillComponentModel> mInstigator;
};

