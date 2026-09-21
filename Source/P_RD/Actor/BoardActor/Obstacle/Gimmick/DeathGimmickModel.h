/*****************************************************************//**
 * @file   DeathGimmickModel.h
 * @brief  사망 트리거 기믹 모델 정의 헤더
 * @author 모호재
 * @date   2026-09-21
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/Obstacle/Gimmick/GimmickModel.h"
#include "DeathGimmickModel.generated.h"

/**
 * @brief  사망 트리거 기믹 모델 (폭발 배럴 등)
 * @details 체력을 잃고 사망할 때 장착된 스킬을 발동하는 기믹의 베이스 클래스.
 *          유닛과 동일한 타일/블록 레이어를 가져 길을 막고, 살아있는 동안 조준 및 피격 대상이 됨.
 */
UCLASS(Blueprintable)
class P_RD_API UDeathGimmickModel : public UGimmickModel
{
	GENERATED_BODY()

public:
	UDeathGimmickModel();

	/* IBoardCombatTarget 상속 */
public:
	bool IsTargetable() const override;
	void OnPostDead() override;

protected:
	bool CanTriggerGimmick() const override;

protected:
	// @brief 사망 시에도 발동을 허용할지 여부 (사망 시 발동하는 기믹용)
	UPROPERTY(Category = "Gimmick", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "CanTriggerWhenDead"))
	bool mCanTriggerWhenDead = true;
};
