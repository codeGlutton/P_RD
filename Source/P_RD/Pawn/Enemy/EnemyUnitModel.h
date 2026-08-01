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

class UEnemyUnitAttributeSet;
class UEquipmentComponentModel;

/**
 * @brief 적 베이스 유닛 모델
 */
UCLASS(abstract)
class P_RD_API UEnemyUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	UEnemyUnitModel();

	/* UUnitModel 상속 */
public:
	void PostInitializeComponentModels() override;
	int32 GetBoardActorLevel() const override;

public:
	UEquipmentComponentModel* GetEquipmentComponentModel() const;

public:
	/**
	 * @brief 스폰 전 난이도 대입 함수
	 * @param Difficulty 난이도
	 */
	void SetDifficulty(int32 Difficulty);

public:
	// @brief 난이도
	int32 GetDifficulty() const override;
	// @brief 플레이어유닛 여부
	bool IsPlayerUnitModel() const override { return false; }

	// @brief 이동 성향
	EMoveTendency GetMoveTendency() const;

private:
	UPROPERTY(Category = Equipment, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "EquipmentCompModel"))
	TObjectPtr<UEquipmentComponentModel> mEquipmentCompModel;

	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UEnemyUnitAttributeSet> mUnitAttributeSet;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	UPROPERTY(Category = Enemy, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;

	// @brief 이동 성향
	UPROPERTY(Category = AI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveTendency"))
	EMoveTendency mMoveTendency = EMoveTendency::HoldRange;
};
