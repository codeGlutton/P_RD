#pragma once

/** @brief 전투 위젯들이 상속하는 베이스입니다. 뷰모델 하나에 묶여 표시와 입력 전달만 담당합니다. */
// WBP는 BindUIModel()로 공용 UCombatUIModel에 연결하고 OnUIRefreshed/OnQueueNodePlayed에서 자기 화면만 다시 그린다.
// 탭/터치는 UIModel의 Request*()를 호출해 의도만 보낸다. 위젯은 게임플레이 상태나 RNG를 직접 소유하지 않는다.

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"
#include "UI/Combat/CombatUITypes.h"

#include "CombatUIWidgetBase.generated.h"

class UCombatUIModel;

UCLASS(Abstract)
class P_RD_API UCombatUIWidgetBase : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 공용 뷰모델에 연결하고 갱신/큐 알림을 구독한다. */
	// 패널 재사용 가능성을 위해 기존 구독을 먼저 끊고, 새 UIModel 연결 직후 All 갱신을 받는다.
	UFUNCTION(BlueprintCallable, Category = "Combat|View")
	virtual void BindUIModel(UCombatUIModel* InUIModel);

	/**
	 * @brief 구독을 끊는다.
	 *
	 * 파생 위젯이 자기 몫의 구독을 더 붙일 수 있으므로 protected 다. 붙인
	 * 쪽이 떼야 하는데 private 이면 뗄 자리가 없다.
	 */
	virtual void UnbindUIModel();

	/** @brief WBP가 자기 도메인 데이터를 읽을 때 사용하는 UIModel 접근자. */
	UFUNCTION(BlueprintPure, Category = "Combat|View")
	UCombatUIModel* GetUIModel() const { return mUIModel; }

protected:
	/** @brief 해당 도메인이 갱신됐을 때 호출. WBP가 자기 화면을 다시 그린다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|View")
	void OnUIRefreshed(ECombatUIDomain Domain);

	/** @brief 행동 큐 노드 하나가 재생됐을 때 호출. WBP가 머리 위 숫자/효과를 띄운다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|View")
	void OnQueueNodePlayed(const FCombatQueueNode& Node);

	/**
	 * @brief 위 두 이벤트의 C++ 판.
	 *
	 * @details
	 * OnUIRefreshed/OnQueueNodePlayed는 BlueprintImplementableEvent라 C++
	 * 파생 클래스가 재정의할 수 없다. 배치를 코드로 조립하는 HUD는 블루프린트
	 * 그래프 없이 갱신을 받아야 해서, 같은 시점에 불리는 네이티브 훅을 함께 둔다.
	 * 블루프린트로 만드는 위젯은 이 둘을 몰라도 된다.
	 */
	virtual void NativeOnUIRefreshed(ECombatUIDomain Domain) {}
	virtual void NativeOnQueueNodePlayed(const FCombatQueueNode& Node) {}

	/** @brief 화면 이탈 시 UIModel 델리게이트 구독을 해제한다. */
	virtual void NativeDestruct() override;

private:
	/** @brief UIModel 도메인 갱신을 WBP 구현 이벤트로 변환한다. */
	UFUNCTION() void HandleUIChanged(ECombatUIDomain Domain);

	/** @brief Queue resolved 이벤트를 WBP의 큐 노드 재생 이벤트로 변환한다. */
	UFUNCTION() void HandleQueueNodeResolved(FCombatQueueNode Node);

	/** @brief 현재 UIModel 구독을 끊고 참조를 비운다. */


protected:
	/** @brief 연결된 공용 전투 UIModel; 위젯은 소유하지 않고 구독/읽기만 한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat|View", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatUIModel> mUIModel;
};
