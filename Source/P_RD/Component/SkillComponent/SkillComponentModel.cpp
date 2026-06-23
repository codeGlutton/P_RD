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


bool USkillComponentModel::GetSkillData(int In_SkillIndex, TSoftObjectPtr<UStaticSkillData>& Out_SkillData)
{
	checkf(mSkillData.IsValidIndex(In_SkillIndex), TEXT("잘못된 배열 범위"))

		Out_SkillData = mSkillData[In_SkillIndex];

	return true;
}

bool USkillComponentModel::SetSkillData(int SkillIndex, TSoftObjectPtr<UStaticSkillData> SkillData)
{
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 배열 범위"))

		mSkillData[SkillIndex] = SkillData;

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(SkillIndex, SkillData);

	return true;
}


bool USkillComponentModel::AddSkillData(TSoftObjectPtr<UStaticSkillData> SkillData)
{
	mSkillData.Add(SkillData);

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(mSkillData.Num() - 1, SkillData);

	return true;
}

bool USkillComponentModel::ActivateSkill(int32 SkillIndex, const TArray<FTileIndex> TargetTiles)
{
	// checkf(GetOwner(), TEXT("주인 Actor가 없습니다."));
	checkf(IsValid(mAbility), TEXT("Ability 없음"));
	checkf(IsValid(GetOwnerModel()), TEXT("OwnerModel 없음"));

	// 주인 액터를 찾아서 BoardActor로 캐스팅한다.
	TWeakObjectPtr<UBoardActorModel> BoardActor = Cast<UBoardActorModel>(GetOwnerModel());

	checkf(IsValid(BoardActor.Get()), TEXT("BoardActor 없음"));

	// Context를 만들어서 스킬을 준비한다.
	FTacticalAbilityContext Context;
	Context.mCasterActor = BoardActor;
	Context.mTargetTile = TargetTiles;

	// 스킬 정보를 담는다.
	// Playload 오브젝트를 생성합니다.
	TObjectPtr<UTacticalEffectPayload_Skill> Payload = NewObject<UTacticalEffectPayload_Skill>();
	Payload->mSkillData = mSkillData[SkillIndex];
	Payload->mTacticalEffectPayloadType = ETacticalEffectPayloadType::Skill;

	Context.mInstigatorData = Payload;

	//mAbility->ActivateAbility(Context);

	return true;
}

void USkillComponentModel::HandelMovePoint(float MovePoint)
{
}
