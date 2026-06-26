// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SkillComponent/SkillComponentModel.h"

#include "Pawn/UnitModel.h"
#include "Pawn/Test/SkillTestUnit.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/TileMap/TileMapModel.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Pawn/Test/SkillTestUnitModel.h"

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

void USkillComponentModel::GetSkillData(int In_SkillIndex, OUT TSoftObjectPtr<UStaticSkillData>& Out_SkillData)
{
	checkf(mSkillData.IsValidIndex(In_SkillIndex), TEXT("잘못된 배열 범위"))

	Out_SkillData = mSkillData[In_SkillIndex];
}

void USkillComponentModel::SetSkillData(int SkillIndex, IN const TSoftObjectPtr<UStaticSkillData>& SkillData)
{
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 배열 범위"))

		mSkillData[SkillIndex] = SkillData;

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(SkillIndex, SkillData);
}


void USkillComponentModel::AddSkillData(IN const TSoftObjectPtr<UStaticSkillData>& SkillData)
{
	mSkillData.Add(SkillData);

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(mSkillData.Num() - 1, SkillData);

}

void USkillComponentModel::ExtractTarget(const TArray<FTileIndex>& TargetTile, ETileLayerFlag ActorFlag, ETargetFilter TargetFilter, OUT TArray<TWeakObjectPtr<UBoardActorModel>>& TargetActors)
{
	// @Note 서브시스템이 작동이 가능하다면 서브시스템에서 가져오자.

	// CombatModel을 가져옵니다.
	//USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	//checkf(IsValid(CombatModel), TEXT("전투 모델이 비어있습니다."));

	// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
	//TWeakObjectPtr<UTileMapModel> TMModel = CombatModel->GetTileMap();
	//checkf(TMModel.IsValid(), TEXT("타일맵 모델이 존재하지 않습니다."));
	// ===============================================================

	// @Note 유닛 모델 생성이 아닌 참조로 바꾸기
	for (int i = 0; i < TargetTile.Num(); ++i)
	{
		// 해당 타일에 유닛 모델을 가져온다.
		//TWeakObjectPtr<UBoardActorModel> TargetBoardModel = TMModel->GetActorOnTile<UBoardActorModel>(TargetTile[i], ActorFlag);
		TWeakObjectPtr<USkillTestUnitModel> TargetBoardModel = NewObject<USkillTestUnitModel>(this);// = NewObject<USkilTestUnit>();
		TargetBoardModel->Initialize();
		TargetBoardModel->BeginPlay();


		checkf(IsValid(TargetBoardModel->GetAttributeComponentModel()), TEXT("컴포넌트 없음"));
		TargetBoardModel->GetAttributeComponentModel()->SetAttributeBaseValue(UUnitAttributeSet::GetMaxHPAttribute(), 100);
		TargetBoardModel->GetAttributeComponentModel()->SetAttributeBaseValue(UUnitAttributeSet::GetHPAttribute(), 100);
		TargetBoardModel->GetAttributeComponentModel()->SetAttributeBaseValue(UUnitAttributeSet::GetDamagePointAttribute(), 10);
		TargetBoardModel->GetAttributeComponentModel()->SetAttributeBaseValue(UUnitAttributeSet::GetDefensePointAttribute(), 5);

		TargetActors.Add(TargetBoardModel);
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
		FBoardCombatTargetSnapshotData* OwnerSnapShot = OwnerUnit->MakeSnapshotData();

		// 효과를 적용한다.
		for (int32 j = 0; j < TargetActors.Num(); ++j)
		{
			// 피격자의 스냅샷을 따옵니다.
			TWeakObjectPtr<UUnitModel> TargetUnit = Cast<UUnitModel>(TargetActors[j]);
			checkf(TargetUnit.IsValid(), TEXT("타겟 유닛 유효하지 않음"));
			FBoardCombatTargetSnapshotData* TargetSnapShot = TargetUnit->MakeSnapshotData();


			SkillData.Get()->mSkillMotionLayers[i].mStaticSkillEffectLayers->ApplySkillEffect(DicePoint, OwnerActor, TargetActors[j], OwnerSnapShot, TargetSnapShot);
		}
	}

	return true;
}

void USkillComponentModel::HandelMovePoint(float MovePoint)
{
}