// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "SkillComponent/SkillCommitResultHolder.h"
#include "../FunctionLibrary/CombatCalculator/CombatCalculatorFunctionLibrary.h"
#include "../FunctionLibrary/CommandLog/CommandLogFunctionLibrary.h"
#include "SRPGFramework/TileActor.h"
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
	checkf(mSkillData.IsValidIndex(In_SkillIndex), TEXT("잘못된 배열 범위"))

	Out_SkillData = mSkillData[In_SkillIndex];

	return true;
}

bool USkillComponent::SetSkillData(int SkillIndex, TSoftObjectPtr<UStaticSkillData> SkillData)
{
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("잘못된 배열 범위"))

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

bool USkillComponent::ActivateSkill(const FCommandLog& SkillResult)
{
	checkf(GetOwner(), TEXT("주인 Actor가 없습니다."));

	for (const FTileLog& TileLog : SkillResult.mTileLog)
	{
		UE_LOG(LogTemp, Warning, TEXT("Timig : %d"), TileLog.mEventTimig);

		for (const TPair<int32, FEventLog>& Element : TileLog.mEventLog)
		{
			int32 TileMapIndex = Element.Key;
			const FEventLog&  EventLog = Element.Value;

			// 로직 처리
			UE_LOG(LogTemp, Warning, TEXT("TileMapIndex : %d"), TileMapIndex);

			if (EventLog.mUnitEventLog.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("Effect Tag : %s"), *EventLog.mUnitEventLog.mGameplayTag.ToString());
				UE_LOG(LogTemp, Warning, TEXT("Effect Value : %f"), EventLog.mUnitEventLog.mValue);
			}
		}
	}

	return true;
}

bool USkillComponent::CalculateSkillResult(int32 SkillIndex, FTileMapCloneData& CloneData, const TArray<FTileIndex>& TileIndex, FCommandLog& Out_Result)
{
	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("스킬 인덱스가 유효하지 않습니다."));

	const AUnit* Unit = Cast<AUnit>(GetOwner());
	TScriptInterface<const ITileActor> Caster = Unit;

	FCommandLogFunctionContext CLFContext;

	CLFContext.mRequestType = ECommandLogRequestType::Skill;
	CLFContext.mSkillData = mSkillData[SkillIndex].Get();
	CLFContext.mTargetTiles = TileIndex;
	CLFContext.mSourceActorID = Unit->GetUniqueID();

	return UCommandLogFunctionLibrary::CalculateSkillCommandLog(CloneData, CLFContext, Out_Result);
}

