#pragma once

/**
 * @file CombatViewWidgetBase.h
 * @brief 전투 위젯들이 상속하는 베이스입니다. 뷰모델 하나에 묶여 표시·입력만 담당합니다.
 *
 * @details
 * 이 베이스를 상속한 WBP(CombatHUD/DicePanel/SkillPanel)는:
 *  - BindViewModel()로 공용 UCombatViewModel에 연결하고,
 *  - OnViewRefreshed/OnQueueNodePlayed(BlueprintImplementableEvent)에서 자기 화면만 다시 그리고,
 *  - 탭/터치는 ViewModel의 Request*()를 호출해 의도만 보낸다.
 * 위젯은 게임플레이/상태/RNG를 갖지 않는다. 진실은 뷰모델 너머의 게임플레이에 있다.
 */

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatViewTypes.h"

#include "CombatViewWidgetBase.generated.h"

class UCombatViewModel;

UCLASS(Abstract)
class P_RD_API UCombatViewWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 공용 뷰모델에 연결하고 갱신/큐 알림을 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|View")
	void BindViewModel(UCombatViewModel* InViewModel);

	UFUNCTION(BlueprintPure, Category = "Combat|View")
	UCombatViewModel* GetViewModel() const { return mViewModel; }

protected:
	/** @brief 해당 도메인이 갱신됐을 때 호출. WBP가 자기 화면을 다시 그린다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|View")
	void OnViewRefreshed(ECombatViewDomain Domain);

	/** @brief 행동 큐 노드 하나가 재생됐을 때 호출. WBP가 머리 위 숫자/효과를 띄운다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|View")
	void OnQueueNodePlayed(const FCombatQueueNode& Node);

	virtual void NativeDestruct() override;

private:
	UFUNCTION() void HandleViewChanged(ECombatViewDomain Domain);
	UFUNCTION() void HandleQueueNodeResolved(FCombatQueueNode Node);

	void UnbindViewModel();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Combat|View", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatViewModel> mViewModel;
};
