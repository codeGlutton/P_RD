#include "RoguelikeStage/StageBuilder.h"

FStageBuilder FStageBuilder::Make()
{
	return FStageBuilder();
}

FStageBuilder FStageBuilder::Make(const FStageBuilderParams& Params)
{
	FStageBuilder Builder;
	Builder.mParams = Params;

	return Builder;
}

FStageBuilder& FStageBuilder::SetStageShape(int32 RowCount, int32 ColumnCount, int32 MaxPathCount)
{
	mParams.mRowCount = RowCount;
	mParams.mColumnCount = ColumnCount;
	mParams.mMaxPathCount = MaxPathCount;

	return *this;
}

FStageBuilder& FStageBuilder::SetRoomWeights(float MonsterRoomWeight, float EliteRoomWeight, float ShopRoomWeight)
{
	mParams.mMonsterRoomWeight = MonsterRoomWeight;
	mParams.mEliteRoomWeight = EliteRoomWeight;
	mParams.mShopRoomWeight = ShopRoomWeight;

	return *this;
}

FStage FStageBuilder::Build() const
{
	FStage NewStage;
	MakeAllRooms(NewStage);
	MakeStartingPoints(NewStage);
	MakeRoutes(NewStage);

	return MoveTemp(NewStage);
}

void FStageBuilder::MakeAllRooms(const FStage& Stage) const
{
	
}

void FStageBuilder::MakeStartingPoints(const FStage& Stage) const
{
}

void FStageBuilder::MakeRoutes(const FStage& Stage) const
{
}
