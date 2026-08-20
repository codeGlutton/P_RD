/*****************************************************************//**
 * @file   StaticGimmickSpawnData.h
 * @brief  기믹 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/ObstacleSpawnData/StaticCombatTargetObstacleSpawnData.h"
#include "StaticGimmickSpawnData.generated.h"

/**
 * @brief  기믹 생성 시 사용되는 정적 Primary Data Asset
 * @details 스폰할 모델/뷰/스킬 목록 필드는 부모 클래스들에 이미 있음. 여기엔 기믹 발동 설정만 추가
 */
UCLASS()
class P_RD_API UStaticGimmickSpawnData : public UStaticCombatTargetObstacleSpawnData
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	// @brief 에셋 저장 시 발동 설정 검사 (횟수가 0이거나 스킬 슬롯 인덱스가 스킬 목록 밖이면 에러)
	EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// @brief 발동 가능 횟수 (음수 = 무제한, 0 입력 금지)
	UPROPERTY(Category = "Gimmick", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TriggerCount"))
	int32 mTriggerCount = 1;

	// @brief 발동 시 시전할 스킬 슬롯 인덱스 (SkillDatas 기준)
	UPROPERTY(Category = "Gimmick", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TriggerSkillIndex"))
	int32 mTriggerSkillIndex = 0;
};
