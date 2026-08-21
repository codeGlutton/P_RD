/*****************************************************************//**
 * @file   GimmickTestsHelper.h
 * @brief  기믹 모델 테스트용 Mock 정의 헤더
 * @author 이문환
 * @date   2026-08-21
 *
 * @details
 *  기믹의 GetTileMap()은 전투 월드 서브시스템에서 타일맵을 꺼내는데,
 *  자동화 테스트 월드에는 그 세팅 흐름이 없음.
 *  테스트가 NewObject로 만든 타일맵을 주입할 수 있게 GetTileMap()을 오버라이드.
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Actor/BoardActor/Obstacle/Gimmick/OverlapGimmickModel.h"
#include "SRPGFramework/EnemyTurnPlannerTestsHelper.h" // UMockEnemyUnitModel
#include "GimmickTestsHelper.generated.h"

class UTileMapModel;
class UBoardMovementComponentModel;

/**
 * @brief 진입 트리거 기믹 Mock
 * @details 서브시스템 대신 테스트가 주입한 타일맵 사용. 스킬/수명은 테스트가 직접 세팅
 */
UCLASS()
class UMockOverlapGimmickModel : public UOverlapGimmickModel
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

public:
	// @brief 테스트 타일맵 주입
	void SetTileMap(UTileMapModel* TileMap)
	{
		mTileMap = TileMap;
	}

	// @brief 남은 발동 횟수 설정 (스폰 데이터 대체)
	void SetRemainingTriggerCount(int32 Count)
	{
		mRemainingTriggerCount = Count;
	}

	// @brief 남은 발동 횟수 조회 (검증용)
	int32 GetRemainingTriggerCount() const
	{
		return mRemainingTriggerCount;
	}

protected:
	// @brief 주입된 타일맵 반환 (서브시스템 조회 대체)
	UTileMapModel* GetTileMap() const override
	{
		return mTileMap;
	}

private:
	// @brief 주입된 타일맵
	UPROPERTY()
	TObjectPtr<UTileMapModel> mTileMap;
};

/**
 * @brief 트랩을 밟는 피해자 유닛 Mock
 * @details Push 레이어가 대상의 이동 컴포넌트에서 타일맵을 얻으므로,
 *          이동 컴포넌트 조회를 주입본(타일맵 주입된 Mock)으로 교체
 */
UCLASS()
class UMockGimmickVictimUnitModel : public UMockEnemyUnitModel
{
	GENERATED_BODY()

public:
	// @brief 테스트 이동 컴포넌트 주입
	void SetBoardMovementComponentModel(UBoardMovementComponentModel* Movement)
	{
		mInjectedMovement = Movement;
	}

	// @brief 주입된 이동 컴포넌트 반환 (주입 전에는 기본 컴포넌트)
	UBoardMovementComponentModel* GetBoardMovementComponentModel() const override
	{
		return (mInjectedMovement != nullptr) ? mInjectedMovement.Get() : Super::GetBoardMovementComponentModel();
	}

private:
	// @brief 주입된 이동 컴포넌트
	UPROPERTY()
	TObjectPtr<UBoardMovementComponentModel> mInjectedMovement;
};
