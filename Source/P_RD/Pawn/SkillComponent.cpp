// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/SkillComponent.h"

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

