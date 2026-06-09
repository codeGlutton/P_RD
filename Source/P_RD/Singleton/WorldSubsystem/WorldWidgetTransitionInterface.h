#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"

#include "Singleton/WorldSubsystem/WorldWidgetType.h"

#include "WorldWidgetTransitionInterface.generated.h"

/**
 * @brief WorldWidgetSubsystem의 Open/Close 흐름에서 위젯이 애니메이션을 끼워 넣기 위한 선택 인터페이스
 *
 * 이 인터페이스는 필수가 아니다. 일반 위젯은 구현하지 않아도 즉시 열리고 닫힌다.
 * 닫힘을 직접 처리할 위젯은 HandleWorldWidgetCloseRequested()에서 true를 반환하고,
 * 처리가 끝난 뒤 UWorldWidgetSubsystem::CompleteCloseWorldWidget()을 호출한다.
 */
UINTERFACE(BlueprintType)
class P_RD_API UWorldWidgetTransitionInterface : public UInterface
{
	GENERATED_BODY()
};

class P_RD_API IWorldWidgetTransitionInterface
{
	GENERATED_BODY()

public:
	/** @brief 위젯이 화면에 열린 직후 호출됨 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI|World Widget")
	void HandleWorldWidgetOpened(EWorldWidgetType WorldWidgetType);

	/**
	 * @brief 위젯 닫기가 요청되었을 때 호출됨
	 *
	 * @return true면 위젯이 닫힘 애니메이션을 직접 처리하며,
	 *         false면 WorldWidgetSubsystem이 즉시 Collapsed 처리한다.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI|World Widget")
	bool HandleWorldWidgetCloseRequested(EWorldWidgetType WorldWidgetType);

	/** @brief 위젯이 실제로 닫힌 직후 호출됨 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI|World Widget")
	void HandleWorldWidgetClosed(EWorldWidgetType WorldWidgetType);
};
