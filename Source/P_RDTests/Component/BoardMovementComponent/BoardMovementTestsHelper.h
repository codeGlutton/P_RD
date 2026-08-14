/*****************************************************************//**
 * @file   BoardMovementTestsHelper.h
 * @brief  보드 이동 컴포넌트 테스트용 Mock 정의 헤더
 * @author 이문환
 * @date   2026-08-13
 *
 * @details
 *  이동 컴포넌트의 GetTileMap()은 전투 월드 서브시스템에서 타일맵을 꺼내는데,
 *  자동화 테스트 월드에는 그 세팅 흐름이 없음.
 *  테스트가 NewObject로 만든 타일맵을 주입할 수 있게 GetTileMap()을 오버라이드.
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Component/BoardMovementComponent/UnitMovementComponentModel.h"
#include "BoardMovementTestsHelper.generated.h"

class UTileMapModel;

/**
 * @brief 유닛 이동 컴포넌트 Mock
 * @details 서브시스템 대신 테스트가 주입한 타일맵 사용. AP 차감 등 유닛 규칙은 실제와 동일
 */
UCLASS()
class UMockUnitMovementComponentModel : public UUnitMovementComponentModel
{
	GENERATED_BODY()

public:
	// @brief 테스트 타일맵 주입
	void SetTileMap(UTileMapModel* TileMap)
	{
		mTileMap = TileMap;
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
