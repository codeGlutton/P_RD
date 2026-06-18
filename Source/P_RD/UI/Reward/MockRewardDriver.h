#pragma once

/** @brief 게임플레이 없이 보상 화면 UI를 만들고 테스트하기 위한 개발용 가짜 드라이버입니다. */
// @file MockRewardDriver.h
// Start()로 가짜 돈/경험치 보상을 뷰모델에 밀어넣고, '받기'(OnRewardClaimed)를 받으면 로그만 남긴다.
// 실제 게임플레이가 붙으면 이 자리를 어댑터로 교체한다(위젯 무수정).

#include "RDMinimal.h"

#include "MockRewardDriver.generated.h"

class URewardViewModel;

UCLASS()
class P_RD_API UMockRewardDriver : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 뷰모델에 가짜 보상을 밀어넣고 '받기' 입력을 구독한다. */
	// 실제 보상 지급/전환 로직을 대신하지 않는다. WBP가 ViewModel 계약을 구현했는지 확인하는 개발용 진입점이다.
	UFUNCTION(BlueprintCallable, Category = "Reward|Mock")
	void Start(URewardViewModel* ViewModel);

private:
	/** @brief Mock에서는 Claim 신호가 왕복했는지만 로그로 확인한다. */
	UFUNCTION() void HandleClaimed();

	/** @brief Mock에서는 3택1 선택 신호가 왕복했는지만 로그로 확인한다. */
	UFUNCTION() void HandleChosen(int32 ChoiceIndex);

	/** @brief 테스트 중 ViewModel이 GC되지 않게 들고 있는 참조; Mock이 실제 소유자는 아니다. */
	UPROPERTY(Transient) TObjectPtr<URewardViewModel> mViewModel;
};
