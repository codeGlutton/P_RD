/*****************************************************************//**
 * @file   SkillResultHolder.h
 * @brief  스킬의 결과값을 GA에 보내기 위한 오브젝트
 * @details 그대로 복사하므로 메모리를 크게 차지. 따라서 메모리를 아끼는 방향으로 수정 요망
 * @author 김준형
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "../../FunctionLibrary/CombatCalculator/CombatResult.h"
#include "SkillCommitResultHolder.generated.h"

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
