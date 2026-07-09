/*****************************************************************//**
 * @file   CinematicWidget.h
 * @brief  시네마틱 UI 베이스
 * @details
 * 인트로/시네마틱 MP4를 MediaPlayer로 재생해 슬레이트 브러시 위에 깔고,
 * 뷰포트에 cover(좌우맞춤 + 상하크롭) 비율로 채운 뒤 재생 종료/타임아웃 시
 * 콜백으로 프론트엔드 전환을 알리는 위젯 베이스다.
 * @author 박용수
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UI/RDUserWidget.h"

#include "CinematicWidget.generated.h"

class SBorder;
class STextBlock;
class SWidget;
class UCinematicWidget;
class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;

/** @brief 시네마틱 재생이 끝났을 때(정상 종료/타임아웃 포함) 호출되는 콜백. 매개변수는 종료된 위젯 자신. */
DECLARE_DELEGATE_OneParam(FOnEndCinematicAnimation, UCinematicWidget*)

/**
 * @brief 시네마틱 표시, UI 열림/닫힘, 재생 완료 알림을 제공하는 위젯 베이스
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCinematicWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	/** @brief 기본 생성자. 시네마틱 관련 슬레이트/미디어 상태를 초기화한다. */
	UCinematicWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * @brief 시네마틱 영상 재생을 시작하고, 종료 시 호출할 콜백을 등록한다.
	 * @param Callback 재생이 끝났을 때(정상 종료 또는 타임아웃) 호출되는 델리게이트. 비워두면 알림 없이 종료만 처리한다.
	 */
	void PlayCinematic(FOnEndCinematicAnimation Callback = FOnEndCinematicAnimation());

	/** @brief 기본 인트로 경로 대신 재생할 Content 상대/절대 mp4 경로를 지정한다. */
	void SetCinematicVideoPath(const FString& InVideoPath);

	/** @brief 재생 종료 시 마지막 프레임을 유지할지 설정한다. */
	void SetHoldLastFrameOnFinish(bool bInHoldLastFrameOnFinish);

	/** @brief 이 시네마틱 위젯의 뷰포트 ZOrder를 지정한다. */
	void SetCinematicViewportZOrder(int32 InViewportZOrder);

	/** @brief 시네마틱을 즉시 종료 처리하고 등록된 종료 콜백을 발생시킨다(중복 호출 방지됨). */
	UFUNCTION(BlueprintCallable, Category = "UI|Cinematic")
	void FinishCinematic();

	/** @brief 프론트엔드 로드가 늦을 때 검은 로딩 대기 화면으로 페이드 전환한다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Cinematic")
	void FadeToLoadingWaitScreen();

protected:
	/**
	 * @brief 슬레이트 트리를 구성한다. 영상 브러시 이미지와 로딩 대기 레이어를 만들어 반환한다.
	 * @return 위젯 루트 슬레이트.
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** @brief 위젯 파괴 시 미디어 재생 정지 및 타이머 정리를 수행한다. */
	virtual void NativeDestruct() override;

	/** @brief 시네마틱 애니메이션 재생 진입점(블루프린트 오버라이드 가능). */
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Cinematic")
	void PlayCinematicAnimation();

	/** @brief PlayCinematicAnimation의 C++ 기본 구현. 영상 재생 또는 폴백 타이머를 시작한다. */
	virtual void PlayCinematicAnimation_Implementation();

	/** @brief UI 닫힘 애니메이션 기본 구현 오버라이드. 시네마틱 종료 페이드에 맞춰 닫는다. */
	virtual void PlayCloseUIAnimation_Implementation() override;

