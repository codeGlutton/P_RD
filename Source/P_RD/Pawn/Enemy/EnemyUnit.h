/*****************************************************************//**
 * @file   EnemyUnit.h
 * @brief  적 베이스 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "Pawn/Unit.h"

#include "EnemyUnit.generated.h"

/**
 * @brief  적 베이스 유닛
 */
UCLASS(abstract)
class P_RD_API AEnemyUnit : public AUnit
{
	GENERATED_BODY()

public:
	AEnemyUnit();

	/* AUnit 상속 */
public:
	int32 GetDifficulty() const override;

protected:
	UUserWidget* GetInfoPanel() const override;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	UPROPERTY(Category = Enemy, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty;
};
