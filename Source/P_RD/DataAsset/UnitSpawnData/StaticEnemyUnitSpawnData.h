/*****************************************************************//**
 * @file   StaticEnemyUnitSpawnData.h
 * @brief  적 유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "StaticEnemyUnitSpawnData.generated.h"

/**
 * @brief  적의 이동 성향 (이동 여부와 목적지를 정하는 기준)
 *
 * @details
 * 수치는 "0.45가 유지인가 접근인가"처럼 경계가 모호해, 명확한 3분류로 택1한다.
 * 추후 AI 고도화 시엔 이 enum을 시드로 float bias를 산출해 상황(아군 배치·피아 HP 등)과 합쳐 계산할 수 있다.
 */
UENUM(BlueprintType)
enum class EMoveTendency : uint8
{
	MoveAway        UMETA(DisplayName = "MoveAway",  ToolTip = "사거리를 유지하며 플레이어와 최대한 멀어짐"),
	HoldRange       UMETA(DisplayName = "HoldRange", ToolTip = "사거리 안에 들면 정지 (밖이면 사거리 안까지만 이동)"),
	MoveClose       UMETA(DisplayName = "MoveClose", ToolTip = "플레이어에게 최대한 붙음"),
};

/**
 * @brief  적 유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticEnemyUnitSpawnData : public UStaticUnitSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(UnitPrimaryAssetTypes::GetEnemyUnitType(), GetFName());
	}

	// @brief 기본 이동 성향: 멀어짐 / 사거리 유지(정지) / 붙음 중 택1
	// @note 사거리는 스킬(추후 스킬+장비+패시브)에서 나오므로 여기엔 눈금값을 두지 않는다.
	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MoveTendency"))
	EMoveTendency mMoveTendency = EMoveTendency::HoldRange;
	
	// @brief 이동포인트
	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MovePoint"))
	int32 mMovePoint{ 5 };
};
