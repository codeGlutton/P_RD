// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SkillComponent/SkillComponentModel.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"


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

bool USkillComponentModel::ActivateSkill(int32 SkillIndex, const TArray<FTileIndex> TargetTiles)
{
	// checkf(GetOwner(), TEXT("주인 Actor가 없습니다."));
	// checkf(IsValid(mAbility), TEXT("Ability 없음"));
	checkf(IsValid(GetOwnerModel()), TEXT("OwnerModel 없음"));
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 스킬 인덱스"));

	// 스킬을 기반으로 효과를 계산한다.
	TSoftObjectPtr<UStaticSkillData> SkillData = mSkillData[SkillIndex];
	checkf(SkillData.IsValid(), TEXT("잘못된 스킬"));

	TWeakObjectPtr<UBoardActorModel> BoardActor = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(BoardActor.IsValid(), TEXT("보드 액터"));

	for (int32 i = 0; i < SkillData.Get()->mSkillMotionLayers.Num(); ++i)
	{
		const FSkillMotionLayer& SkillMotionLayer = SkillData.Get()->mSkillMotionLayers[i];

		// Context 오브젝트를 생성합니다.
		FTacticalEffectRequestContainer EffectContainer;

		// 효과의 정보를 가져온다.
		FBoardCombatTargetSnapshotData SnapShotData;
		SnapShotData.mAttributes.Add(UUnitAttributeSet::GetHPAttribute(), 10);		// 임시 데이터

		// 타겟을 우선 가져옵니다.
		TArray<TWeakObjectPtr<UBoardActorModel>> TargetActors;
		ExtractTarget(TargetTiles, ETileLayerFlag::Unit, ETargetFilter::All, TargetActors);

		for (int32 j = 0; j < TargetActors.Num(); ++j)
		{
			// 기본 효과값을 넣는다.
			EffectContainer.mTargetRequests.Add(TargetActors[j].Get(), SnapShotData);
		}

		//	모션 전 패시브
		//	for(Passive : Passsives)
		//	{
		//		Context
		//		{
		//			발동시킨 대상 = 없음,
		//			발동시킨 대상의 스냅샷 = 없음,
		//			소유자,
		//			소유자의 스냅샷
		//		}
		//
		//		패시브
		//		Passive->ActivateAbility(Context, EffectContexts, PassiveStackContexts);
		//		Passive->UpdatePassive(PassiveStackContexts);
		//	}

		//	for(TargetActor : TargetActors)
		//	{
		//		EffectContexts 복제
		//		피격 전 패시브
		//		for(TargetActorPassive : TargetActorPassives)
		//		{
		//			Context
		//			{
		//				발동시킨 대상,
		//				발동시킨 대상의 스냅샷,
		//				소유자,
		//				소유자의 스냅샷
		//			}
		//				
		//		패시브
		//		Passive->ActivateAbility(Context, EffectContexts, PassiveStackContexts);
		//		Passive->UpdatePassive(PassiveStackContexts);
		//		}
		//			
		//		타격 전 패시브
		//		for(Passive : Passives)
		//		{
		//			Context
		//			{
		//				발동시킨 대상,
		//				발동시킨 대상의 스냅샷,
		//				소유자,
		//				소유자의 스냅샷
		//			}	
		// 			패시브
		//			Passive->ActivateAbility(Context, EffectContexts, PassiveStackContexts);
		//			Passive->UpdatePassive(PassiveStackContexts);
		//		}
		//	}

		// 효과를 적용한다.
		ApplyEffect(EffectContainer);

		//	for(TargetActor : TargetActors)
		//	{
		//		EffectContexts 복제
		//		피격 후 패시브
		//		for(TargetActorPassive : TargetActorPassives)
		//		{
		//			Context
		//			{
		//				발동시킨 대상,
		//				발동시킨 대상의 스냅샷,
		//				소유자,
		//				소유자의 스냅샷
		//			}
		//				
		//		패시브
		//		Passive->ActivateAbility(Context, EffectContexts, PassiveStackContexts);
		//		Passive->UpdatePassive(PassiveStackContexts);
		//		}
		//		ApplyEffect(TargetActors, EffectContexts);
		//			
		//		타격 후 패시브
		//		for(Passive : Passives)
		//		{
		//			Context
		//			{
		//				발동시킨 대상,
		//				발동시킨 대상의 스냅샷,
		//				소유자,
		//				소유자의 스냅샷
		//			}	
		// 			패시브
		//			Passive->ActivateAbility(Context, EffectContexts, PassiveStackContexts);
		//			Passive->UpdatePassive(PassiveStackContexts);
		//		}
		//		ApplyEffect(TargetActors, EffectContexts);
		//	}

		//	모션 후 패시브
		//	for(Passive : Passsives)
		//	{
		//		Context = 발동시킨 대상, 발동시킨 대상의 스냅샷, 소유자, 소유자의 스냅샷
		//	
		//		패시브
		//		Passive->ActivateAbility(Context, EffectContexts, PassiveStackContexts);
		//		Passive->UpdatePassive(PassiveStackContexts);
		//	}
		//	ApplyEffect(TargetActors, EffectContexts);
	}

	return true;
}

