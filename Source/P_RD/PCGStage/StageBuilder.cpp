#include "PCGStage/StageBuilder.h"

#include "PCGStage/Room.h"

#include "Engine/AssetManager.h"
#include "DataAsset/PrimaryAssetType.h"

#include "FunctionLibrary/RandomStreamFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogStageBuilder)

FStageBuilder::FStageBuilder(const FRandomStream& BuildStream, const FGlobalStageBuildSetting& GlobalSetting) :
	mBuildStream(BuildStream),
	mGlobalSetting(GlobalSetting)
{
}

FStageBuilder FStageBuilder::Make(const FRandomStream& BuildStream, const FGlobalStageBuildSetting& GlobalSetting)
{
	FStageBuilder Builder(BuildStream, GlobalSetting);

	return Builder;
}

FStageBuilder FStageBuilder::Make(const FRandomStream& BuildStream, const FGlobalStageBuildSetting& GlobalSetting, const FStageBuilderParams& Params)
{
	FStageBuilder Builder = FStageBuilder::Make(BuildStream, GlobalSetting);
	Builder.SetParams(Params);

	return Builder;
}

FStageBuilder& FStageBuilder::SetParams(const FStageBuilderParams& Params)
{
	bool IsStageChanged = mParams.mStageLevel != Params.mStageLevel;

	mParams = Params;
	if (mIsLoadedIds == false || IsStageChanged == true)
	{
		LoadAllAssetIds();
	}

	return *this;
}

FStage FStageBuilder::Build() const
{
	FStage NewStage;
	Build(OUT NewStage);
	return NewStage;
}

void FStageBuilder::Build(OUT FStage& NewStage) const
{
	InitStage(OUT NewStage);

	TArray<FRoomEdge> Edges;
	CreateStartRoom(OUT NewStage);
	Edges = MakeStartRoutes(OUT NewStage);

	const int32 RowCount = mParams.mRowCount;
	for (int32 RowIndex = 1; RowIndex < RowCount - 1; ++RowIndex)
	{
		CreateRooms(OUT NewStage, RowIndex, Edges);
		Edges = MakeNextRoutes(OUT NewStage, RowIndex, Edges);
	}

	CreateRooms(OUT NewStage, RowCount - 1, Edges);

	/* 디버깅 */
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	for (auto Iter = NewStage.mRoomRows.rbegin(); Iter != NewStage.mRoomRows.rend(); ++Iter)
	{
		FString RowStr = TEXT("");
		for (const TInstancedStruct<FRoom>& RoomInst : Iter->mRooms)
		{
			ERoomType Type = ERoomType::None;
			FString LinkRoomStr = TEXT("");
			if (const FRoom* Room = RoomInst.GetPtr<FRoom>())
			{
				Type = Room->mType;
				for (int32 NextColumn : Room->mNextRoomColumns)
				{
					LinkRoomStr += FString::FromInt(NextColumn) + TEXT(",");
				}
			}
			RowStr.Append(FString::Printf(TEXT(" [%-12s](%-7s) "), *EnumToString(Type), *LinkRoomStr));
		}
		UE_LOG(LogStageBuilder, Log, TEXT("(%s)"), *RowStr);
	}
#endif
}

