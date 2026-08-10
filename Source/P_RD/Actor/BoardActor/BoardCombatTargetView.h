/*****************************************************************//**
 * @file   BoardCombatTargetView.h
 * @brief  전투 가능한 액터 뷰 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-07-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "BoardCombatTargetView.generated.h"

class USkillAnimationComponent;
class UCombatTargetVFXTimelineComponent;

UINTERFACE(MinimalAPI)
class UBoardCombatTargetView : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  전투 가능한 액터 뷰 인터페이스
 */
class P_RD_API IBoardCombatTargetView
{
	GENERATED_BODY()

public:
	virtual USkillAnimationComponent* GetSkillAnimationComponent() const = 0;
	virtual UCombatTargetVFXTimelineComponent* GetCombatTargetVFXTimelineComponent() const = 0;
	virtual UPrimitiveComponent* GetTargetMeshComponent() const = 0;
};
