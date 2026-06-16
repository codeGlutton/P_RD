#pragma once

/**
 * @file MockRewardDriver.h
 * @brief 게임플레이 없이 보상 화면 UI를 만들고 테스트하기 위한 개발용 가짜 드라이버입니다.
 *
 * @details
 * Start()로 가짜 돈/경험치 보상을 뷰모델에 밀어넣고, '받기'(OnRewardClaimed)를 받으면 로그만 남긴다.
 * 실제 게임플레이가 붙으면 이 자리를 어댑터로 교체한다(위젯 무수정).
 */

#include "RDMinimal.h"

#include "MockRewardDriver.generated.h"

class URewardViewModel;

UCLASS()
class P_RD_API UMockRewardDriver : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 뷰모델에 가짜 보상을 밀어넣고 '받기' 입력을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|Mock")
	void Start(URewardViewModel* ViewModel);

private:
	UFUNCTION() void HandleClaimed();

	UPROPERTY(Transient) TObjectPtr<URewardViewModel> mViewModel;
};
