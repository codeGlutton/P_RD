// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SkillComponent/SkillComponentModel.h"

#include "DataAsset/SkillData/StaticSkillData.h"

#include "Pawn/UnitModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "TAS/Effect/TacticalEffectContext.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

// Sets default values for this component's properties
USkillComponentModel::USkillComponentModel()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	

	// ...
}


void USkillComponentModel::Initialize()
{
	Super::Initialize();
}

// Called when the game starts
void USkillComponentModel::BeginPlay()
{
	Super::BeginPlay();

	// ...

}

bool USkillComponentModel::TryGetSkillData(int32 SkillIndex, OUT TSoftObjectPtr<UStaticSkillData>& Out_SkillData) const
{
	if (mSkillData.IsValidIndex(SkillIndex) == false)
	{
		Out_SkillData = nullptr;
		return false;
	}

	Out_SkillData = mSkillData[SkillIndex];
	return Out_SkillData.IsNull() == false;
}

void USkillComponentModel::GetSkillData(int32 SkillIndex, OUT TSoftObjectPtr<UStaticSkillData>& Out_SkillData) const
{
	if (TryGetSkillData(SkillIndex, OUT Out_SkillData) == false)
	{
		ensureMsgf(false, TEXT("잘못된 스킬 인덱스 범위: %d"), SkillIndex);
	}
}

int32 USkillComponentModel::GetSkillDataNum() const
{
	return mSkillData.Num();
}

void USkillComponentModel::SetSkillData(int32 SkillIndex, IN const TSoftObjectPtr<UStaticSkillData>& SkillData)
{
	checkf(mSkillData.IsValidIndex(SkillIndex) == true, TEXT("잘못된 스킬 인덱스 범위"));

	UStaticSkillData* PreSkillData = mSkillData[SkillIndex].Get();
	mSkillData[SkillIndex] = SkillData;

	if (OnChangeSkill.IsBound())
		OnChangeSkill.Broadcast(SkillIndex, SkillData.Get(), PreSkillData);
}


void USkillComponentModel::AddSkillData(IN const TSoftObjectPtr<UStaticSkillData>& SkillData)
{
	const int32 SkillIndex = mSkillData.Add(SkillData);

	if (OnChangeSkill.IsBound())
	{
		OnChangeSkill.Broadcast(SkillIndex, nullptr, SkillData.Get());
	}
}

void USkillComponentModel::ExtractTarget(const TArray<FTileIndex>& TargetTile, ETileLayerFlag ActorFlag, ETargetFilter TargetFilter, OUT TArray<TWeakObjectPtr<UBoardActorModel>>& TargetActors)
{
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(IsValid(CombatModel), TEXT("전투 모델이 비어있습니다."));

	UTileMapModel* TileMapModel = CombatModel->GetTileMap();
	checkf(IsValid(TileMapModel), TEXT("타일맵 모델이 존재하지 않습니다."));

	UUnitModel* OwnerUnit = Cast<UUnitModel>(GetOwnerModel());
	checkf(IsValid(OwnerUnit), TEXT("스킬 소유 유닛이 존재하지 않습니다."));

	for (const FTileIndex& TileIndex : TargetTile)
	{
		for (UBoardActorModel* TargetActor : TileMapModel->GetActorsOnTile(TileIndex, ActorFlag))
		{
			if (TargetActor == nullptr)
			{
				continue;
			}

			UUnitModel* TargetUnit = Cast<UUnitModel>(TargetActor);
			bool bIncludeTarget = TargetFilter != ETargetFilter::None;
			if (TargetUnit != nullptr)
			{
				if (TargetUnit == OwnerUnit)
				{
					bIncludeTarget = EnumHasAnyFlags(TargetFilter, ETargetFilter::IncludeSelf);
				}
				else if (TargetUnit->GetGenericTeamId() == OwnerUnit->GetGenericTeamId())
				{
					bIncludeTarget = EnumHasAnyFlags(TargetFilter, ETargetFilter::IncludeAlly);
				}
				else
				{
					bIncludeTarget = EnumHasAnyFlags(TargetFilter, ETargetFilter::IncludeEnemy);
				}
			}

			if (bIncludeTarget)
			{
				TargetActors.AddUnique(TWeakObjectPtr<UBoardActorModel>(TargetActor));
			}
		}
	}
}

bool USkillComponentModel::ActivateSkill(int32 SkillIndex, const TArray<FTileIndex>& TargetTiles, float DicePoint)
{
	checkf(IsValid(GetOwnerModel()), TEXT("OwnerModel 없음"));
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 스킬 인덱스"));

	TSoftObjectPtr<UStaticSkillData> SkillData = mSkillData[SkillIndex];
	checkf(SkillData.IsValid(), TEXT("잘못된 스킬"));
	checkf(!SkillData.IsPending(), TEXT("로드되어있지 않는 스킬"));

	TWeakObjectPtr<UBoardActorModel> OwnerActor = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(OwnerActor.IsValid(), TEXT("보드 액터"));

	for (int32 i = 0; i < SkillData.Get()->mSkillMotionLayers.Num(); ++i)
	{
		const FSkillMotionLayer& SkillMotionLayer = SkillData.Get()->mSkillMotionLayers[i];
		
		// 스킬 포인트를 사용하여 특정 포인트로 전환합니다.


		// 타겟 액터를 가져옵니다.
		TArray<TWeakObjectPtr<UBoardActorModel>> TargetActors;
		ExtractTarget(TargetTiles,
			(ETileLayerFlag)SkillData.Get()->mSkillMotionLayers[i].mStaticSkillEffectLayers->mTileLayer,
			(ETargetFilter)SkillData.Get()->mSkillMotionLayers[i].mStaticSkillEffectLayers->mTargetFilter, TargetActors);

		// 시전자의 스냅샵을 따옵니다.
		TWeakObjectPtr<UUnitModel> OwnerUnit = Cast<UUnitModel>(OwnerActor.Get());
		checkf(OwnerUnit.IsValid(), TEXT("주인 유닛 유효하지 않음"));
		FBoardCombatTargetSnapshotData OwnerSnapShot = OwnerUnit->MakeSnapshotData();

		// 효과를 적용한다.
		for (int32 j = 0; j < TargetActors.Num(); ++j)
		{
			// 피격자의 스냅샷을 따옵니다.
			TWeakObjectPtr<UUnitModel> TargetUnit = Cast<UUnitModel>(TargetActors[j]);
			if (!TargetUnit.IsValid())
				continue;
			FBoardCombatTargetSnapshotData TargetSnapShot = TargetUnit->MakeSnapshotData();

			SkillData.Get()->mSkillMotionLayers[i].mStaticSkillEffectLayers->ApplySkillEffect(DicePoint, OwnerActor, TargetActors[j], &OwnerSnapShot, &TargetSnapShot);
		}
	}

	return true;
}

void USkillComponentModel::HandelMovePoint(float MovePoint)
{
}