void FStageBuilder::LoadAllAssetIds()
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	auto StageFilter = [FilterStr = EnumToString(mParams.mStageLevel)](const FAssetData& AssetData) -> bool {
		FString FoundStr;
		bool IsFound = AssetData.GetTagValue(TEXT("mStageLevel"), OUT FoundStr) == true;
		return IsFound == true && FoundStr == FilterStr;
		};

	mRoomAssetIds[static_cast<uint8>(ERoomType::Treasure)] = GetFilteredPrimaryAssets(RoomPrimaryAssetTypes::GetTreasureRoomType(), StageFilter);
	mRoomAssetIds[static_cast<uint8>(ERoomType::Shop)] = GetFilteredPrimaryAssets(RoomPrimaryAssetTypes::GetShopRoomType(), StageFilter);
	mRoomAssetIds[static_cast<uint8>(ERoomType::Monster)] = GetFilteredPrimaryAssets(RoomPrimaryAssetTypes::GetMonsterRoomType(), StageFilter);
	mRoomAssetIds[static_cast<uint8>(ERoomType::EliteMonster)] = GetFilteredPrimaryAssets(RoomPrimaryAssetTypes::GetEliteMonsterRoomType(), StageFilter);
	mRoomAssetIds[static_cast<uint8>(ERoomType::BossMonster)] = GetFilteredPrimaryAssets(RoomPrimaryAssetTypes::GetBossMonsterRoomType(), StageFilter);

	const uint8 RarityTypeCount = StaticCast<uint8>(ERarityType::Count);
	for (uint8 RarityTypeIndex = 0; RarityTypeIndex < RarityTypeCount; ++RarityTypeIndex)
	{
		auto RarityFilter = [FilterStr = EnumToString(StaticCast<ERarityType>(RarityTypeIndex))](const FAssetData& AssetData) -> bool {
			FString FoundStr;
			bool IsFound = AssetData.GetTagValue(TEXT("mRarityType"), OUT FoundStr) == true;
			return IsFound == true && FoundStr == FilterStr;
			};
		mEquipmentAssetIds[RarityTypeIndex] = GetFilteredPrimaryAssets(EquipmentPrimaryAssetTypes::GetEquipmentType(), RarityFilter);
		const TArray<FAssetData> SkillAssetDatas = GetFilteredPrimaryAssetDatas(SkillPrimaryAssetTypes::GetActiveType(), RarityFilter);
		
		const uint8 JobTypeCount = StaticCast<uint8>(EUnitJobType::PlayerJobCount);
		for (uint8 JobTypeIndex = 0; JobTypeIndex < JobTypeCount; ++JobTypeIndex)
		{
			auto JobFilter = [FilterStr = EnumToString(StaticCast<EUnitJobType>(JobTypeIndex))](const FAssetData& AssetData) -> bool {
				FString FoundStr;
				bool IsFound = AssetData.GetTagValue(TEXT("mJobType"), OUT FoundStr) == true;
				return IsFound == true && FoundStr == FilterStr;
				};

			mJobSkillAssetIds[JobTypeIndex][RarityTypeIndex] = GetFilteredPrimaryAssets(SkillAssetDatas, JobFilter);
		}

		auto JobFilter = [FilterStr = EnumToString(EUnitJobType::Common)](const FAssetData& AssetData) -> bool {
			FString FoundStr;
			bool IsFound = AssetData.GetTagValue(TEXT("mJobType"), OUT FoundStr) == true;
			return IsFound == true && FoundStr == FilterStr;
			};
		mCommonSkillAssetIds[RarityTypeIndex] = GetFilteredPrimaryAssets(SkillAssetDatas, JobFilter);

	}

	mIsLoadedIds = true;
}

void FStageBuilder::InitStage(OUT FStage& Stage) const
{
	const int32 RowCount = mParams.mRowCount;
	const int32 ColumnCount = mParams.mColumnCount;

	Stage.mStageLevel = mParams.mStageLevel;
	Stage.mStaticStageSpawnDataId = mParams.mStaticStageSpawnDataId;

	FRoomRow Row;
	Row.mRooms.Init(TInstancedStruct<FRoom>(), ColumnCount);
	Stage.mRoomRows.Init(Row, RowCount);
}

