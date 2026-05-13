#include "PCGStage/StageBuilder.h"
#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

#include "PCGStage/Room.h"

#include "Engine/AssetManager.h"
#include "DataAsset/PrimaryAssetType.h"

FStageBuilder FStageBuilder::Make(const UObject* WorldContextObject)
{
	FStageBuilder Builder;
	Builder.mWorld = WorldContextObject->GetWorld();

	Builder.LoadAllAssetIds();

	return Builder;
}

FStageBuilder FStageBuilder::Make(const UObject* WorldContextObject, const FStageBuilderParams& Params)
{
	FStageBuilder Builder;
	Builder.mWorld = WorldContextObject->GetWorld();
	Builder.mParams = Params;

	Builder.LoadAllAssetIds();

	return Builder;
}

FStageBuilder& FStageBuilder::SetStageLevel(EStageLevelType StageLevel)
{
	mParams.mStageLevel = StageLevel;

	return *this;
}

FStageBuilder& FStageBuilder::SetStageShape(int32 RowCount, int32 ColumnCount, int32 MaxPathCount, int32 MinStartPointCount, int32 MaxStartPointCount)
{
	mParams.mRowCount = RowCount;
	mParams.mColumnCount = ColumnCount;
	mParams.mMaxPathCount = MaxPathCount;
	mParams.mMinStartPointCount = MinStartPointCount;
	mParams.mMaxStartPointCount = MaxStartPointCount;

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
	MakeEmptyRooms(NewStage);
	MakeStartingPoints(NewStage);
	MakeRoutes(NewStage);

	return MoveTemp(NewStage);
}

void FStageBuilder::LoadAllAssetIds()
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	AssetManager->GetPrimaryAssetIdList(RoomPrimaryAssetTypes::GetTreasureRoomType(mParams.mStageLevel), mTreasureRoomAssetIds);
	AssetManager->GetPrimaryAssetIdList(RoomPrimaryAssetTypes::GetShopRoomType(mParams.mStageLevel), mShopRoomAssetIds);
	AssetManager->GetPrimaryAssetIdList(RoomPrimaryAssetTypes::GetMonsterRoomType(mParams.mStageLevel), mMonsterRoomAssetIds);
	AssetManager->GetPrimaryAssetIdList(RoomPrimaryAssetTypes::GetEliteMonsterRoomType(mParams.mStageLevel), mEliteMonsterRoomAssetIds);
	AssetManager->GetPrimaryAssetIdList(RoomPrimaryAssetTypes::GetBossMonsterRoomType(mParams.mStageLevel), mBossMonsterRoomAssetIds);

	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetWeaponType(ERarityType::Common), mCommonWeaponAssetIds);
	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetWeaponType(ERarityType::Rare), mRareWeaponAssetIds);
	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetWeaponType(ERarityType::Epic), mEpicWeaponAssetIds);

	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetGlovesType(ERarityType::Common), mCommonGlovesAssetIds);
	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetGlovesType(ERarityType::Rare), mRareGlovesAssetIds);
	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetGlovesType(ERarityType::Epic), mEpicGlovesAssetIds);

	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetBootsType(ERarityType::Common), mCommonBootsAssetIds);
	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetBootsType(ERarityType::Rare), mRareBootsAssetIds);
	AssetManager->GetPrimaryAssetIdList(EquipmentPrimaryAssetTypes::GetBootsType(ERarityType::Epic), mEpicBootsAssetIds);

	AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetAttackType(ERarityType::Common), mCommonAttackSkillAssetIds);
	AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetAttackType(ERarityType::Rare), mRareAttackSkillAssetIds);
	AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetAttackType(ERarityType::Epic), mEpicAttackSkillAssetIds);

	AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetSpellType(ERarityType::Common), mCommonSpellSkillAssetIds);
	AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetSpellType(ERarityType::Rare), mRareSpellSkillAssetIds);
	AssetManager->GetPrimaryAssetIdList(SkillPrimaryAssetTypes::GetSpellType(ERarityType::Epic), mEpicSpellSkillAssetIds);

	AssetManager->GetPrimaryAssetIdList(DicePrimaryAssetTypes::GetDiceType(ERarityType::Common), mCommonDiceAssetIds);
	AssetManager->GetPrimaryAssetIdList(DicePrimaryAssetTypes::GetDiceType(ERarityType::Rare), mRareDiceAssetIds);
	AssetManager->GetPrimaryAssetIdList(DicePrimaryAssetTypes::GetDiceType(ERarityType::Epic), mEpicDiceAssetIds);
}

