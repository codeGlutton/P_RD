/*****************************************************************//**
 * @file   SkillEffectLayer.h
 * @brief  하나의 스킬 모션 내에서 적용하는 단일 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-06-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SkillEffectLayer.generated.h"

class UTileMapModel;
class UBoardActorModel;

USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer
{
	GENERATED_BODY()

public:
	virtual ~FSkillEffectLayer() = default;

public:
	virtual void ApplyPointEffect(float DiceSum) const PURE_VIRTUAL(FSkillEffectLayer::ApplyPointEffect, return; );
	virtual void CommitEffect(float DiceSum) const PURE_VIRTUAL(FSkillEffectLayer::CommitEffect, return; );
};