TArray<FRoomEdge> FStageBuilder::MakeStartRoutes(OUT FStage& Stage) const
{
	const int32 StartPointCount = mBuildStream.RandRange(mParams.mMinStartPointCount, mParams.mMaxStartPointCount);
	const int32 MaxPathCount = mParams.mMaxPathCount;
	const int32 ColumnCount = mParams.mColumnCount;

	checkf(StartPointCount < ColumnCount, TEXT("시작점이 최대 가로 점보다 많아질 수 없습니다"));
	checkf(StartPointCount < MaxPathCount, TEXT("시작점이 최대 경로보다 많아질 수 없습니다"));

	TArray<int32> StartPoints;
	StartPoints.Reserve(FMath::Max(ColumnCount, MaxPathCount));
	
	/* 칼럼 인덱스 채우기 */
	for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		StartPoints.Push(ColumnIndex);
	}
	/* 하나씩 빼가면서 모든 시작방에서 도달할 수 있는 일반 방 결정 */
	for (int32 i = StartPointCount; i < ColumnCount; ++i)
	{
		const int32 RemoveIndex = mBuildStream.RandRange(0, StartPoints.Num() - 1);
		StartPoints.RemoveAtSwap(RemoveIndex, EAllowShrinking::No);
	}

	FRoom& StartRoom = Stage.mRoomRows[0].mRooms[Stage.mStartColumn].GetMutable<FRoom>();
	StartRoom.mNextRoomColumns = StartPoints;

	/* 시작 방 -> 일반 방의 모든 경로 결정 */
	for (int32 i = StartPointCount; i < MaxPathCount; ++i)
	{
		const int32 PathStartPoint = StartPoints[mBuildStream.RandRange(0, StartPointCount - 1)];
		StartPoints.Push(PathStartPoint);
	}

	/* 방 생성 데이터로 사용될 Edge 반환 데이터 채우기 */
	TArray<FRoomEdge> NewEdges;
	NewEdges.Init(FRoomEdge(), ColumnCount);
	for (int32 i = 0; i < MaxPathCount; ++i)
	{
		NewEdges[StartPoints[i]].mPreRoomColumns.Push(Stage.mStartColumn);
	}
	return NewEdges;
}

TArray<FRoomEdge> FStageBuilder::MakeNextRoutes(OUT FStage& Stage, int32 CurRowIndex, const TArray<FRoomEdge>& CurEdges) const
{
	const int32 ColumnCount = mParams.mColumnCount;
	const int32 RowCount = mParams.mRowCount;

	TArray<FRoomEdge> NewEdges;
	NewEdges.Init(FRoomEdge(), ColumnCount);

	if (CurRowIndex == RowCount - 2)
	{
		/* 상점 방 -> 보스 방 연결 */

		for (int32 CurColumnIndex = 0; CurColumnIndex < ColumnCount; ++CurColumnIndex)
		{
			const TArray<int32>& PreRoomColumns = CurEdges[CurColumnIndex].mPreRoomColumns;
			if (PreRoomColumns.IsEmpty() == true)
			{
				continue;
			}

			TInstancedStruct<FRoom>& CurRoomInst = Stage.mRoomRows[CurRowIndex].mRooms[CurColumnIndex];
			FRoom* CurRoomPtr = CurRoomInst.GetMutablePtr<FRoom>();
			CurRoomPtr->mNextRoomColumns.Push(Stage.mStartColumn);

			for (int32 PreRoomColumnIndex : PreRoomColumns)
			{
				NewEdges[Stage.mStartColumn].mPreRoomColumns.Push(CurColumnIndex);
			}
		}
		return NewEdges;
	}

	/* 일반 방 -> 일반 방 연결 */

	int32 MaxAllocatedColumnIndex = 0;

	for (int32 CurColumnIndex = 0; CurColumnIndex < ColumnCount; ++CurColumnIndex)
	{
		const TArray<int32>& PreRoomColumns = CurEdges[CurColumnIndex].mPreRoomColumns;
		if (PreRoomColumns.IsEmpty() == true)
		{
			continue;
		}

		const int32 MinNewColumnIndex = FMath::Max(MaxAllocatedColumnIndex, CurColumnIndex - 1);
		const int32 MaxNewColumnIndex = FMath::Min(CurColumnIndex + 1, ColumnCount - 1);

		TInstancedStruct<FRoom>& CurRoomInst = Stage.mRoomRows[CurRowIndex].mRooms[CurColumnIndex];
		FRoom* CurRoomPtr = CurRoomInst.GetMutablePtr<FRoom>();
		TSet<int32> NextRoomColumns;

		for (int32 PreRoomColumnIndex : PreRoomColumns)
		{
			int32 NewColumnIndex = mBuildStream.RandRange(MinNewColumnIndex, MaxNewColumnIndex);
			NewEdges[NewColumnIndex].mPreRoomColumns.Push(CurColumnIndex);
			NextRoomColumns.Add(NewColumnIndex);

			if (MaxAllocatedColumnIndex < NewColumnIndex)
			{
				MaxAllocatedColumnIndex = NewColumnIndex;
			}
		}

		CurRoomPtr->mNextRoomColumns = NextRoomColumns.Array();
	}

	return NewEdges;
}

