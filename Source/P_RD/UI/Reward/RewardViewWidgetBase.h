#pragma once

/**
 * @file RewardViewWidgetBase.h
 * @brief 전투 보상 화면 WBP가 상속하는 베이스입니다. 보상 뷰모델에 묶여 표시·입력만 담당합니다.
 *
 * @details
 * 이 베이스를 상속한 WBP는:
 *  - BindViewModel()로 URewardViewModel에 연결하고,
 *  - OnRewardRefreshed(BlueprintImplementableEvent)에서 GetReward()를 읽어 돈/경험치 카운트업·막대를 그리고,
 *  - '받기' 버튼은 RequestClaim()으로 의도만 보낸다.
 * 위젯은 보상 지급/계산을 하지 않는다. 진실은 게임플레이에 있다.
 */

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardViewTypes.h"

#include "RewardViewWidgetBase.generated.h"

class URewardViewModel;

UCLASS(Abstract)
class P_RD_API URewardViewWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 보상 뷰모델에 연결하고 갱신 알림을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|View")
	void BindViewModel(URewardViewModel* InViewModel);

	UFUNCTION(BlueprintPure, Category = "Reward|View")
	URewardViewModel* GetViewModel() const { return mViewModel; }

	/** @brief '받기' 버튼이 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|View")
	void Claim();

protected:
	/** @brief 보상값이 들어왔을 때 호출. WBP가 돈/경험치 연출을 시작한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|View")
	void OnRewardRefreshed();

	virtual void NativeDestruct() override;

private:
	UFUNCTION() void HandleViewChanged();

	void UnbindViewModel();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Reward|View", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URewardViewModel> mViewModel;
};
