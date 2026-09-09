#pragma once

#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "UObject/UnrealType.h"
#include "RunCheckpointTestsHelper.generated.h"

UCLASS()
class URunCheckpointTestData : public URunPersistData
{
	GENERATED_BODY()
public:
	void Seed()
	{
		mStage.InitializeAs<FStage>();
		FStage& Stage = mStage.GetMutable();
		Stage.mStageLevel = EStageLevelType::Stage1;
		Stage.mCurRow = Stage.mCurColumn = 0;
		Stage.mRoomRows.SetNum(1);
		Stage.mRoomRows[0].mRooms.SetNum(2);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			Stage.mRoomRows[0].mRooms[Index].InitializeAs<FRoom>();
			FRoom& Room = Stage.mRoomRows[0].mRooms[Index].GetMutable();
			Room.mType = ERoomType::Monster;
			Room.mColumn = Index;
			Room.mWasSelected = Index == 0;
		}
		mMoney = 100.f;
		mDifficulty = 2;
		mArtifactIds = { FPrimaryAssetId(TEXT("Artifact"), TEXT("Entry")) };
		mRewardSkillIds = { FPrimaryAssetId(TEXT("Skill"), TEXT("Entry")) };
		mRewardEquipmentIds = { FPrimaryAssetId(TEXT("Equipment"), TEXT("Entry")) };
		mRunLog.mKilledEnemyUnits.Add(FPrimaryAssetId(TEXT("Enemy"), TEXT("Entry")), 3);
		mStageBuildStream.Initialize(177);
		mEventStream.Initialize(288);
		mStageBuildStream.FRand();
		mEventStream.FRand();
		for (int32 Index = 0; Index < mPartyPlayers.Num(); ++Index)
		{
			SetHP(Index, 80.f + Index);
			*FindFProperty<FStructProperty>(UPlayerUnitPersistData::StaticClass(), TEXT("mPlayerUnitId"))
				->ContainerPtrToValuePtr<FPrimaryAssetId>(mPartyPlayers[Index]) =
				FPrimaryAssetId(TEXT("Player"), FName(*FString::Printf(TEXT("Party%d"), Index)));
		}
	}
	void MutateDuringCombat()
	{
		mMoney = 27.f;
		mArtifactIds.Reset();
		mRewardSkillIds.Reset();
		mRewardEquipmentIds.Reset();
		mRunLog.mKilledEnemyUnits.Reset();
		mEventStream.FRand();
		mStageBuildStream.FRand();
		for (int32 Index = 0; Index < mPartyPlayers.Num(); ++Index) SetHP(Index, 1.f);
	}
	void GrantCompletedRoomTransaction()
	{
		mStage.GetMutable().mClearData.mIsCleared = true;
		mMoney += 50.f;
		mRoomTransactions.GoldClaimed = mRoomTransactions.ExpClaimed = true;
		mRoomTransactions.TreasureOpened = mRoomTransactions.RestUsed = true;
		mRoomTransactions.SelectedArtifact = FPrimaryAssetId(TEXT("Artifact"), TEXT("Reward"));
		mArtifactIds.Add(mRoomTransactions.SelectedArtifact);
		mRoomTransactions.ClaimedChoices.Add(2);
		mRoomTransactions.SoldShopSlots.Add(4);
	}
	float Money() const { return mMoney; }
	float HP(int32 Index) const
	{
		return FindFProperty<FFloatProperty>(UPlayerUnitPersistData::StaticClass(), TEXT("mHP"))
			->GetPropertyValue_InContainer(mPartyPlayers[Index]);
	}
	void SetHP(int32 Index, float Value)
	{
		FindFProperty<FFloatProperty>(UPlayerUnitPersistData::StaticClass(), TEXT("mHP"))
			->SetPropertyValue_InContainer(mPartyPlayers[Index], Value);
	}
};

UCLASS()
class UCheckpointPlayerPersistTestData : public UPlayerUnitPersistData
{
	GENERATED_BODY()
public:
	int32 StackCount(FGameplayTag Tag) const { return mTagCountMap.FindRef(Tag); }
};

UCLASS()
class UCheckpointCombatTestModel : public USRPGCombatModel
{
	GENERATED_BODY()
public:
	bool EvaluateEmptyRound() { return EvaluateRound(); }
};
