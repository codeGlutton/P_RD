// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillComponent.h"
#include "SkillComponent/SkillCommitResultHolder.h"
#include "../FunctionLibrary/CombatCalculator/CombatCalculatorFunctionLibrary.h"
//#include "../FunctionLibrary/CommandLog/CommandLogFunctionLibrary.h"
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

	//if(OnSkillChange.IsBound())
	//	OnSkillChange.Broadcast(SkillIndex, SkillData);

	return true;
}


bool USkillComponent::AddSkillData(TSoftObjectPtr<UStaticSkillData> SkillData)
{
	mSkillData.Add(SkillData);

	//if (OnSkillChange.IsBound())
	//	OnSkillChange.Broadcast(mSkillData.Num() - 1, SkillData);

	return true;
}

//bool USkillComponent::ActivateSkill(const FCommandLog& SkillResult)
//{
//	checkf(GetOwner(), TEXT("주인 Actor가 없습니다."));
//
//	FString Log;
//
//	Log += "\nCommandLog\n{\n";
//	for (const FTileLog& TileLog : SkillResult.mTileLog)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Timig : %s"), *StaticEnum<EEffectEventTiming>()->GetNameStringByValue((int32)TileLog.mEventTimig));
//		Log += FString::Printf(TEXT("\tTimig : %s\n"), *StaticEnum<EEffectEventTiming>()->GetNameStringByValue((int32)TileLog.mEventTimig));
//		Log += "\t{\n";
//		for (const TPair<int32, FEventLog>& Element : TileLog.mEventLog)
//		{
//			int32 TileMapIndex = Element.Key;
//			const FEventLog&  EventLog = Element.Value;
//
//			// 로직 처리
//			UE_LOG(LogTemp, Warning, TEXT("TileMapIndex : %d"), TileMapIndex);
//			Log += FString::Printf(TEXT("\t\tTileMapIndex : %d\n"), TileMapIndex);
//
//			Log += FString::Printf(TEXT("\t\tUnitEvent\n\t\t{\n"));
//			if (EventLog.mUnitEventLog.IsValid())
//			{
//				UE_LOG(LogTemp, Warning, TEXT("UnitID : %d"), EventLog.mUnitEventLog.mTargetUnitID);
//				Log += FString::Printf(TEXT("\t\t\tUnitID : %d\n"), EventLog.mUnitEventLog.mTargetUnitID);
//
//				UE_LOG(LogTemp, Warning, TEXT("Effect Tag : %s"), *EventLog.mUnitEventLog.mGameplayTag.ToString());
//				Log += FString::Printf(TEXT("\t\t\tEffect Tag : %s\n"), *EventLog.mUnitEventLog.mGameplayTag.ToString());
//
//				UE_LOG(LogTemp, Warning, TEXT("Effect Value : %f"), EventLog.mUnitEventLog.mValue);
//				Log += FString::Printf(TEXT("\t\t\tEffect Value : %f\n"), EventLog.mUnitEventLog.mValue);
//
//			}
//			Log += "\t\t}\n";
//		}
//		Log += "\t}\n";
//	}
//	Log += "}";
//
//	UE_LOG(LogTemp, Warning, TEXT("%s"), *Log);
//
//
//
//	return true;
//}
//
//bool USkillComponent::CalculateSkillResult(int32 SkillIndex, TArray<FTileIndex>	mTargetTiles, const FTileMapCloneData& In_CloneData,  FCommandLog& Out_Result)
//{
//	checkf(mSkillData.IsValidIndex(SkillIndex), TEXT("스킬 인덱스가 유효하지 않습니다."));
//
//	FCommandLogFunctionContext CLFContext;
//	CLFContext.mRequestType = ECommandLogRequestType::Skill;
//	CLFContext.mSourceActorID = GetOwner()->GetUniqueID(); // 추후 액터 아이디로 변경
//	CLFContext.mTargetTiles = mTargetTiles;
//	CLFContext.mSkillData = mSkillData[SkillIndex].Get();
//
//	return UCommandLogFunctionLibrary::CalculateSkillCommandLog(In_CloneData, CLFContext, Out_Result);
//}

