#pragma once

/** @brief 전투 보상 화면 WBP가 상속하는 베이스입니다. 보상 뷰모델에 묶여 표시·입력만 담당합니다. */
// @file RewardUIWidgetBase.h
// 이 베이스를 상속한 WBP는:
// - BindUIModel()로 URewardUIModel에 연결하고,
// - OnRewardRefreshed(BlueprintImplementableEvent)에서 GetReward()를 읽어 돈/경험치 카운트업·막대를 그리고,
// - '받기' 버튼은 RequestClaim()으로 의도만 보낸다.
// 위젯은 보상 지급/계산을 하지 않는다. 진실은 게임플레이에 있다.

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardUIWidgetBase.generated.h"

class URewardUIModel;

UCLASS(Abstract)
class P_RD_API URewardUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 보상 뷰모델에 연결하고 갱신 알림을 구독한다. */
	// 같은 WBP 인스턴스가 다른 전투 보상에 재사용될 수 있으므로 기존 구독을 먼저 끊는다.
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void BindUIModel(URewardUIModel* InUIModel);

	/** @brief WBP가 카운트업/막대 연출 중 현재 UIModel을 읽기 위한 접근자. */
	UFUNCTION(BlueprintPure, Category = "Reward|UI")
	URewardUIModel* GetUIModel() const { return mUIModel; }

	/** @brief '받기' 버튼이 호출. 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void Claim();

	/** @brief 3택1 선택지 카드가 호출. 선택 의도만 뷰모델로 넘긴다. */
	UFUNCTION(BlueprintCallable, Category = "Reward|UI")
	void ChooseReward(int32 ChoiceIndex);

protected:
	/** @brief 보상값이 들어왔을 때 호출. WBP가 돈/경험치 연출을 시작한다. */
	// C++은 표시 데이터 전달까지만 담당하고, 실제 애니메이션 타이밍은 WBP가 결정한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|UI")
	void OnRewardRefreshed();

	/** @brief 3택1 선택지가 들어왔을 때 호출. WBP가 선택지 카드 3개를 그린다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|UI")
	void OnRewardChoicesRefreshed();

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 정리한다. */
	virtual void NativeDestruct() override;

private:
	/** @brief UIModel 변경 알림을 WBP 갱신 이벤트로 변환한다. */
	UFUNCTION() void HandleUIChanged();

	/** @brief 선택지 변경 알림을 WBP 선택지 갱신 이벤트로 변환한다. */
	UFUNCTION() void HandleChoicesChanged();

	/** @brief 현재 UIModel 구독을 해제하고 참조를 비운다. */
	void UnbindUIModel();

protected:
	/** @brief 현재 바인딩된 보상 상태 소유자; 위젯은 이 객체를 소유하지 않고 구독만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Reward|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URewardUIModel> mUIModel;
};
