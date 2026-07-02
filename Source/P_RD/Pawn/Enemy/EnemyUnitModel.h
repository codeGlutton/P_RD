/*****************************************************************//**
 * @file   EnemyUnitModel.h
 * @brief  적 베이스 유닛 모델 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/UnitModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h" // EMoveTendency
#include "EnemyUnitModel.generated.h"

/**
 * @brief 적 베이스 유닛 모델
 */
UCLASS(abstract)
class P_RD_API UEnemyUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	UEnemyUnitModel();

	// @brief 스폰 데이터에서 스킬 데이터를 얻어서 스킬 컴포넌트에 적재
	void PostInitializeComponentModels() override;

	// @brief 난이도
	int32 GetDifficulty() const override;
	
	// @brief 플레이어유닛 여부
	bool IsPlayerUnitModel() const override { return false; }

	// @brief 이동 성향
	EMoveTendency GetMoveTendency() const;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	UPROPERTY(Category = Enemy, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty;

	// @brief 이동 성향
	UPROPERTY(Category = AI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveTendency"))
	EMoveTendency mMoveTendency = EMoveTendency::HoldRange;
};
