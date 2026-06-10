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
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "UI/RDUserWidget.h"
#include "RDGameModeBase.generated.h"

class UUserPersistData;
class URunPersistData;

struct FRoomTransitionExecuteParams;

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogRDGameMode, Log, All)

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

protected:
	/**
	 * @brief 현재 방 진입 시 페이드 레이어를 걷어내는 UI 흐름을 시작한다.
	 *
	 * @details
	 * FadeInOut 월드 위젯의 OpenUI() 완료 콜백에서 CloseUI()를 호출한다.
	 * GameMode는 페이드 구현 방식을 모르고, 위젯의 OpenUI/CloseUI 생명주기 완료 시점만 따른다.
	 */
	void StartFadeInUI() const;

	/**
	 * @brief 다음 방 전환 전에 화면을 덮고 외부 준비 완료를 알린다.
	 *
	 * @details
	 * 방 전환 시스템이 ExternalReady를 기다리는 경우, 화면이 완전히 페이드아웃된 뒤
	 * MarkExternalReadyForTransition()을 호출해야 새 레벨 전환이 검은 화면 뒤에서 진행된다.
	 */
	void StartFadeOutUI();

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
	 * @brief 방 전환 준비 완료 시 UI 닫힘 타이밍에 맞춰 실제 전환을 이어간다.
	 *
	 * @param RoomRowIndex 준비된 방의 행 인덱스
	 * @param RoomColumnIndex 준비된 방의 열 인덱스
	 */
	void OnReadyToTransition(int32 RoomRowIndex, int32 RoomColumnIndex);
	void OnPreTransition(int32 RoomRowIndex, int32 RoomColumnIndex);

public:
	const UUserPersistData* GetUserPersistData() const;
	const URunPersistData* GetRunPersistData() const;

protected:
	void ClearRunPersistData();

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
};
