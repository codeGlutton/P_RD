/*****************************************************************//**
 * @file   StageBuilderParams.h
 * @brief  스테이지 내 방들을 생성해주는 설정 값 TableRow 정의 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "PCGStage/RoomType.h"
#include "DataAsset/RarityRate.h"
#include "StageBuilderParams.generated.h"

/**
 * @brief 방을 생성할 때 사용되는 방 타입 열거형
 */
UENUM(BlueprintType)
enum class ERoomBuildType : uint8
{
	Shop = 0,
	Monster,
	EliteMonster,
	Count			UMETA(Hidden),
};

USTRUCT(BlueprintType)
struct FRoomBuildRate
{
	GENERATED_BODY()

public:
	float GetRate(ERoomBuildType Type) const;
	float GetRate(ERoomBuildType Type, const TArray<ERoomBuildType>& IncludedTypes) const;
	ERoomType GetType(float Rate) const;
	ERoomType GetType(float Rate, const TArray<ERoomBuildType>& IncludedTypes) const;

protected:
	float GetTotalWeight() const;
	float GetTotalWeight(const TArray<ERoomBuildType>& IncludedTypes) const;
	ERoomType ConventToRoomType(ERoomBuildType Type) const;

public:
	UPROPERTY(Category = "Weight", EditAnywhere, meta = (DisplayName = "Weights", ArraySizeEnum = "ERoomBuildType"))
	float mWeights[static_cast<uint8>(ERoomBuildType::Count)] = { 4.5f, 10.f, 2.f };
};

/**
 * @brief  스테이지 내 방들을 생성해주는 설정 값 TableRow
 */
USTRUCT(BlueprintType)
struct FStageBuilderParams : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Level", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StageLevel"))
	EStageLevelType mStageLevel = EStageLevelType::None;

	UPROPERTY(Category = "Level", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StaticStageSpawnDataId"))
	FPrimaryAssetId mStaticStageSpawnDataId;

public:
	UPROPERTY(Category = "Shape", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RowCount", ClampMin = 3, UIMin = 3, SliderExponent = 1))
	int32 mRowCount = 15;
	UPROPERTY(Category = "Shape", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ColumnCount", ClampMin = 1, UIMin = 1, SliderExponent = 2))
	int32 mColumnCount = 7;
	UPROPERTY(Category = "Shape", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MaxPathCount", ClampMin = 1, UIMin = 1, SliderExponent = 1))
	int32 mMaxPathCount = 5;
	UPROPERTY(Category = "Shape", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MinStartPointCount", ClampMin = 1, UIMin = 1, SliderExponent = 1))
	int32 mMinStartPointCount = 2;
	UPROPERTY(Category = "Shape", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MaxStartPointCount", ClampMin = 1, UIMin = 1, SliderExponent = 1))
	int32 mMaxStartPointCount = 4;

public:
	UPROPERTY(Category = "Rate", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RoomBuildRate"))
	FRoomBuildRate mRoomBuildRate = { 4.5f, 10.f, 2.f };

public:
	UPROPERTY(Category = "Rate", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillRarityRate"))
	FRarityRate mSkillRarityRate = { 10.f, 5.f, 1.f };

	UPROPERTY(Category = "Rate", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EquipmentRarityRate"))
	FRarityRate mEquipmentRarityRate = { 10.f, 5.f, 1.f };

	UPROPERTY(Category = "Rate", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceRarityRate"))
	FRarityRate mDiceRarityRate = { 10.f, 5.f, 1.f };
};

