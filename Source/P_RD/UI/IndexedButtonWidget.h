#pragma once

/**
 * @file IndexedButtonWidget.h
 * @brief 자신의 index를 함께 알려주는 UButton 래퍼입니다.
 *
 * 스킬 목록·주사위 목록처럼 버튼을 런타임에 N개 만들어 붙일 때, 어느 버튼이 눌렸는지를
 * 알아야 합니다. 기본 UButton의 OnClicked는 index를 싣지 않아 호출부가 람다 캡처나 별도
 * 매핑으로 index를 따라붙여야 합니다. 그 반복을 없애려고 클릭/누름을 index와 함께 다시
 * 쏘아주는 얇은 래퍼를 두었습니다.
 */

#include "RDMinimal.h"
#include "Components/Button.h"

#include "IndexedButtonWidget.generated.h"

/** @brief 눌린 버튼의 index(ButtonIndex)를 실어 보내는 이벤트. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIndexedButtonWidgetEvent, int32, ButtonIndex);

/**
 * @brief 런타임으로 생성하는 버튼이 자신의 index를 함께 전달하도록 하는 작은 UButton 래퍼.
 */
UCLASS()
class P_RD_API UIndexedButtonWidget : public UButton
{
	GENERATED_BODY()

public:
	UIndexedButtonWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief 이 버튼이 목록에서 몇 번째인지 설정한다. 생성 직후 호출해 두는 것이 전제. */
	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void SetButtonIndex(int32 InButtonIndex);

	/** @brief 설정된 index를 돌려준다. 설정 전이면 INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category = "UI|Input")
	int32 GetButtonIndex() const;

	/** @brief 클릭 시 index와 함께 발생하는 이벤트. 호출부는 이것만 구독하면 된다. */
	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedClicked;

	/** @brief 누름(press) 시 index와 함께 발생하는 이벤트. */
	UPROPERTY(BlueprintAssignable, Category = "UI|Input")
	FIndexedButtonWidgetEvent OnIndexedPressed;

private:
	/** @brief 부모 UButton의 OnClicked를 받아 OnIndexedClicked로 index를 실어 다시 쏜다. */
	UFUNCTION()
	void HandleClicked();

	/** @brief 부모 UButton의 OnPressed를 받아 OnIndexedPressed로 index를 실어 다시 쏜다. */
	UFUNCTION()
	void HandlePressed();

	// @brief 이 버튼의 목록 내 위치. 설정 전 상태를 구분하려고 INDEX_NONE으로 시작한다.
	UPROPERTY(Category = "UI|Input", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 mButtonIndex = INDEX_NONE;
};
