#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"

#include "CombatStatusButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FCombatStatusButtonSignal, bool, bAlly, int32, SlotIndex);

/**
 * @brief 스크롤 상태 행에서 진영과 배열 index를 함께 전달하는 투명 버튼.
 *
 * UButton의 기본 OnPressed/OnReleased에는 송신자 인자가 없어 동적으로 늘어나는
 * 상태 행을 한 핸들러에 묶을 수 없다. 이 버튼이 자신의 문맥을 보태 전달한다.
 */
UCLASS()
class P_RD_API UCombatStatusButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(bool bInAlly, int32 InSlotIndex);

	UPROPERTY(BlueprintAssignable)
	FCombatStatusButtonSignal OnStatusPressed;

	UPROPERTY(BlueprintAssignable)
	FCombatStatusButtonSignal OnStatusReleased;

private:
	UFUNCTION()
	void ForwardPressed();

	UFUNCTION()
	void ForwardReleased();

	UPROPERTY(Transient)
	bool mAlly = false;

	UPROPERTY(Transient)
	int32 mSlotIndex = INDEX_NONE;
};
