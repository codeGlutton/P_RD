// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "SkillComponent/SkillCommitResultHolder.h"
#include "../FunctionLibrary/CombatCalculator/CombatCalculatorFunctionLibrary.h"
#include "Pawn/Unit.h"

// Sets default values for this component's properties
USkillComponent::USkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool USkillComponent::GetSkillData(int In_SkillIndex, TSoftObjectPtr<UStaticSkillData>& Out_SkillData)
{
	if (!mSkillData.IsValidIndex(In_SkillIndex))
		return false;

	Out_SkillData = mSkillData[In_SkillIndex];

	return true;
}

bool USkillComponent::SetSkillData(int SkillIndex, TSoftObjectPtr<UStaticSkillData> SkillData)
{
	if (!mSkillData.IsValidIndex(SkillIndex))
		return false;

	mSkillData[SkillIndex] = SkillData;

	if(OnSkillChange.IsBound())
		OnSkillChange.Broadcast(SkillIndex, SkillData);

	return true;
}


bool USkillComponent::AddSkillData(TSoftObjectPtr<UStaticSkillData> SkillData)
{
	mSkillData.Add(SkillData);

	if (OnSkillChange.IsBound())
		OnSkillChange.Broadcast(mSkillData.Num() - 1, SkillData);

	return true;
}


bool USkillComponent::GetPreviewEffectIconData(const FPreviewEffectIconData& Out_Data)
{

	return false;
}

bool USkillComponent::ActivateSkill()
{
	if (!GetOwner())
		return false;

	// 어빌리티를 발동시킨다.
	FGameplayEventData EventData;

	EventData.Instigator = GetOwner();	// 사용자

	EventData.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Test.GameplayAbility.Skill"));

	// 1. 구조체를 담을 UObject 홀더 생성
	USkillCommitResultHolder* ResultHolder = NewObject<USkillCommitResultHolder>();

	// 2. 결과 데이터 복사
	ResultHolder->CommitResult = mSkillCommitResult;

	// 3. 이벤트 데이터의 OptionalObject에 래퍼 오브젝트 바인딩
	EventData.OptionalObject = ResultHolder;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), EventData.EventTag, EventData);

	return true;
}

bool USkillComponent::CalculateSkillResult(int32 SkillIndex, const TArray<FTileIndex>& Tiles)
{
	return UCombatCalculatorFunctionLibrary::CalculateSkillResult(mSkillData[SkillIndex].Get(), Tiles, mSkillCommitResult);
}

