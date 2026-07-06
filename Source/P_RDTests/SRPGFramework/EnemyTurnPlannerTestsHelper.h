/*****************************************************************//**
 * @file   EnemyTurnPlannerTestsHelper.h
 * @brief  USRPGEnemyTurnPlanner 테스트용 Mock 유닛모델 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *
 * @details
 *  USRPGEnemyTurnPlanner::PlanTurn은 static 함수지만,
 *  UEnemyUnitModel, UPlayerUnitModel, UTileMapModel을 파라미터로 요구.
 *  파라미터로 넘길 최소한의 Mock 정의
 *
 *  Pre/PostInitializeComponentModels 함수는 빈 코드로 대체하고,
 *  이동포인트는 모델의 mMovePoint를 코드로 세팅해서 사용.
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Pawn/UnitModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h" // EMoveTendency
#include "EnemyTurnPlannerTestsHelper.generated.h"

/**
 * @brief 적유닛 Mock
 * @details 이동 성향/스킬/이동력을 코드로 세팅할 수 있는 최소 UEnemyUnitModel.
 */
UCLASS()
class UMockEnemyUnitModel : public UEnemyUnitModel
{
	GENERATED_BODY()

public:
	UMockEnemyUnitModel()
	{
		// UnitAttributeSet은 이제 부모(UEnemyUnitModel) 생성자가 동일한 이름("UnitAttributeSet")으로 만든다.
		// 여기서 중복 생성/선언하면 UHT 섀도잉 에러 — 상속분을 그대로 사용한다.
	}

	/**
	 * 월드서브시스템 건너뛰기 (빈 코드)
	 */
	virtual void PreInitializeComponentModels() override
	{
	}
	virtual void PostInitializeComponentModels() override
	{
	}

	// @brief 이동성향 설정
	void SetMoveTendency(EMoveTendency Tendency)
	{
		mMoveTendency = Tendency;
	}

	// @brief 이동포인트 설정
	void SetMovePoint(int32 MovePoint)
	{
		mMovePoint = MovePoint;
	}

};

/**
 * @brief 플레이어유닛 Mock
 * @details 플레이어의 타일 좌표만 필요하니까 UUnitModel 베이스로 제작.
 */
UCLASS()
class UMockPlayerUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	/**
	 * 월드서브시스템 건너뛰기 (빈 코드)
	 */
	virtual void PreInitializeComponentModels() override
	{
	}
	virtual void PostInitializeComponentModels() override
	{
	}
	
	virtual int32 GetDifficulty() const override
	{
		return 0;
	}
	
	virtual bool IsPlayerUnitModel() const override
	{
		return true;
	}
};
