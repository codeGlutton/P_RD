/*****************************************************************//**
 * @file   AnimNotifyCondition.h
 * @brief  애니메이션 노티파이 실행 조건 정의 헤더
 * @author 모호재
 * @date   2026-07-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "AnimNotifyCondition.generated.h"

class USkeletalMeshComponent;

/**
 * @brief 애님 노티파이 실행 조건 기본 구조체
 */
USTRUCT(BlueprintType)
struct P_RD_API FAnimNotifyCondition
{
	GENERATED_BODY()

public:
	virtual ~FAnimNotifyCondition() {}

	/**
	 * @brief 조건이 충족되는지 검증합니다.
	 * @param MeshComp 검증을 진행할 스켈레탈 메시 컴포넌트
	 * @return 조건이 충족되면 true
	 */
	virtual bool EvaluateCondition(const USkeletalMeshComponent* MeshComp) const
	{
		return true;
	}
};

/**
 * @brief 이펙트 VFX 활성화 옵션을 검증하는 조건 구조체
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Effect VFX Option Condition"))
struct P_RD_API FAnimNotifyCondition_EffectOption : public FAnimNotifyCondition
{
	GENERATED_BODY()

public:
	virtual bool EvaluateCondition(const USkeletalMeshComponent* MeshComp) const override;
};
