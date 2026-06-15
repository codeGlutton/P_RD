#pragma once

/**
 * @file MockCombatDriver.h
 * @brief 게임플레이가 준비되기 전, UCombatViewModel에 가짜 데이터를 밀어넣는 개발용 드라이버입니다.
 *
 * @details
 * 실제 게임플레이(모호재님 액션/시뮬레이션)가 붙기 전에 박용수님이 UI를 먼저 만들고 테스트할 수 있게,
 * 가짜 유닛/주사위/스킬을 Set*()으로 밀어넣고 위젯 입력(Request*)을 받아 흉내냅니다. 게임플레이가 붙으면
 * 이 드라이버를 실제 어댑터로 교체하기만 하면 됩니다(위젯은 무수정).
 */

#include "RDMinimal.h"
#include "UI/Combat/CombatViewTypes.h"

#include "MockCombatDriver.generated.h"

class UCombatViewModel;

UCLASS(BlueprintType)
class P_RD_API UMockCombatDriver : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 뷰모델에 가짜 초기 상태를 채우고 입력 델리게이트를 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mock")
	void Start(UCombatViewModel* ViewModel);

	/** @brief 가짜 예측 큐(데미지·상태이상·힐 3노드)를 만들어 밀어넣는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mock")
	void PushMockPreview();

	/** @brief 큐 맨 앞 노드를 하나 재생(처리)한다. 연속 공격 재생을 흉내낸다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mock")
	void PlayNextQueueNode();

private:
	UFUNCTION() void HandleCommand(ECombatInputType Type, int32 IntPayload);
	UFUNCTION() void HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	void RebuildDicePush();   // mDice를 뷰모델로 push

private:
	UPROPERTY(Transient) TObjectPtr<UCombatViewModel> mViewModel;
	UPROPERTY(Transient) TArray<FDiceSlotView> mDice;
	UPROPERTY(Transient) TArray<int32> mSelectedDice;
};
