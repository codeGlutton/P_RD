// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/TimeScaleComponent/TimeScaleComponent.h"

// Sets default values for this component's properties
UTimeScaleComponent::UTimeScaleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTimeScaleComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTimeScaleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTimeScale(DeltaTime);				// 시간 배율 조정
	UpdateActivateRequests(DeltaTime);		// 요청 상태 갱신
}

FTimeScaleHandle UTimeScaleComponent::RequestTimeScale(UObject* Requester, float TargetTimeScale, float BlendSpeed, float Duration)
{
	const int32 NewID = mNextHandleID++;

	FTimeScaleRequest TimeScaleRequest;
	TimeScaleRequest.TargetTimeScale = TargetTimeScale;
	TimeScaleRequest.CurrentTimeScale = 1.f;
	TimeScaleRequest.BlendSpeed = FMath::Max(BlendSpeed, mMinBlendSpeed);
	TimeScaleRequest.Duration = Duration;
	TimeScaleRequest.RemaingTime = Duration;
	TimeScaleRequest.Requester = Requester;
	TimeScaleRequest.bReleasing = false;

	mActivateRequests.Add(NewID, TimeScaleRequest);

	return FTimeScaleHandle(NewID);
}

void UTimeScaleComponent::ReleaseTimeScale(const FTimeScaleHandle& Handle)
{
	if (!Handle.IsValid())
		return;

	if (FTimeScaleRequest* Req = mActivateRequests.Find(Handle.ID))
	{
		Req->bReleasing = true;
		Req->TargetTimeScale = 1.f;
	}

}

void UTimeScaleComponent::UpdateTimeScale(float DeltaTime)
{
	mCurrentTimeScale = 1.f;

	for (auto& Pair : mActivateRequests)
	{
		Pair.Value.CurrentTimeScale = FMath::FInterpConstantTo(Pair.Value.CurrentTimeScale, Pair.Value.TargetTimeScale, DeltaTime, Pair.Value.BlendSpeed);
		mCurrentTimeScale *= Pair.Value.CurrentTimeScale;
	}

	mCurrentTimeScale = FMath::Max(mCurrentTimeScale, mMinTimeScale);

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), mCurrentTimeScale);
}

void UTimeScaleComponent::UpdateActivateRequests(float DeltaTime)
{
	// 제거 목록
	TArray<int32> KeysToRemove;

	for (auto& Pair : mActivateRequests)
	{
		FTimeScaleRequest& Req = Pair.Value;

		if (!Req.bReleasing)
		{
			if (Req.Duration == -1.f)
			{
				// 무기한 요청은 Requester 무효화 시에만 종료 트리거
				if (!Req.Requester.IsValid())
				{
					Req.bReleasing = true;
					Req.TargetTimeScale = 1.f;
				}
			}
			else
			{
				Req.RemaingTime -= DeltaTime;
				if (Req.RemaingTime <= 0.f)
				{
					Req.bReleasing = true;
					Req.TargetTimeScale = 1.f;
				}
			}
		}
		else if (FMath::IsNearlyEqual(Req.CurrentTimeScale, 1.f, KINDA_SMALL_NUMBER))
		{
			KeysToRemove.Add(Pair.Key);
		}
	}

	// 실제 제거
	RemoveExpiredActivateRequests(KeysToRemove);
}

void UTimeScaleComponent::RemoveExpiredActivateRequests(const TArray<int32>& KeysToRemove)
{
	for (int32 Key : KeysToRemove)
	{
		mActivateRequests.Remove(Key);
	}
}

