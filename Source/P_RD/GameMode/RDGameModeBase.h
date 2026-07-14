/*****************************************************************//**
 * @file   RDGameModeBase.h
 * @brief  RD 프로젝트 게임 모드 베이스 정의 헤더
 * @author 모호재, 박용수
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Singleton/InstanceSubsystem/PersistentDataType.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "UI/FadeInOutWidget.h"
#include "UI/RDUserWidget.h"
#include "RDGameModeBase.generated.h"

class UUserPersistData;
class URunPersistData;

struct FRoomTransitionExecuteParams;

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogRDGameMode, Log, All)

/**
 * @brief RD 게임모드 상태 플래그
 */
enum class ERDFadeOutStateFlag : uint8
{
	None = 0,

	FadeBGMEnded = 1 << 0,
	FadeAnimationEnded = 1 << 1,

	ReadyToCallback = FadeBGMEnded | FadeAnimationEnded,
};
ENUM_CLASS_FLAGS(ERDFadeOutStateFlag);

/**
 * @brief  RD 프로젝트 게임 모드 베이스
 */
UCLASS(abstract)
class P_RD_API ARDGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARDGameModeBase();

	/* AGameModeBase 상속 */
protected:
	void BeginPlay() override;

protected:
	virtual void InitializeCommonRoom();
	virtual void InitializeRoom();
	virtual void BeginRoom();

	/* UI 진입점 */
public:
	/**
	 * @brief 현재 런이 살아 있는지 확인한다.
	 * @return 활성 RunPersistData가 있으면 true
	 */
	UFUNCTION(Category = UI, BlueprintPure)
	bool HasActiveRun() const;

	/**
	 * @brief 지금 런 포기 버튼을 사용할 수 있는지 조회한다.
	 * @return 런 포기 버튼을 활성화할 수 있으면 true
	 */
	UFUNCTION(Category = UI, BlueprintPure)
	bool CanAbandonRun() const;

public:
	UFUNCTION(Category = UI, BlueprintPure)
	bool ResetFromOptionPanel() const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool BackFromOptionPanel() const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetMasterVolume(float Volume) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetBGMVolume(float Volume) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetSFXVolume(float Volume) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetVoiceVolume(float Volume) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetUIVolume(float Volume) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetFpsLimit(int32 FpsLimit) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetOverallQuality(EOverallQualityType QualityType) const;

	UFUNCTION(Category = UI, BlueprintPure)
	bool SetLanguage(ELanguageType Language) const;

protected:
	/**
	 * @brief 페이드 레이어를 열고 검은 화면에서 게임 화면으로 드러내는 페이드인을 시작한다.
	 *
	 * @details
	 * OpenUI()는 페이드 레이어를 뷰포트에 올리는 공통 생명주기만 처리한다.
	 * 실제 페이드 방향과 완료 시점은 FadeInOut 위젯의 StartFadeIn()이 관리한다.
	 *
	 * @param OnEndFadeInAnimation 페이드인이 끝난 뒤 실행할 선택 콜백
	 */
	void StartFadeInUI(FOnEndFadeInAnimation OnEndFadeInAnimation = FOnEndFadeInAnimation()) const;

	/**
	 * @brief 페이드 레이어를 열고 다음 화면 전환 전에 화면을 검게 덮는 페이드아웃을 시작한다.
	 *
	 * @details
	 * OpenUI()는 페이드 레이어를 뷰포트에 올리는 공통 생명주기만 처리한다.
	 * 실제 페이드 방향과 완료 시점은 FadeInOut 위젯의 StartFadeOut()이 관리한다.
	 * 이 함수는 방 전환 여부를 판단하지 않고, 페이드아웃 완료 뒤 해야 할 일만 대리자로 받는다.
	 * 따라서 파생 GameMode는 단순 페이드아웃이나 별도 후속 작업을 같은 진입점으로 요청할 수 있다.
	 *
	 * @param OnEndFadeOutAnimation 페이드아웃이 끝난 뒤 실행할 선택 콜백
	 */
	void StartFadeOutUI(FOnEndFadeOutAnimation OnEndFadeOutAnimation = FOnEndFadeOutAnimation()) const;

	/**
	 * @brief 로딩 알림 위젯을 공통 OpenUI 생명주기로 연다.
	 *
	 * @param OnEndUIOpenAnimation 로딩 알림이 열린 뒤 실행할 선택 콜백
	 */
	void OpenLoadingNotifyUI(FOnEndUIOpenAnimation OnEndUIOpenAnimation = FOnEndUIOpenAnimation()) const;

	/**
	 * @brief 로딩 알림 위젯을 공통 CloseUI 생명주기로 닫는다.
	 *
	 * @param OnEndUICloseAnimation 로딩 알림이 닫힌 뒤 실행할 선택 콜백
	 */
	void CloseLoadingNotifyUI(FOnEndUICloseAnimation OnEndUICloseAnimation = FOnEndUICloseAnimation()) const;

