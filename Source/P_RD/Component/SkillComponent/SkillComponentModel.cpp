#include "Component/SkillComponent/SkillComponentModel.h"

#include "Engine/AssetManager.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "Actor/TileMap/TileMapModel.h"

namespace
{
	UStaticSkillData* LoadStaticSkillData(const FPrimaryAssetId& SkillId)
	{
		if (SkillId.IsValid() == false)
		{
			return nullptr;
		}

		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr)
		{
			return nullptr;
		}

		if (UStaticSkillData* Loaded = AssetManager->GetPrimaryAssetObject<UStaticSkillData>(SkillId))
		{
			return Loaded;
		}

		const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(SkillId);
		return Cast<UStaticSkillData>(AssetPath.TryLoad());
	}
}

FSkillEntry::FSkillEntry(UStaticSkillData* Data) : mData(Data)
{
}

bool FSkillEntry::IsValid() const
{
	return mData != nullptr;
}

USkillComponentModel::USkillComponentModel()
{
	mSkillEntries.Init(FSkillEntry(), 4 /*추가 스킬*/ + 2 /*기본 스킬*/);
}

void USkillComponentModel::SetSkillFrom(const TArray<TSoftObjectPtr<UStaticSkillData>>& SkillList)
{
	// 초기화 로직 (몬스터는 6개 이상의 스킬도 소유할 수 있음)

	int32 NextSkillIndex = 0;
	for (const TSoftObjectPtr<UStaticSkillData>& Skill : SkillList)
	{
		SetSkill(NextSkillIndex++, Skill.Get());
	}
}

void USkillComponentModel::SetSkillFrom(const TArray<FPrimaryAssetId>& SkillList)
{
	// 초기화 로직 (몬스터는 6개 이상의 스킬도 소유할 수 있음)

	int32 NextSkillIndex = 0;
	for (const FPrimaryAssetId& AssetId : SkillList)
	{
		UStaticSkillData* StaticSkillData = LoadStaticSkillData(AssetId);
		if (StaticSkillData != nullptr)
		{
			SetSkill(NextSkillIndex++, StaticSkillData);
		}
	}
}

const FSkillEntry* USkillComponentModel::GetSkill(int32 SkillIndex) const
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	return &mSkillEntries[SkillIndex];
}

void USkillComponentModel::SetSkill(int32 SkillIndex, UStaticSkillData* SkillData)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));

	const UStaticSkillData* PreSkillData = mSkillEntries[SkillIndex].mData;
	mSkillEntries[SkillIndex] = FSkillEntry(SkillData);

	OnChangeSkillUI.Broadcast(SkillIndex, SkillData, PreSkillData);
}

bool USkillComponentModel::ActivateSkill(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, int32 DiceSum)
{
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 사용 스킬 인덱스"));

	FSkillEntry& SkillEntry = mSkillEntries[SkillIndex];
	checkf(SkillEntry.IsValid() == true, TEXT("빈 스킬 시전 오류"));

	IBoardCombatTarget* OwnerCombatTarget = Cast<IBoardCombatTarget>(GetOwnerModel());
	checkf(OwnerCombatTarget != nullptr, TEXT("스킬을 시전할 Owner가 유효하지 않음"));

	// TODO

	return true;
}

TArray<FTileIndex> USkillComponentModel::GetAimableTiles(UTileMapModel* MapModel, int32 SkillIndex, int32 DiceSum) const
{
	TArray<FTileIndex> AimableTiles;
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const float AimRange = StaticSkillData->mAimRangeDefaultValue + DiceSum * StaticSkillData->mAimRangeRatio;
	const EAimPattern Pattern = StaticSkillData->mAimPattern;
	const bool CanAimObstacle = StaticSkillData->mCanAimBoardActor;
	const bool IsIndirect = StaticSkillData->mIsIndirect;

	return MapModel->GetAimableTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, AimRange, Pattern, CanAimObstacle, IsIndirect);
}

TArray<FTileIndex> USkillComponentModel::GetEffectTiles(UTileMapModel* MapModel, int32 SkillIndex, const FTileIndex& TargetIndex, int32 DiceSum) const
{
	TArray<FTileIndex> AimableTiles;
	checkf(mSkillEntries.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));
	UStaticSkillData* StaticSkillData = mSkillEntries[SkillIndex].mData;
	checkf(StaticSkillData != nullptr, TEXT("잘못된 스킬 데이터"));

	const EEffectPattern Pattern = StaticSkillData->mEffectPattern;
	const int32 EffectRange = StaticSkillData->mEffectAreaDefaultValue + DiceSum * StaticSkillData->mEffectAreaRatio;
	const bool IsPenetration = StaticSkillData->mIsPenetration;

	return MapModel->GetEffectTiles(GetOwnerModel<UBoardActorModel>()->GetTileTransform().mIndex, TargetIndex, Pattern, EffectRange, IsPenetration);
}