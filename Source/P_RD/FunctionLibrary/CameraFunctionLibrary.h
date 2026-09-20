/*****************************************************************//**
 * @file   CameraFunctionLibrary.h
 * @brief  카메라 관련 정적 헬퍼 함수 라이브러리 헤더
 * @author 모호재
 * @date   2026-08-11
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CameraFunctionLibrary.generated.h"

class ACombatCameraPawn;

/**
 * @brief 카메라 연관 헬퍼 정적 함수 라이브러리
 */
UCLASS()
class P_RD_API UCameraFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static bool IsCameraShakePossible(const UObject* WorldContextObject);

public:
	static FIntVector2 GetMainViewportSize(const UObject* WorldContextObject);
	static FVector2D GetSizeOnMainViewport(const UObject* WorldContextObject, const FVector2D& SizeRatio);

public:
	static ACombatCameraPawn* GetMainCameraPawn(const UObject* WorldContextObject);

private:
	static APlayerController* GetMainController(const UObject* WorldContextObject);
};
