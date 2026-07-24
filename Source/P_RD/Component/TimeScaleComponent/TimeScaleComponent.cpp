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

FTimeScaleHandle UTimeScaleComponent::RequestTimeScale(UObject* Requester, float TargetTimeScale, int32 Priority, float BlendSpeed, float Duration)
{
	const int32 NewID = mNextHandleID++;

	FTimeScaleRequest TimeScaleRequest;
	TimeScaleRequest.TargetTimeScale = TargetTimeScale;
	TimeScaleRequest.Priority = Priority;
	TimeScaleRequest.BlendSpeed = BlendSpeed;
	TimeScaleRequest.Duration = Duration;
	TimeScaleRequest.RemaingTime = Duration;
	TimeScaleRequest.Requester = Requester;

	mActivateRequests.Add(NewID, TimeScaleRequest);

	return FTimeScaleHandle(NewID);
}

void UTimeScaleComponent::ReleaseTimeScale(FTimeScaleHandle Handle)
{
	if (!Handle.IsValid())
		return;

	mActivateRequests.Remove(Handle.ID);

	// Handle을 무효화합니다.
	Handle.Invalidate();
}

void UTimeScaleComponent::UpdateTimeScale(float DeltaTime)
{
	//=====================================
	// 우선순위가 가장 높은 요청만 처리합니다.
	mTargetScale = 1.f;
	//mBlendSpeed = FLT_MAX;
	int32 Priority = -1.f;

	for (const auto& Pair : mActivateRequests)
	{
		if (Priority < Pair.Value.Priority)
		{
			mTargetScale = Pair.Value.TargetTimeScale;
			mBlendSpeed = Pair.Value.BlendSpeed;
			Priority = Pair.Value.Priority;
		}
	}

	mCurScale = FMath::FInterpConstantTo(mCurScale, mTargetScale, DeltaTime, mBlendSpeed);

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), mCurScale);
}

void UTimeScaleComponent::UpdateActivateRequests(float DeltaTime)
{
	// 제거 목록
	TArray<int32> KeysToRemove;

	for (auto& Pair : mActivateRequests)
	{
		Pair.Value.RemaingTime -= DeltaTime;

		// 주기가 -1이라면 무기한이므로 시간을 감소시키지 않습니다.
		if (Pair.Value.Duration == -1.f)
		{
			// 요청자가 무효하다면 요청을 제거합니다.
			if (!Pair.Value.Requester.IsValid())
			{
				// 제거 목록에 추가
				KeysToRemove.Add(Pair.Key);
			}

			continue;
		}

		// 지속시간이 다 지냈다면 종료합니다.
		if (Pair.Value.RemaingTime <= 0.f)
		{
			// 제거 목록에 추가
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

