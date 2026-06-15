#pragma once

/**
 * @file IndexedButtonWidget.h
 * @brief 자신의 index를 함께 알려주는 UButton 래퍼입니다.
 *
 * @details
 * 스킬 목록·주사위 목록처럼 버튼을 런타임에 N개 만들어 붙일 때, 어느 버튼이 눌렸는지를
 * 알아야 합니다. 기본 UButton의 OnClicked는 index를 싣지 않아 호출부가 람다 캡처나 별도
 * 매핑으로 index를 따라붙여야 합니다. 그 반복을 없애려고 클릭/누름을 index와 함께 다시
 * 쏘아주는 얇은 래퍼를 두었습니다.
 *
 * 이 클래스의 의도는 "목록 소유자"를 만드는 것이 아니라, UButton의 입력 이벤트에 목록 index
 * payload만 붙여주는 것입니다. 버튼 생성, 정렬, 데이터 소유권은 호출부가 계속 담당하고,
 * 이 위젯은 어떤 항목이 눌렸는지 전달하는 작은 어댑터 역할만 합니다.
 */

#include "RDMinimal.h"
#include "Components/Button.h"

#include "IndexedButtonWidget.generated.h"

/**
 * @brief 눌린 버튼의 index(ButtonIndex)를 실어 보내는 이벤트.
 * @param ButtonIndex SetButtonIndex()로 지정한 목록 index. 설정 전이면 INDEX_NONE입니다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndexedButtonWidgetEvent, int32, ButtonIndex);

/**
 * @brief 런타임으로 생성하는 버튼이 자신의 index를 함께 전달하도록 하는 작은 UButton 래퍼.
 *
 * @details
 * 런타임 UI 목록은 같은 버튼 클래스를 여러 번 생성한 뒤 각 항목의 데이터 index만 다르게 붙이는
 * 패턴이 많습니다. 이 클래스는 그 패턴에서 발생하는 "버튼 포인터 -> index" 역조회 코드를 없애고,
 * Blueprint와 C++ 양쪽 호출부가 동일한 이벤트 형태를 구독하게 하려는 용도입니다.
 *
 * @note mButtonIndex는 유효성 검사를 하지 않습니다. 배열 범위 검사는 실제 데이터를 소유한 호출부가
 * 처리해야 하며, 이 위젯은 전달받은 값을 그대로 다시 방송합니다.
 */
UCLASS()
class P_RD_API UIndexedButtonWidget : public UButton
{
	GENERATED_BODY()

public:
	/**
	 * @brief 부모 UButton 입력 이벤트를 index payload 이벤트로 연결합니다.
	 *
	 * @details
	 * 생성 시 OnClicked/OnPressed를 내부 핸들러에 한 번만 연결합니다. 그 뒤 호출부는 원본 UButton 이벤트를
	 * 직접 구독하지 않고 OnIndexedClicked/OnIndexedPressed만 구독하면 됩니다.
	 */
	UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 이 버튼이 목록에서 몇 번째인지 설정합니다.
	 *
	 * @details
	 * 일반적으로 목록을 구성하는 루프에서 버튼을 생성한 직후 호출합니다. 클릭 시점에 별도의
	 * 캡처/맵 조회를 하지 않도록 index를 버튼 인스턴스 안에 저장해 둡니다.
	 *
	 * @param InButtonIndex 호출부 목록에서 이 버튼이 대표하는 항목 index.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void SetButtonIndex(int32 InButtonIndex);

	/**
	 * @brief 설정된 index를 돌려줍니다.
	 * @return SetButtonIndex()로 저장한 index. 아직 설정하지 않았다면 INDEX_NONE입니다.
	 */
	UFUNCTION(BlueprintPure, Category = "UI|Input")
	int32 GetButtonIndex() const;

	/**
	 * @brief 클릭 시 index와 함께 발생하는 이벤트입니다.
	 *
	 * 호출부는 UButton::OnClicked 대신 이 이벤트를 구독하면 어떤 항목이 선택됐는지 바로 알 수 있습니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedClicked;

	/**
	 * @brief 누름(press) 시 index와 함께 발생하는 이벤트입니다.
	 *
	 * 클릭 확정 전의 press 피드백, 사운드, hover-like 강조가 필요한 UI에서 사용합니다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedPressed;

private:
	/**
	 * @brief 부모 UButton의 OnClicked를 받아 OnIndexedClicked로 index를 실어 다시 방송합니다.
	 */
	UFUNCTION()
	void HandleClicked();

	/**
	 * @brief 부모 UButton의 OnPressed를 받아 OnIndexedPressed로 index를 실어 다시 방송합니다.
	 */
	UFUNCTION()
	void HandlePressed();

	/**
	 * @brief 이 버튼의 목록 내 위치입니다.
	 *
	 * 설정 전 상태를 명확히 구분하려고 INDEX_NONE으로 시작합니다. 이 값은 UI 목록의 데이터 소유권을
	 * 뜻하지 않으며, 이벤트 payload로만 사용됩니다.
	 */
	UPROPERTY(Category = "UI|Input", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 mButtonIndex = INDEX_NONE;
};
