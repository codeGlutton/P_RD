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
DECLARE_MULTICAST_DELEGATE(FOnMainCameraReturned);

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

	/**
	 * @brief 뷰(카메라)가 강조에서 원위치로 완전히 돌아왔을 때 호출한다.
	 *
	 * 카드 복귀처럼 "카메라가 제자리인 다음에" 할 일들이 이 신호를 기다린다.
	 */
	void NotifyMainCameraReturned();

	/** @brief 강조 요청 뒤 아직 원위치로 못 돌아왔는가. */
	bool IsMainCameraEmphasized() const { return mIsMainCameraEmphasized; }

public:
	FOnRequestZoomInMainCamera OnRequestZoomInMainCamera;
	FOnRequestZoomOutMainCamera OnRequestZoomOutMainCamera;
	/** @brief 강조 복귀 완료 알림. NotifyMainCameraReturned 가 쏜다. */
	FOnMainCameraReturned OnMainCameraReturned;

private:
	/**
	 * @brief 강조 중 표시.
	 *
	 * 구독자(카메라 뷰)가 있을 때만 세운다. 심(시뮬레이션) 월드에는 카메라가
	 * 없어 복귀 통지도 오지 않는다 -- 거기서 세워 두면 영영 안 풀린다.
	 */
	bool mIsMainCameraEmphasized = false;
};
