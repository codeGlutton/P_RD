/*****************************************************************//**
 * @file   IntroGameMode.h
 * @brief  인트로 용 게임 모드 정의 헤더
 * @author 모호재
 * @date   2026-06-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameMode/RDGameModeBase.h"
#include "IntroGameMode.generated.h"

class UCinematicWidget;

/**
 * @brief 인트로 게임모드 상태 플래그
 *
 * @details
 * 인트로 종료 조건은 영상 완료 하나가 아니라, 유저 세이브 로드 + 런 세이브 로드 + 시네마틱 종료 세 가지다.
 * 각 비동기 콜백이 자기 플래그만 세우고 TryToMarkExternalReady()가 합류 지점을 맡는다.
 */
enum class EIntroGameModeStateFlag : uint8
{
	None = 0,

	UserDataLoaded = 1 << 0,
	RunDataLoaded = 1 << 1,
	CinematicAnimationEnded = 1 << 2,

	ReadyToTransition = UserDataLoaded | RunDataLoaded | CinematicAnimationEnded,
};
ENUM_CLASS_FLAGS(EIntroGameModeStateFlag);

/**
 * @brief  인트로 용 게임 모드
 */
UCLASS(abstract)
class P_RD_API AIntroGameMode : public ARDGameModeBase
{
	GENERATED_BODY()

public:
	AIntroGameMode();

	/* ARDGameModeBase 상속 */
protected:
	void BeginRoom() override;

private:
	void OnEndCinematicAnimation(UCinematicWidget* CinematicWidget);
	void OnLoadUserData(const FString& SlotName, int32 SlotIndex, USaveGame* SaveGame);
	void OnLoadRunData(const FString& SlotName, int32 SlotIndex, USaveGame* SaveGame);

protected:
	void TryToMarkExternalReady();

private:
	void StartLoadingWaitDelay();
	void ClearLoadingWaitDelay();
	void ShowLoadingWaitScreenIfStillNeeded();
	void MarkIntroExternalReady();

protected:
	EIntroGameModeStateFlag mStateFlag = EIntroGameModeStateFlag::None;

private:
	TWeakObjectPtr<UCinematicWidget> mCinematicHUD;
	FTimerHandle mLoadingWaitDelayTimerHandle;
	float mLoadingWaitDelaySeconds = 0.25f;
};