protected:
	/**
	 * 방 로드 및 자동 트랜지션을 시도하는 함수
	 * @details
	 * mRequireExternalReady 설정 시, 외부 타이밍 알림에 전환 가능성이 종속적
	 * mShowFadeInUIOnAutoTransition 설정 시, 전환 이후에 FadeIn 애님 실행
	 * mShowFadeOutUIOnAutoTransition 설정 시, 전환 이전에 FadeOut 애님 실행
	 * 
	 * @param RoomRowIndex 다음 방 행 인덱스
	 * @param RoomColumnIndex 다음 방 열 인덱스
	 * @return 요청 성공 여부
	 */
	bool PreloadAndTransitionRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex);
	bool PreloadAndTransitionRoomAsync(EStageLevelType StageLevel);
	bool PreloadAndTransitionFrontendRoomAsync();

protected:
	bool MarkExternalReadyForTransition();

private:
	/**
	 * @brief 방 진입 직후 검은 전환 레이어를 걷는 페이드인을 시작한다.
	 */
	void StartFadeInUIForRoomTransition();

	/**
	 * @brief 방 전환 전 페이드아웃 완료 시 외부 준비 완료 신호를 전달한다.
	 *
	 * @details
	 * StartFadeOutUI()는 범용 페이드아웃 도구로 남기고, 방 전환에서만 필요한
	 * MarkExternalReadyForTransition() 연결은 이 private 함수에 모은다.
	 * 일반 방 이동, 새 스테이지 첫 방 이동, 프론트엔드 방 이동은 모두 페이드아웃이 끝난 뒤
	 * 같은 방식으로 ExternalReady를 전달해야 한다.
	 * 각 호출부가 같은 대리자 생성과 checkf 처리를 반복하면 전환 후속 작업이 바뀔 때 세 곳을 함께 고쳐야 하므로,
	 * 이 함수가 전환 전용 콜백을 한 번만 구성한다.
	 */
	void StartFadeOutUIForRoomTransition();

private:
	/**
	 * @brief 방 전환 준비 완료 시 UI 닫힘 타이밍에 맞춰 실제 전환을 이어간다.
	 *
	 * @param RoomRowIndex 준비된 방의 행 인덱스
	 * @param RoomColumnIndex 준비된 방의 열 인덱스
	 */
	void OnReadyToTransition(int32 RoomRowIndex, int32 RoomColumnIndex);
	void OnPreTransition(int32 RoomRowIndex, int32 RoomColumnIndex);

public:
	UUserPersistData* GetUserPersistData();
	URunPersistData* GetRunPersistData();
	const UUserPersistData* GetUserPersistData() const;
	const URunPersistData* GetRunPersistData() const;

protected:
	void ClearRunPersistData();

public:
	USoundBase* GetMainBGM() const;

protected:
	void SetMainBGM(USoundBase* BGM, bool IsOverride = true);

protected:
	UPROPERTY(Category = "UI", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "HUDClass"))
	TSubclassOf<UUserWidget> mHUDClass;
	UPROPERTY(Category = "UI", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "WorldWidgets"))
	TSet<EWorldWidgetType> mWorldWidgets;

protected:
	// @brief 다음 방 Preload 요청 여부
	bool mWasNextRoomPreloadRequested = false;

protected:
	// @brief 자동으로 현재 방 진입 시, Fade In UI를 띄울지 여부
	bool mShowFadeInUIOnTransition = false;
	// @brief 자동으로 다음 방 전환 시, Fade Out UI를 띄울지 여부
	bool mShowFadeOutUIOnTransition = false;
	// @brief 다음 방 전환 전, Preload 대기 중에 로딩 UI를 띄울지 여부
	bool mShowLoadingNotifyUIOnTransition = false;
	// @brief 다음 방 전환 전, 추가적인 작업을 대기해야하는지 여부
	bool mWaitExternalWorkOnTransition = false;

protected:
	float mBGMFadeInDuration = 1.5f;
	float mBGMFadeOutDuration = 1.5f;

private:
	UPROPERTY(Transient)
	mutable TObjectPtr<UAudioComponent> mBgmComponent = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> mMainBGM = nullptr;

protected:
	mutable ERDFadeOutStateFlag mFadeOutStateFlag = ERDFadeOutStateFlag::None;
};
