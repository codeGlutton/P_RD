/*****************************************************************//**
 * @file   StaticPuddleGimmickSpawnData.h
 * @brief  장판 기믹 생성 시 사용되는 정적 Primary Data Asset 정의 헤더
 * @author 이문환
 * @date   2026-08-21
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/ObstacleSpawnData/StaticGimmickSpawnData.h"
#include "StaticPuddleGimmickSpawnData.generated.h"

/**
 * @brief  장판 기믹 생성 시 사용되는 정적 Primary Data Asset
 * @details 진입 발동 설정은 부모(기믹 DA)에 이미 있음. 여기엔 장판의 라운드 수명만 추가
 */
UCLASS()
class P_RD_API UStaticPuddleGimmickSpawnData : public UStaticGimmickSpawnData
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	// @brief 에셋 저장 시 라운드 수명 검사 (0이면 에러)
	EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// @brief 유지되는 라운드 수 (음수 = 무제한, 0 입력 금지)
	UPROPERTY(Category = "Gimmick", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RoundLifetime"))
	int32 mRoundLifetime = 3;
};
