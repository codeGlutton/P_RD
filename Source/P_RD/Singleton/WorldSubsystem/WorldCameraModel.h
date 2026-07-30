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
	 * @brief 실제 카메라 뷰가 강조 요청을 받아들였을 때 호출한다.
	 *
	 * 요청 델리게이트가 연결됐다는 사실만으로는 카메라 Pawn/Component의
	 * 존재를 보장할 수 없다. 뷰가 요청을 실제로 처리한 뒤 이 함수로
	 * 확인해 줘야 강조 중 상태가 된다.
	 */
	void NotifyMainCameraEmphasisStarted();

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
	 * 실제 카메라 컴포넌트가 강조 요청을 처리했을 때만 세운다.
	 * 심(시뮬레이션) 월드처럼 중계 델리게이트만 있고 카메라가 없는 경우에는
	 * 세우지 않는다 -- 복귀 통지가 없는 월드에서 영영 잠기는 것을 막는다.
	 */
	bool mIsMainCameraEmphasized = false;
};