private:
	/** @brief 검은 화면 페이드의 목적(어떤 후속 동작을 위해 페이드하는지)을 구분하는 상태값. */
	enum class ECinematicFadePurpose : uint8
	{
		None,        ///< 진행 중인 페이드 없음
		LoadingWait, ///< 페이드 완료 후 로딩 대기 화면 유지
		Close,       ///< 페이드 완료 후 시네마틱 UI 닫고 종료
	};

	/** @brief MediaPlayer/MediaTexture/MediaSource 인스턴스가 없으면 생성하고 콜백 바인딩을 보장한다. */
	void EnsureCinematicMediaObjects();

	/**
	 * @brief 시네마틱 영상 파일을 열어 재생을 시도한다.
	 * @return 재생 시작에 성공하면 true, 파일 누락/오픈 실패 시 false.
	 */
	bool PlayCinematicVideo();

	/** @brief 검은 로딩 대기 레이어를 보이게 한다. */
	void ShowLoadingWaitScreen();

	/** @brief 검은 로딩 대기 레이어를 숨긴다. */
	void HideLoadingWaitScreen();

	/**
	 * @brief 검은 화면으로의 페이드를 시작한다.
	 * @param FadePurpose 페이드 완료 후 수행할 동작(로딩 대기 유지 / UI 닫기).
	 */
	void StartFadeToBlack(ECinematicFadePurpose FadePurpose);

	/** @brief 페이드 타이머마다 호출되어 경과 시간에 따라 검은 레이어 불투명도를 갱신한다. */
	void TickFadeToBlack();

	/** @brief 페이드가 끝났을 때 목적(mFadePurpose)에 따라 후속 동작을 마무리한다. */
	void FinishFadeToBlack();

	/**
	 * @brief 로딩 대기 레이어의 불투명도를 설정한다(페이드 진행 표현용).
	 * @param Opacity 0.0(투명)~1.0(완전한 검은 화면).
	 */
	void SetLoadingWaitLayerOpacity(float Opacity);

	/** @brief 진행 중인 시네마틱 미디어 재생을 정지한다. */
	void StopCinematicMedia();

	/** @brief 재생 완료 경로에서 마지막 프레임 유지 옵션을 적용한 뒤 종료 콜백을 실행한다. */
	void FinishCinematicPlayback();

	/** @brief 지원되는 플랫폼에서는 종료 직전 프레임으로 되돌려 정지한다. */
	void HoldCinematicLastFrame();

	/**
	 * @brief 영상 종료 이벤트 미수신 대비 폴백 타이머를 시작한다.
	 * @param DurationSeconds 이 시간이 지나면 강제로 종료 처리한다.
	 */
	void StartDefaultCinematicTimer(float DurationSeconds);

	/** @brief 폴백(기본 지속시간) 타이머를 해제한다. */
	void ClearDefaultCinematicTimer();

	/** @brief 페이드 진행 타이머를 해제한다. */
	void ClearCinematicFadeTimer();

	/** @brief 게임 설정의 인트로 영상 상대 경로를 실제 재생 가능한 절대/플랫폼 경로로 변환한다. */
	FString ResolveCinematicVideoPath() const;
	/** @brief MP4 파일의 tkhd 박스에서 영상 픽셀 해상도를 직접 읽는다(미디어 재생 타이밍과 무관하게 cover 비율 확정용). 실패 시 0. */
	FVector2D ReadCinematicVideoFileDimensions(const FString& VideoPath) const;

	/**
	 * @brief 미디어 오픈 성공 콜백. 플레이어에서 영상 실제 해상도를 읽어 cover 비율을 확정한다.
	 * @param OpenedUrl 실제로 열린 미디어 URL.
	 */
	UFUNCTION()
	void HandleCinematicMediaOpened(FString OpenedUrl);

	/**
	 * @brief 미디어 오픈 실패 콜백. 폴백 경로로 진행해 인트로가 멈추지 않게 한다.
	 * @param FailedUrl 열기에 실패한 미디어 URL.
	 */
	UFUNCTION()
	void HandleCinematicMediaOpenFailed(FString FailedUrl);

	/** @brief 영상이 끝까지 재생됐을 때 콜백. 즉시 종료 처리로 이어진다. */
	UFUNCTION()
	void HandleCinematicMediaEndReached();

