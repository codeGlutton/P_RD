/*****************************************************************//**
 * @file   PlayerUnit.h
 * @brief  플레이어 베이스 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "Pawn/Unit.h"
#include "DataAsset/UnitSpawnData/PlayerJobType.h"

#include "PlayerUnit.generated.h"

class ULevelAttributeSet;

/**
 * @brief  플레이어 베이스 유닛
 */
UCLASS(abstract)
class P_RD_API APlayerUnit : public AUnit
{
	GENERATED_BODY()

public:
	APlayerUnit();

	/* APawn 상속 */
public:
	void PostInitializeComponents() override;
	void OnConstruction(const FTransform& Transform) override;

	/* AUnit 상속 */
protected:
	UUserWidget* GetInfoPanel() const override;

public:
	EPlayerJobType GetPlayerJobType() const;
	int32 GetPlayerLevel() const;
	int32 GetDifficulty() const override;
	bool IsPlayerUnit() const override;

private:
	UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "LevelAttributeSet"))
	TObjectPtr<ULevelAttributeSet> mLevelAttributeSet;

protected:
	UPROPERTY(Category = Unit, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "JobType"))
	EPlayerJobType mJobType;
};
