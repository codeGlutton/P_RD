/*****************************************************************//**
 * @file   GameplayAbility_Base.h
 * @brief  스킬 데미지 헤더
 * @author 김준형
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "GameplayEffect_Base.h"
#include "GameplayEffect_Damage.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UGameplayEffect_Damage : public UGameplayEffect_Base
{
	GENERATED_BODY()

public:
	UGameplayEffect_Damage();
};