protected:
	/**
	 * @brief 영상 재생 이벤트가 들어오지 않았을 때 인트로가 멈추지 않도록 기다릴 기본 시간
	 *
	 * @details
	 * MediaPlayer가 영상을 정상 종료하면 OnEndReached에서 즉시 FinishCinematic()을 호출한다.
	 * 파일 누락, 플랫폼별 미디어 초기화 실패, 종료 이벤트 미수신 같은 상황에서는 이 시간이 지나면
	 * 프론트엔드 전환 조건을 계속 진행한다.
	 */
	UPROPERTY(Category = "UI|Cinematic", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", DisplayName = "Default Cinematic Duration"))
	float mDefaultCinematicDuration = 3.25f;

	/**
	 * @brief 인트로 영상이 끝난 뒤 검은 화면으로 자연스럽게 넘어가는 시간
	 *
	 * @details
	 * 프론트엔드 로드가 늦을 때만 검은 로딩 대기 화면으로 페이드한다.
	 * 로드가 이미 끝났다면 같은 페이드 시간을 사용해 인트로 HUD를 닫고 바로 타이틀로 전환한다.
	 */
	UPROPERTY(Category = "UI|Cinematic", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", DisplayName = "Fade Out Duration"))
	float mFadeOutDuration = 0.35f;

private:
	/** @brief 시네마틱 MP4 재생을 담당하는 미디어 플레이어. */
	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> mCinematicMediaPlayer;

	/** @brief 플레이어 출력 프레임을 받는 미디어 텍스처(브러시/머티리얼 소스). */
	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> mCinematicMediaTexture;

	/** @brief 재생할 영상 파일을 가리키는 파일 미디어 소스. */
	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> mCinematicMediaSource;

	/** @brief 재생 종료 시 알릴 콜백. PlayCinematic 호출 시 등록된다. */
	FOnEndCinematicAnimation OnEndCinematicAnimation;

	/** @brief 영상 텍스처를 화면에 깔기 위한 슬레이트 브러시. */
	FSlateBrush mCinematicVideoBrush;
	/** @brief 미디어가 열릴 때 플레이어에서 읽은 영상 실제 해상도(타이밍 안전한 cover 비율 기준). 0이면 미확정. */
	FVector2D mCinematicVideoNativeSize = FVector2D::ZeroVector;
	/** @brief 영상 브러시를 표시하는 슬레이트 이미지 위젯. */
	TSharedPtr<SWidget> mCinematicVideoImage;

	/** @brief 페이드/로딩 대기 시 화면을 덮는 검은 레이어. */
	TSharedPtr<SBorder> mLoadingWaitLayer;

	/** @brief 로딩 대기 화면에 표시하는 안내 텍스트. */
	TSharedPtr<STextBlock> mLoadingWaitText;

	/** @brief 종료 이벤트 미수신 대비 폴백(기본 지속시간) 타이머 핸들. */
	FTimerHandle mDefaultCinematicTimerHandle;

	/** @brief 검은 화면 페이드 진행 타이머 핸들. */
	FTimerHandle mCinematicFadeTimerHandle;

	/** @brief 현재 페이드의 목적(완료 후 동작 분기 기준). */
	ECinematicFadePurpose mFadePurpose = ECinematicFadePurpose::None;

	/** @brief 비어 있으면 인트로 기본 설정 경로를 사용한다. */
	FString mOverrideCinematicVideoPath;

	/** @brief 페이드 시작 후 누적 경과 시간(초). 불투명도 보간에 사용. */
	float mFadeElapsedTime = 0.0f;

	/** @brief 로딩 대기 레이어의 현재 불투명도(0~1). */
	float mLoadingWaitLayerOpacity = 0.0f;

	/** @brief 종료 처리가 이미 수행됐는지 여부. 종료 콜백 중복 발생을 막는다. */
	bool mCinematicFinished = false;

	/** @brief 재생 완료 후 CloseUI 전까지 마지막 프레임을 유지한다. */
	bool mHoldLastFrameOnFinish = false;
};
