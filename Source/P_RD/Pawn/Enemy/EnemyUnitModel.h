/*****************************************************************//**
 * @file   EnemyUnitModel.h
 * @brief  적 베이스 유닛 모델 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/UnitModel.h"
#include "EnemyUnitModel.generated.h"

/**
 * @brief  적 베이스 유닛 모델
 */
UCLASS(abstract)
class P_RD_API UEnemyUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	UEnemyUnitModel();

	/* AUnit 상속 */
public:
	int32 GetDifficulty() const override;
	// 적 유닛은 플레이어가 아니다. 미구현 시 UUnitModel의 순수가상이 호출돼 크래시한다.
	bool IsPlayerUnitModel() const override { return false; }

protected:
	UUserWidget* GetInfoPanel() const override;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	UPROPERTY(Category = Enemy, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty;
};