void FStageBuilder::MakeEmptyRooms(OUT FStage& Stage) const
{
	const int32 RowCount = mParams.mRowCount;
	const int32 ColumnCount = mParams.mColumnCount;

	FStageRow Row;
	Row.mRooms.Init(TInstancedStruct<FRoom>(nullptr), ColumnCount);
	Stage.mRoomRows.Init(Row, RowCount);
}

void FStageBuilder::MakeStartingPoints(OUT FStage& Stage) const
{
	const FRandomStream& BuildStream = URandomStreamFunctionLibrary::GetStageBuildStream(mWorld);
	const int32 StartPointCount = BuildStream.RandRange(mParams.mMinStartPointCount, mParams.mMaxStartPointCount);
	const int32 MaxPathCount = mParams.mMaxPathCount;
	const int32 ColumnCount = mParams.mColumnCount;

	checkf(StartPointCount < ColumnCount, TEXT("시작점이 최대 가로 점보다 많아질 수 없습니다"));
	checkf(StartPointCount < MaxPathCount, TEXT("시작점이 최대 경로보다 많아질 수 없습니다"));

	const int32 StartColumn = Stage.mStartColumn = ColumnCount / 2;
	FRoom& StartRoom = CreateRoom(ERoomType::Monster, 0, StartColumn, Stage.mRoomRows[0].mRooms[StartColumn]);

	TArray<int32>& StartPoints = StartRoom.mNextRoomColumns;
	StartPoints.Reserve(FMath::Max(ColumnCount, MaxPathCount));

	// 칼럼 인덱스 채우기
	for (int32 i = 0; i < ColumnCount; ++i)
	{
		StartPoints.Push(i);
	}
	// 하나씩 빼가면서 모든 시작점 결정
	for (int32 i = StartPointCount; i < ColumnCount; ++i)
	{
		const int32 RemoveIndex = BuildStream.RandRange(0, StartPoints.Num() - 1);
		StartPoints.RemoveAtSwap(RemoveIndex, EAllowShrinking::No);
	}
	// 각 경로의 시작점 결정
	for (int32 i = StartPointCount; i < MaxPathCount; ++i)
	{
		const int32 PathStartPoint = StartPoints[BuildStream.RandRange(0, StartPointCount - 1)];
		StartPoints.Push(PathStartPoint);
	}
}

void FStageBuilder::MakeRoutes(OUT FStage& Stage) const
{
}

FRoom& FStageBuilder::CreateRoom(ERoomType Type, int32 Row, int32 Column, TInstancedStruct<FRoom>& Room) const
{
	const FRandomStream& BuildStream = URandomStreamFunctionLibrary::GetStageBuildStream(mWorld);

	FRoom* NewRoomPtr = nullptr;
	switch (Type)
	{
	case ERoomType::Treasure:
	{
		Room.InitializeAs<FTreasureRoom>();
		auto& NewRoom = Room.GetMutable<FTreasureRoom>();
		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::Shop:
	{
		Room.InitializeAs<FShopRoom>();
		auto& NewRoom = Room.GetMutable<FShopRoom>();
		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::Monster:
	{
		Room.InitializeAs<FMonsterRoom>();
		auto& NewRoom = Room.GetMutable<FMonsterRoom>();
		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::EliteMonster:
	{
		Room.InitializeAs<FEliteMonsterRoom>();
		auto& NewRoom = Room.GetMutable<FEliteMonsterRoom>();
		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::BossMonster:
	{
		Room.InitializeAs<FBossMonsterRoom>();
		auto& NewRoom = Room.GetMutable<FBossMonsterRoom>();
		NewRoomPtr = &NewRoom;
		break;
	}
	}
	checkf(NewRoomPtr != nullptr, TEXT("알 수 없는 방 타입 오류"));

	NewRoomPtr->mRow = Row;
	NewRoomPtr->mColumn = Column;
	NewRoomPtr->mPositionOffsetRate = FVector2D(BuildStream.FRandRange(-1., 1.), BuildStream.FRandRange(-1., 1.));

	return *NewRoomPtr;
}
