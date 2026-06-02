// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "Unit.h"

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
	return false;
}

bool USkillComponent::SetSkillData(int SkillIndex, TSoftObjectPtr<UStaticSkillData> SkillData)
{
	return false;
}

bool USkillComponent::CalculatePredictedSales(int32 In_SkillIndex, TArray<TPair<FString, float>>& Out_TagValue)
{
	return false;
}

bool USkillComponent::ActivateSkill(int32 SkillIndex, TArray<TPair<int32, int32>> Tiles)
{
	return false;
}

bool USkillComponent::TestActivateSkill(int32 SkillIndex, TArray<AUnit*> UnitArray)
{
	if (!GetOwner())
		return false;

	// 어빌리티를 발동시킨다.
	FGameplayEventData EventData;

	EventData.Instigator = GetOwner();	// 사용자

	EventData.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Test.GameplayAbility.Skill"));

	// 2. 빈 타겟 데이터 구조체를 동적 할당합니다.
	FGameplayAbilityTargetData_ActorArray* ArrayTargetData = new FGameplayAbilityTargetData_ActorArray();


	// 3. 소프트 포인터 배열을 순회하며 실제 액터 포인터를 추출해 채워 넣습니다.
	for (const TSoftObjectPtr<AUnit>& UnitSoftPtr : UnitArray)
	{
		// 소프트 포인터가 가리키는 대상이 현재 메모리에 로드되어 있고 유효한지 확인
		if (UnitSoftPtr.IsValid())
		{
			// TargetActorArray는 TWeakObjectPtr<AActor> 배열이지만, 
			// 일반 포인터(AActor*)를 넣으면 자동으로 변환되어 들어갑니다.
			ArrayTargetData->TargetActorArray.Add(UnitSoftPtr.Get());
		}
	}

	// 4. 포장된 데이터를 EventData에 추가
	EventData.TargetData.Add(ArrayTargetData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), EventData.EventTag, EventData);

	return true;
}

