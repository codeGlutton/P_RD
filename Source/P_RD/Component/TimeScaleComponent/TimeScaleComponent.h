/*****************************************************************//**
 * @file   TimeScaleComponent.h
 * @brief  시간 배율 컴포넌트
 * @author 김준형
 * @date   2026-07-24
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeScaleComponent.generated.h"

USTRUCT(BlueprintType)
struct P_RD_API FTimeScaleRequest
{
	GENERATED_BODY()

	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "목표 시간 배율", AllowPrivateAccess = "true"))
	float TargetTimeScale = 1.f;

	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "현재 시간 배율", AllowPrivateAccess = "true"))
	float CurrentTimeScale = 1.f;
	
	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "블랜드 속도", AllowPrivateAccess = "true"))
	float BlendSpeed = 0.f;
	
	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "지속 시간", AllowPrivateAccess = "true"))
	float Duration = 0.f;

	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "남은 시간", AllowPrivateAccess = "true"))
	float RemaingTime = 0.f;

	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "해제 상태", AllowPrivateAccess = "true"))
	bool bReleasing = false;

	UPROPERTY(Category = TimeScaleRequest, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "요청한 대상", AllowPrivateAccess = "true"))
	TWeakObjectPtr<UObject> Requester;
};

USTRUCT(BlueprintType)
struct P_RD_API FTimeScaleHandle
{
	GENERATED_BODY()

	FTimeScaleHandle() : ID(INDEX_NONE) {}

	bool IsValid() const { return ID != INDEX_NONE; }
	void Invalidate() { ID = INDEX_NONE; }

	bool operator==(const FTimeScaleHandle& Other) const { return ID == Other.ID; }
	friend uint32 GetTypeHash(const FTimeScaleHandle& Handle) { return GetTypeHash(Handle.ID); }

private:
	// 생성은 오직 Subsystem만 할 수 있게 friend로 제한
	friend class UTimeScaleComponent;
	explicit FTimeScaleHandle(int32 InID) : ID(InID) {}

	UPROPERTY()
	int32 ID;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P_RD_API UTimeScaleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTimeScaleComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	int32 mNextHandleID = 0;

	UPROPERTY(Category = TimeScale, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "시간 조정 요청", AllowPrivateAccess = "true"))
	TMap<int32, FTimeScaleRequest> mActivateRequests;

	UPROPERTY(Category = TimeScale, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "최소 시간 배율", AllowPrivateAccess = "true"))
	float mMinTimeScale = 0.01f;

	UPROPERTY(Category = TimeScale, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "최소 변경 시간", AllowPrivateAccess = "true"))
	float mMinBlendSpeed = 0.01f;

	UPROPERTY(Category = TimeScale, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "현재 시간 배율", AllowPrivateAccess = "true"))
	float mCurrentTimeScale = 1.f;


public:
	/*Get, Set*/

public:
	/*
	* @brief 시간 배율 조정을 요청하는 함수입니다.
	* 
	* @details 
	* Duration이 끝나고 배율 복구까지 시간이 걸리므로 총 (변경 시간 + 유지시간)Duration + 변경시간 동안 시간 배율 변경됩니다.
	* 
	* @params Requester 요청자
	* 무기한 시간 배율 요청자가 존재할 시 요청자가 제거되면 시간 배율 요청도 제거하기 위한 파라미터입니다.
	* 
	* @params TargetScale
	* 변경 시키고 싶은 TimeScale 값입니다.
	* 
	* @params BlendSpeed TargetTimeScale에 도달하는 속도입니다.
	* BlendSpeed = 1 이라면 1초동안 배율을 변경시킵니다.
	* BlendSpeed = 2 이라면 0.5초 동안 배율을 변경시킵니다.
	* BlendSpeed = MAX이라면 거의 즉시 배율을 변경됩니다.
	* 
	* @params Duration 요청 후 시간 배율 조정이 유지되는 시간입니다.
	* Duration은 게임 시간 기준입니다. TimeScale이 감소할 수록 실제시간으로 오래 걸리며, TimeScale이 증가할 수록 실제 시간으로 짧게 걸립니다.
	* Duration = -1이라면 무기한 유지됩니다.
	* Duration = 1이라면 요청이 들어오고 1초 후 배율 복구하기 시작합니다.
	* 
	* @return FTileScaleHandle
	* 요청을 해제할 때 사용하는 구조체입니다.
	*/
	UFUNCTION(BlueprintCallable)
	FTimeScaleHandle RequestTimeScale(UObject* Requester, float TargetTimeScale, float BlendSpeed = 3.402823466e+38f, float Duration = -1.f);


	/*
	* @brief 시간 배율 요청을 해제합니다.
	* 
	* @params Handle
	* RequestTimeScale에서 반환된 Handle을 매개변수로 넣어서 시간 배율 조정을 해제합니다.
	* 해제하여도 복구하는데 시간이 걸립니다.
	*/
	UFUNCTION(BlueprintCallable)
	void ReleaseTimeScale(const FTimeScaleHandle& Handle);

private:

	/*
	* @brief 요청 처리
	*/
	void UpdateTimeScale(float DeltaTime);

	/*
	* @brief 요청 상태 갱신
	*/
	void UpdateActivateRequests(float DeltaTime);      
	
	/*
	* @brief 만료된 요청 제거
	*/
	void RemoveExpiredActivateRequests(const TArray<int32>& KeysToRemove); // 실제 제거
};
