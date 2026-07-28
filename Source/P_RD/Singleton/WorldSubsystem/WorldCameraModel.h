/*****************************************************************//**
 * @file   WorldCameraModel.h
 * @brief  월드 공간 상의 카메라 제어 서브시스템 모델 정의 헤더
 * @author 모호재
 * @date   2026-07-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "WorldCameraModel.generated.h"

// World Camera 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogWorldCamera, Log, All)

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRequestZoomInMainCamera, const FVector& /*Location*/, float /*ScreenSize*/);
DECLARE_MULTICAST_DELEGATE(FOnRequestZoomOutMainCamera);

/**
 * @brief  월드 공간 상의 카메라 제어 서브시스템 모델
 */
UCLASS()
class P_RD_API UWorldCameraModel : public UObjectModel
{
	GENERATED_BODY()

public:
	void RequestZoomInMainCamera(const FVector& Location, float ScreenSize);
	void RequestZoomOutMainCamera();

public:
	FOnRequestZoomInMainCamera OnRequestZoomInMainCamera;
	FOnRequestZoomOutMainCamera OnRequestZoomOutMainCamera;
};
