// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SkillComponent/SkillComponentModel.h"


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
	checkf(SkillData.IsPending(), TEXT("로드되어있지 않는 스킬"));

	TWeakObjectPtr<UBoardActorModel> BoardActor = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(BoardActor.IsValid(), TEXT("보드 액터"));

	for (int32 i = 0; i < SkillData.Get()->mSkillMotionLayers.Num(); ++i)
	{
		TArray<UTacticalEffectContext*> EffectContexts;
		const FSkillMotionLayer& SkillMotionLayer = SkillData.Get()->mSkillMotionLayers[i];

		// Context 오브젝트를 생성합니다.
		// 추후 팩토리 구성으로 Context 생성하도록 희망
		UTacticalEffectContext* EffectContext = SkillMotionLayer.mStaticSkillEffectLayers->CreateContext(BoardActor);
		EffectContexts.Add(EffectContext);

		// 우선 패시브 없이

		// 효과를 적용한다.
		for (int32 j = 0; j < TargetTiles.Num(); ++j)
		{
			ApplyEffect(TargetTiles[j], EffectContexts);
		}

		// 우선 패시브 없이
	}

	return true;
}

void USkillComponentModel::HandelMovePoint(float MovePoint)
{
}

void USkillComponentModel::ApplyEffect(FTileIndex TargetTile, TArray<UTacticalEffectContext*>& EffectContexts)
{
	TWeakObjectPtr<UBoardActorModel> BoardActor = Cast<UBoardActorModel>(GetOwnerModel());
	checkf(BoardActor.IsValid(), TEXT("보드 액터"));

	// 각각의 타일에게 효과를 적용한다.
	for (int32 j = 0; j < EffectContexts.Num(); ++j)
	{
		UTacticalEffect* EffectCDO = EffectContexts[j]->mTacticalEffect->GetDefaultObject<UTacticalEffect>();
		EffectCDO->ActivateEffect(*BoardActor.Get(), TargetTile, EffectContexts[j]);
	}
}
