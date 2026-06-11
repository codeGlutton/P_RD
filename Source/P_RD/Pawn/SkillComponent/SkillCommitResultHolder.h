/*****************************************************************//**
 * @file   SkillResultHolder.h
 * @brief  스킬의 결과값을 GA에 보내기 위한 오브젝트
 * @author 김준형
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "../../FunctionLibrary/CombatCalculator/CombatResult.h"
#include "SkillResultHolder.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API USkillCommitResultHolder : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FSkillCommitResult CommitResult;
};
