/*****************************************************************//**
 * @file   CinematicWidget.h
 * @brief  시네마틱 UI 베이스
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UI/RDUserWidget.h"

#include "CinematicWidget.generated.h"

class SImage;
class SBorder;
class STextBlock;
class UCinematicWidget;
class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;

DECLARE_DELEGATE_OneParam(FOnEndCinematicAnimation, UCinematicWidget*)

/**
 * @brief 시네마틱 표시, UI 열림/닫힘, 재생 완료 알림을 제공하는 위젯 베이스
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API UCinematicWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	UCinematicWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void PlayCinematic(FOnEndCinematicAnimation Callback = FOnEndCinematicAnimation());

	UFUNCTION(BlueprintCallable, Category = "UI|Cinematic")
	void FinishCinematic();

	UFUNCTION(BlueprintCallable, Category = "UI|Cinematic")
	void FadeToLoadingWaitScreen();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintNativeEvent, Category = "UI|Cinematic")
	void PlayCinematicAnimation();
	virtual void PlayCinematicAnimation_Implementation();
	virtual void PlayCloseUIAnimation_Implementation() override;

private:
	enum class ECinematicFadePurpose : uint8
	{
		None,
		LoadingWait,
		Close,
	};

	void EnsureCinematicMediaObjects();
	bool PlayCinematicVideo();
	void ShowLoadingWaitScreen();
	void HideLoadingWaitScreen();
	void StartFadeToBlack(ECinematicFadePurpose FadePurpose);
	void TickFadeToBlack();
	void FinishFadeToBlack();
	void SetLoadingWaitLayerOpacity(float Opacity);
	void StopCinematicMedia();
	void StartDefaultCinematicTimer(float DurationSeconds);
	void ClearDefaultCinematicTimer();
	void ClearCinematicFadeTimer();
	FString ResolveCinematicVideoPath() const;

	UFUNCTION()
	void HandleCinematicMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void HandleCinematicMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void HandleCinematicMediaEndReached();

protected:
	/**
	 * @brief 인트로에서 재생할 MP4 파일의 Content 기준 상대 경로
	 *
	 * @details
	 * AI 생성 원본 영상은 SVN 규칙에 맞춰 OutSideAsset/AICreation 아래에 둔다.
	 * Android 패키징에서 MediaPlayer가 직접 파일을 열 수 있도록 DefaultGame.ini의 NonUFS 스테이징 경로에도
	 * 같은 폴더를 등록해 둔다.
	 */
	UPROPERTY(Category = "UI|Cinematic", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Cinematic Video Path"))
	FString mCinematicVideoPath = TEXT("SVN/OutSideAsset/AICreation/MS_IntroCinematic_02.mp4");

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
	UPROPERTY(Transient)
	TObjectPtr<UMediaPlayer> mCinematicMediaPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UMediaTexture> mCinematicMediaTexture;

	UPROPERTY(Transient)
	TObjectPtr<UFileMediaSource> mCinematicMediaSource;

	FOnEndCinematicAnimation OnEndCinematicAnimation;
	FSlateBrush mCinematicVideoBrush;
	TSharedPtr<SImage> mCinematicVideoImage;
	TSharedPtr<SBorder> mLoadingWaitLayer;
	TSharedPtr<STextBlock> mLoadingWaitText;
	FTimerHandle mDefaultCinematicTimerHandle;
	FTimerHandle mCinematicFadeTimerHandle;
	ECinematicFadePurpose mFadePurpose = ECinematicFadePurpose::None;
	float mFadeElapsedTime = 0.0f;
	float mLoadingWaitLayerOpacity = 0.0f;
	bool mCinematicFinished = false;
};
