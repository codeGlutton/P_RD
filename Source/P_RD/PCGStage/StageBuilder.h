/*****************************************************************//**
 * @file   StageBuilder.h
 * @brief  스테이지 내 방들을 생성해주는 빌더 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/Stage.h"
#include "PCGStage/StageLevelType.h"
#include "StageBuilder.generated.h"

USTRUCT(BlueprintType)
struct FStageBuilderParams
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Level", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StageLevel"))
	EStageLevelType mStageLevel;

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
	UPROPERTY(Category = "Weight", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MonsterRoomWeight"))
	float mMonsterRoomWeight = 10.f;
	UPROPERTY(Category = "Weight", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EliteRoomWeight"))
	float mEliteRoomWeight = 2.f;
	UPROPERTY(Category = "Weight", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ShopRoomWeight"))
	float mShopRoomWeight = 4.5f;
};

/**
 * @brief  스테이지 내 방들을 생성해주는 빌더 객체
 */
struct FStageBuilder
{
private:
	FStageBuilder() = default;

public:
	static FStageBuilder Make(const UObject* WorldContextObject);
	static FStageBuilder Make(const UObject* WorldContextObject, const FStageBuilderParams& Params);
	FStageBuilder& SetStageLevel(EStageLevelType StageLevel);
	FStageBuilder& SetStageShape(int32 RowCount, int32 ColumnCount, int32 MaxPathCount, int32 MinStartPointCount, int32 MaxStartPointCount);
	FStageBuilder& SetRoomWeights(float MonsterRoomWeight, float EliteRoomWeight, float ShopRoomWeight);
	FStage Build() const;

protected:
	/**
	 * 방 랜덤 요소를 정하기 위해 미리 에셋 모든 ID 배열을 캐싱해두는 함수
	 */
	void LoadAllAssetIds();

protected:
	void MakeEmptyRooms(OUT FStage& Stage) const;
	void MakeStartingPoints(OUT FStage& Stage) const;
	void MakeRoutes(OUT FStage& Stage) const;

protected:
	FRoom& CreateRoom(ERoomType Type, int32 Row, int32 Column, TInstancedStruct<FRoom>& Room) const;

protected:
	const UWorld* mWorld;
	FStageBuilderParams mParams;

protected:
	TArray<FPrimaryAssetId> mTreasureRoomAssetIds;
	TArray<FPrimaryAssetId> mShopRoomAssetIds;
	TArray<FPrimaryAssetId> mMonsterRoomAssetIds;
	TArray<FPrimaryAssetId> mEliteMonsterRoomAssetIds;
	TArray<FPrimaryAssetId> mBossMonsterRoomAssetIds;

	TArray<FPrimaryAssetId> mCommonWeaponAssetIds;
	TArray<FPrimaryAssetId> mRareWeaponAssetIds;
	TArray<FPrimaryAssetId> mEpicWeaponAssetIds;
	TArray<FPrimaryAssetId> mCommonGlovesAssetIds;
	TArray<FPrimaryAssetId> mRareGlovesAssetIds;
	TArray<FPrimaryAssetId> mEpicGlovesAssetIds;
	TArray<FPrimaryAssetId> mCommonBootsAssetIds;
	TArray<FPrimaryAssetId> mRareBootsAssetIds;
	TArray<FPrimaryAssetId> mEpicBootsAssetIds;

	TArray<FPrimaryAssetId> mCommonAttackSkillAssetIds;
	TArray<FPrimaryAssetId> mRareAttackSkillAssetIds;
	TArray<FPrimaryAssetId> mEpicAttackSkillAssetIds;
	TArray<FPrimaryAssetId> mCommonSpellSkillAssetIds;
	TArray<FPrimaryAssetId> mRareSpellSkillAssetIds;
	TArray<FPrimaryAssetId> mEpicSpellSkillAssetIds;

	TArray<FPrimaryAssetId> mCommonDiceAssetIds;
	TArray<FPrimaryAssetId> mRareDiceAssetIds;
	TArray<FPrimaryAssetId> mEpicDiceAssetIds;
};
