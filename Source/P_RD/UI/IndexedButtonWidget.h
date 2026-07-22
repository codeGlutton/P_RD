#pragma once

/**
 * @file IndexedButtonWidget.h
 * @brief 클릭할 때 자신의 번호도 같이 알려주는 버튼입니다.
 *
 * @details
 * 스킬 목록처럼 버튼을 여러 개 만들면, 어떤 버튼이 눌렸는지 번호가 필요합니다.
 * 기본 UButton은 번호를 알려주지 않으므로, 이 클래스가 클릭 이벤트에 번호를 붙여서 다시 알려줍니다.
 */

#include "RDMinimal.h"
#include "Components/Button.h"

#include "IndexedButtonWidget.generated.h"

/**
 * @brief 눌린 버튼 번호를 보내는 이벤트입니다.
 * @param ButtonIndex SetButtonIndex()로 넣어 둔 번호. 아직 넣지 않았다면 INDEX_NONE입니다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndexedButtonWidgetEvent, int32, ButtonIndex);

/**
 * @brief 여러 개 생성되는 버튼이 자기 번호를 같이 알려주게 만든 UButton입니다.
 *
 * @details
 * 버튼을 만드는 쪽에서 SetButtonIndex()로 번호를 넣어 둡니다.
 * 이후 클릭되면 OnIndexedClicked로 그 번호가 같이 넘어옵니다.
 *
 * @note 번호가 실제 배열 범위 안에 있는지는 호출하는 쪽에서 확인해야 합니다.
 */
UCLASS()
class P_RD_API UIndexedButtonWidget : public UButton
{
	GENERATED_BODY()

public:
	/**
	 * @brief 기본 버튼 이벤트를 번호가 붙은 이벤트로 연결합니다.
	 *
	 * @details
	 * 생성될 때 OnClicked/OnPressed를 내부 함수에 연결합니다.
	 * 호출하는 쪽은 OnIndexedClicked/OnIndexedPressed만 쓰면 됩니다.
	 */
	UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 이 버튼의 번호를 설정합니다.
	 *
	 * @details
	 * 보통 버튼을 만든 직후에 호출합니다.
	 *
	 * @param InButtonIndex 이 버튼이 가리키는 목록 번호.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void SetButtonIndex(int32 InButtonIndex);

	/**
	 * @brief 이 버튼의 번호를 돌려줍니다.
	 * @return SetButtonIndex()로 저장한 번호. 아직 설정하지 않았다면 INDEX_NONE입니다.
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Input")
	int32 GetButtonIndex() const;

	/**
	 * @brief 클릭됐을 때 버튼 번호와 함께 발생하는 이벤트입니다.
	 *
	 * UButton::OnClicked 대신 이 이벤트를 쓰면 어떤 항목이 눌렸는지 바로 알 수 있습니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedClicked;

	/**
	 * @brief 버튼을 누르는 순간 번호와 함께 발생하는 이벤트입니다.
	 *
	 * 클릭 확정 전 사운드나 강조 효과가 필요할 때 씁니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedPressed;

private:
	/**
	 * @brief 기본 클릭 이벤트를 번호가 붙은 클릭 이벤트로 바꿔 보냅니다.
	 */
	UFUNCTION()
	void HandleClicked();

	/**
	 * @brief 기본 누름 이벤트를 번호가 붙은 누름 이벤트로 바꿔 보냅니다.
	 */
	UFUNCTION()
	void HandlePressed();

	/**
	 * @brief 이 버튼이 가리키는 목록 번호입니다.
	 *
	 * 설정 전 상태를 구분하려고 INDEX_NONE으로 시작합니다.
	 */
	UPROPERTY(Category = "UI|Input", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 mButtonIndex = INDEX_NONE;
};