void USkillComponentModel::HandelMovePoint(float MovePoint)
{
}

void USkillComponentModel::ApplyEffect(FTacticalEffectRequestContainer& TacticalEffectRequestContainer)
{
	TWeakObjectPtr<UBoardActorModel> BoardActor = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(BoardActor.IsValid(), TEXT("보드 액터"));

	for (TMap<TObjectPtr<UBoardActorModel>, FBoardCombatTargetSnapshotData>::TIterator It = TacticalEffectRequestContainer.mTargetRequests.CreateIterator(); It; ++It)
	{
		// It.Key()와 It.Value()로 접근
		TObjectPtr<UBoardActorModel> Actor = It.Key();
		FBoardCombatTargetSnapshotData& Data = It.Value();

		// 적용
		// 각각에게 적용합니다.

		UE_LOG(LogTemp, Warning, TEXT("TestActor : %d"), Actor->GetUniqueID());


		for (int i = 0; i < Data.mAttributes.Num(); ++i)
		{
			for (TMap<FGameplayAttribute, float>::TIterator iter = Data.mAttributes.CreateIterator(); iter; ++iter)
			{
				float BaseValue = Actor->FindComponentModelByClass<UAttributeSetComponentModel>()->GetAttributeCurrentValue(iter.Key());
				//UE_LOG(LogTemp, Warning, TEXT("BaseValue : %d"), BaseValue);
				float CurrentValue = BaseValue - iter.Value();
				//UE_LOG(LogTemp, Warning, TEXT("CurrentValue : %d"), CurrentValue);
				FString Name = iter.Key().AttributeName;
				//UE_LOG(LogTemp, Warning, TEXT("AttributeName : %s"), *Name);
			}
		}
	}
}

void USkillComponentModel::ExtractTarget(const TArray<FTileIndex>& TargetTile, ETileLayerFlag ActorFlag, ETargetFilter TargetFilter, OUT TArray<TWeakObjectPtr<UBoardActorModel>>& TargetActors)
{
	// CombatModel을 가져옵니다.
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(IsValid(CombatModel), TEXT("전투 모델이 비어있습니다."));

	// 서브 시스템에 접근하여 타일맵 모델을 가져온다.
	TWeakObjectPtr<UTileMapModel> TMModel = CombatModel->GetTileMap();
	checkf(TMModel.IsValid(), TEXT("타일맵 모델이 존재하지 않습니다."));
	// ===============================================================

	for (int i = 0; i < TargetTile.Num(); ++i)
	{
		// 해당 타일에 유닛 모델을 가져온다.
		TWeakObjectPtr<UBoardActorModel> TargetBoardModel = TMModel->GetActorOnTile<UBoardActorModel>(TargetTile[i], ActorFlag);

		// 유닛이 없다면 반환한다.
		if (!TargetBoardModel.IsValid())
			continue;

		TargetActors.Add(TargetBoardModel);
	}
}
