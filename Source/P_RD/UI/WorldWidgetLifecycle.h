/*****************************************************************//**
 * @file   WorldWidgetLifecycle.h
 * @brief  월드 공용 위젯 열기/닫기 생명주기 인터페이스 정의 헤더
 * @author Codex
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"

#include "WorldWidgetLifecycle.generated.h"

/**
 * @brief  월드 공용 위젯 열기/닫기 생명주기 인터페이스
 */
UINTERFACE(BlueprintType)
class P_RD_API UWorldWidgetLifecycle : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  위젯별 열기/닫기 연출을 숨기기 위한 인터페이스
 */
class P_RD_API IWorldWidgetLifecycle
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = UI, BlueprintCallable, BlueprintNativeEvent)
	void OpenUI();

	UFUNCTION(Category = UI, BlueprintCallable, BlueprintNativeEvent)
	void CloseUI();
};