void FStageBuilder::CreateStartRoom(OUT FStage& Stage) const
{
	const int32 ColumnCount = mParams.mColumnCount;
	const int32 StartColumn = Stage.mStartColumn = ColumnCount / 2;
	FRoom& StartRoom = CreateRoom(ERoomType::Monster, 0, StartColumn, Stage.mRoomRows[0].mRooms[StartColumn]);

	Stage.mCurRow = 0;
	Stage.mCurColumn = StartColumn;
}

void FStageBuilder::CreateRooms(OUT FStage& Stage, int32 CurRowIndex, const TArray<FRoomEdge>& CurEdges) const
{
	const int32 ColumnCount = mParams.mColumnCount;
	const int32 RowCount = mParams.mRowCount;

	/* 특정 타입의 행 검사 */

	ERoomType NewRoomType = ERoomType::None;
	if (CurRowIndex == RowCount - 1)
	{
		NewRoomType = ERoomType::BossMonster;
	}
	else if (CurRowIndex == RowCount - 2)
	{
		NewRoomType = ERoomType::Shop;
	}
	else if (CurRowIndex % mGlobalSetting.mTreasureColumnInterval == 0)
	{
		NewRoomType = ERoomType::Treasure;
	}

	if (NewRoomType == ERoomType::None)
	{
		/* 특정 타입의 행이 아닌 경우, 각 방의 루트를 검사해서 적절한 방 타입 도입 */

		const bool IsRowInShopRange = CurRowIndex > mGlobalSetting.mBottomShopColumnOffset && CurRowIndex < (RowCount - mGlobalSetting.mTopShopColumnOffset);
		for (int32 CurColumnIndex = 0; CurColumnIndex < ColumnCount; ++CurColumnIndex)
		{
			const TArray<int32>& PreRoomColumns = CurEdges[CurColumnIndex].mPreRoomColumns;
			if (PreRoomColumns.IsEmpty() == true)
			{
				continue;
			}

			bool IsPreRoomShop = false;
			bool IsPreRoomElite = false;
			for (int32 PreRoomColumnIndex : PreRoomColumns)
			{
				TInstancedStruct<FRoom>& PreRoomInst = Stage.mRoomRows[CurRowIndex - 1].mRooms[PreRoomColumnIndex];
				FRoom* PreRoomPtr = PreRoomInst.GetMutablePtr<FRoom>();
				if (PreRoomPtr == nullptr)
				{
					continue;
				}

				if (PreRoomPtr->mType == ERoomType::Shop)
				{
					IsPreRoomShop = true;
				}
				if (PreRoomPtr->mType == ERoomType::EliteMonster)
				{
					IsPreRoomElite = true;
				}
			}

			TArray<ERoomBuildType> CandidateRoomTypes;
			CandidateRoomTypes.Push(ERoomBuildType::Monster);
			if (IsRowInShopRange == true && IsPreRoomShop == false)
			{
				CandidateRoomTypes.Push(ERoomBuildType::Shop);
			}
			if (IsPreRoomElite == false)
			{
				CandidateRoomTypes.Push(ERoomBuildType::EliteMonster);
			}

			// 후보들 중에 랜덤 배정
			NewRoomType = mParams.mRoomBuildRate.GetType(mBuildStream.FRand(), CandidateRoomTypes);
			
			TInstancedStruct<FRoom>& NewRoomInst = Stage.mRoomRows[CurRowIndex].mRooms[CurColumnIndex];
			FRoom& NewRoom = CreateRoom(NewRoomType, CurRowIndex, CurColumnIndex, NewRoomInst);
		}
	}
	else
	{
		/* 정해진 타입의 행인 경우 */

		for (int32 CurColumnIndex = 0; CurColumnIndex < ColumnCount; ++CurColumnIndex)
		{
			const TArray<int32>& PreRoomColumns = CurEdges[CurColumnIndex].mPreRoomColumns;
			if (PreRoomColumns.IsEmpty() == true)
			{
				continue;
			}

			TInstancedStruct<FRoom>& NewRoomInst = Stage.mRoomRows[CurRowIndex].mRooms[CurColumnIndex];
			FRoom& NewRoom = CreateRoom(NewRoomType, CurRowIndex, CurColumnIndex, NewRoomInst);
		}
	}
}

