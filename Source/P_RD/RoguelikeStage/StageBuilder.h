/*****************************************************************//**
 * @file   StageBuilder.h
 * @brief  스테이지 내 방들을 생성해주는 빌더 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "RoguelikeStage/Stage.h"

struct FStageBuilderParams
{
public:
	int32 mRowCount = 7;
	int32 mColumnCount = 15;
	int32 mMaxPathCount = 6;

public:
	float mMonsterRoomWeight = 10.f;
	float mEliteRoomWeight = 2.f;
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
	static FStageBuilder Make();
	static FStageBuilder Make(const FStageBuilderParams& Params);
	FStageBuilder& SetStageShape(int32 RowCount, int32 ColumnCount, int32 MaxPathCount);
	FStageBuilder& SetRoomWeights(float MonsterRoomWeight, float EliteRoomWeight, float ShopRoomWeight);
	FStage Build() const;

protected:
	void MakeAllRooms(const FStage& Stage) const;
	void MakeStartingPoints(const FStage& Stage) const;
	void MakeRoutes(const FStage& Stage) const;

protected:
	FStageBuilderParams mParams;
};