FRoom& FStageBuilder::CreateRoom(ERoomType Type, int32 Row, int32 Column, TInstancedStruct<FRoom>& Room) const
{
	FRoom* NewRoomPtr = nullptr;
	switch (Type)
	{
	case ERoomType::Treasure:
	{
		Room.InitializeAs<FTreasureRoom>();
		auto& NewRoom = Room.GetMutable<FTreasureRoom>();

		const uint8 RarityTypeIndex = GetRandomRarityIndex(mParams.mEquipmentRarityRate);
		const TArray<FPrimaryAssetId>& EquipmentIdArray = mEquipmentAssetIds[RarityTypeIndex];
		NewRoom.mRewardEquipmentDataId = URandomStreamFunctionLibrary::GetRandomItem(mBuildStream, EquipmentIdArray);
		
		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::Shop:
	{
		Room.InitializeAs<FShopRoom>();
		auto& NewRoom = Room.GetMutable<FShopRoom>();

		const uint8 JobTypeCount = StaticCast<uint8>(EUnitJobType::PlayerJobCount);
		for (uint8 JobTypeIndex = 0; JobTypeIndex < JobTypeCount; ++JobTypeIndex)
		{
			for (int32 i = 0; i < 2; ++i)
			{
				const uint8 RarityTypeIndex = GetRandomRarityIndex(mParams.mSkillRarityRate);
				const TArray<FPrimaryAssetId>& SkillIdArray = mJobSkillAssetIds[JobTypeIndex][RarityTypeIndex];
				NewRoom.mSaleJobSkillDataItems[JobTypeIndex].mSaleItemIds.Push(URandomStreamFunctionLibrary::GetRandomItem(mBuildStream, SkillIdArray));
			}
		}
		for (int32 i = 0; i < 3; ++i)
		{
			const uint8 RarityTypeIndex = GetRandomRarityIndex(mParams.mSkillRarityRate);
			const TArray<FPrimaryAssetId>& SkillIdArray = mCommonSkillAssetIds[RarityTypeIndex];
			NewRoom.mSaleCommonSkillDataItems.mSaleItemIds.Push(URandomStreamFunctionLibrary::GetRandomItem(mBuildStream, SkillIdArray));
		}

		for (int32 i = 0; i < 3; ++i)
		{
			const uint8 RarityTypeIndex = GetRandomRarityIndex(mParams.mEquipmentRarityRate);
			const TArray<FPrimaryAssetId>& EquipmentIdArray = mEquipmentAssetIds[RarityTypeIndex];
			NewRoom.mSaleEquipmentDataItems.mSaleItemIds.Push(URandomStreamFunctionLibrary::GetRandomItem(mBuildStream, EquipmentIdArray));
		}

		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::Monster:
	{
		Room.InitializeAs<FMonsterRoom>();
		auto& NewRoom = Room.GetMutable<FMonsterRoom>();

		NewRoom.mRewardMoney = URandomStreamFunctionLibrary::GetRandomFromInterval(mBuildStream, mGlobalSetting.mMonsterRewardMoney);
		NewRoom.mRewardExp = mGlobalSetting.mMonsterRewardExp;

		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::EliteMonster:
	{
		Room.InitializeAs<FEliteMonsterRoom>();
		auto& NewRoom = Room.GetMutable<FEliteMonsterRoom>();

		NewRoom.mRewardMoney = URandomStreamFunctionLibrary::GetRandomFromInterval(mBuildStream, mGlobalSetting.mEliteRewardMoney);
		NewRoom.mRewardExp = mGlobalSetting.mEliteRewardExp;

		const uint8 RarityTypeIndex = GetRandomRarityIndex(mParams.mEquipmentRarityRate);
		const TArray<FPrimaryAssetId>& EquipmentIdArray = mEquipmentAssetIds[RarityTypeIndex];
		NewRoom.mRewardEquipmentDataId = URandomStreamFunctionLibrary::GetRandomItem(mBuildStream, EquipmentIdArray);

		NewRoomPtr = &NewRoom;
		break;
	}
	case ERoomType::BossMonster:
	{
		Room.InitializeAs<FBossMonsterRoom>();
		auto& NewRoom = Room.GetMutable<FBossMonsterRoom>();

		NewRoom.mRewardMoney = URandomStreamFunctionLibrary::GetRandomFromInterval(mBuildStream, mGlobalSetting.mBossRewardMoney);
		NewRoom.mRewardExp = mGlobalSetting.mBossRewardExp;

		NewRoomPtr = &NewRoom;
		break;
	}
	}
	checkf(NewRoomPtr != nullptr, TEXT("알 수 없는 방 타입 오류"));

	const TArray<FPrimaryAssetId>& RoomIdArray = mRoomAssetIds[static_cast<uint8>(Type)];
	NewRoomPtr->mRow = Row;
	NewRoomPtr->mColumn = Column;
	NewRoomPtr->mPositionOffsetRate = FVector2D(mBuildStream.FRandRange(-1., 1.), mBuildStream.FRandRange(-1., 1.));
	NewRoomPtr->mStaticRoomSpawnDataId = URandomStreamFunctionLibrary::GetRandomItem(mBuildStream, RoomIdArray);

	return *NewRoomPtr;
}

TArray<FPrimaryAssetId> FStageBuilder::GetFilteredPrimaryAssets(const FPrimaryAssetType& AssetType, TFunctionRef<bool(const FAssetData&)> Filter) const
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	TArray<FPrimaryAssetId> ResultIds;

	TArray<FAssetData> AllMetaDatas;
	AssetManager->GetPrimaryAssetDataList(AssetType, AllMetaDatas);

	for (const FAssetData& MetaData : AllMetaDatas)
	{
		if (Filter(MetaData) == true)
		{
			ResultIds.Add(MetaData.GetPrimaryAssetId());
		}
	}

	return ResultIds;
}

TArray<FPrimaryAssetId> FStageBuilder::GetFilteredPrimaryAssets(const TArray<FAssetData>& AssetDataList, TFunctionRef<bool(const FAssetData&)> Filter) const
{
	TArray<FPrimaryAssetId> ResultIds;

	for (const FAssetData& MetaData : AssetDataList)
	{
		if (Filter(MetaData) == true)
		{
			ResultIds.Add(MetaData.GetPrimaryAssetId());
		}
	}

	return ResultIds;
}

TArray<FAssetData> FStageBuilder::GetFilteredPrimaryAssetDatas(const FPrimaryAssetType& AssetType, TFunctionRef<bool(const FAssetData&)> Filter) const
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));

	TArray<FAssetData> ResultDatas;

	TArray<FAssetData> AllMetaDatas;
	AssetManager->GetPrimaryAssetDataList(AssetType, AllMetaDatas);

	for (const FAssetData& MetaData : AllMetaDatas)
	{
		if (Filter(MetaData) == true)
		{
			ResultDatas.Add(MetaData);
		}
	}

	return ResultDatas;
}

uint8 FStageBuilder::GetRandomRarityIndex(const FRarityRate& RarityRate) const
{
	return StaticCast<uint8>(RarityRate.GetType(mBuildStream.FRand()));
}

ERarityType FStageBuilder::GetRandomRarity(const FRarityRate& RarityRate) const
{
	return RarityRate.GetType(mBuildStream.FRand());
}


