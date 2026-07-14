#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

#include "UI/FadeInOutWidget.h"
#include "UI/RDUserWidget.h"

#include "Components/AudioComponent.h"

DEFINE_LOG_CATEGORY(LogRDGameMode);

ARDGameModeBase::ARDGameModeBase()
{
}

void ARDGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	/* 공용 방 로직 */

	InitializeCommonRoom();
	for (EWorldWidgetType WorldWidgetType : mWorldWidgets)
	{
		WorldWidgetSubsystem->InitWorldWidget(WorldWidgetType);
	}

	/* 전용 방 로직 */
	
	InitializeRoom();
	WorldWidgetSubsystem->InitHUD(mHUDClass);

	/* 페이드 인 애니메이션 실행 */

	if (mShowFadeInUIOnTransition == true)
	{
		StartFadeInUIForRoomTransition();
	}

	BeginRoom();
}

void ARDGameModeBase::InitializeCommonRoom()
{
}

void ARDGameModeBase::InitializeRoom()
{
}

void ARDGameModeBase::BeginRoom()
{
}

bool ARDGameModeBase::HasActiveRun() const
{
	const URunPersistData* RunPersistData = GetRunPersistData();
	return RunPersistData != nullptr && RunPersistData->IsActive();
}

bool ARDGameModeBase::CanAbandonRun() const
{
	return HasActiveRun() == true;
}

bool ARDGameModeBase::ResetFromOptionPanel() const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->ResetOptions();

	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	checkf(SaveGameSubsystem != nullptr, TEXT("세이브 서브시스템 nullptr 오류"));
	SaveGameSubsystem->SaveOptionAsync(FAsyncSaveGameToSlotDelegate());

	return true;
}

bool ARDGameModeBase::BackFromOptionPanel() const
{
	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	checkf(SaveGameSubsystem != nullptr, TEXT("세이브 서브시스템 nullptr 오류"));
	SaveGameSubsystem->SaveOptionAsync(FAsyncSaveGameToSlotDelegate());

	return true;
}

bool ARDGameModeBase::SetMasterVolume(float Volume) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetVolume(EGameVolumeType::Master, Volume);

	return true;
}

bool ARDGameModeBase::SetBGMVolume(float Volume) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetVolume(EGameVolumeType::BGM, Volume);

	return true;
}

bool ARDGameModeBase::SetSFXVolume(float Volume) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetVolume(EGameVolumeType::SFX, Volume);

	return true;
}

bool ARDGameModeBase::SetVoiceVolume(float Volume) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetVolume(EGameVolumeType::Voice, Volume);

	return true;
}

bool ARDGameModeBase::SetUIVolume(float Volume) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetVolume(EGameVolumeType::UI, Volume);

	return true;
}

bool ARDGameModeBase::SetFpsLimit(int32 FpsLimit) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetFpsLimit(FpsLimit);

	return true;
}

bool ARDGameModeBase::SetOverallQuality(EOverallQualityType QualityType) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetOverallQuality(QualityType);

	return true;
}

bool ARDGameModeBase::SetLanguage(ELanguageType Language) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetLanguage(Language);

	return true;
}

/**
 * @brief 페이드 레이어를 열고 검은 화면에서 게임 화면으로 드러내는 페이드인을 시작한다.
 *
 * @details
 * OpenUI()는 페이드 전환 레이어를 뷰포트에 올리는 공통 UI 생명주기만 처리한다.
 * 실제로 검은 화면을 걷어 게임 화면을 보여주는 일은 UFadeInOutWidget의 StartFadeIn()이 담당한다.
 *
 * 역할 구분:
 * ARDGameModeBase는 페이드 위젯을 찾아 열고, 어떤 방향의 페이드를 시작할지만 요청한다.
 * UFadeInOutWidget은 그 결정을 받아 실제 화면을 어떻게 드러낼지와 완료 시점을 처리한다.
 */
void ARDGameModeBase::StartFadeInUI(FOnEndFadeInAnimation OnEndFadeInAnimation) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::FadeInOut) == true, TEXT("FadeInOut이 준비되지 않음"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	UFadeInOutWidget* FadeInOutWidget = WorldWidgetSubsystem->GetWorldWidget<UFadeInOutWidget>(EWorldWidgetType::FadeInOut);
	checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));

	FadeInOutWidget->OpenUI();
	FadeInOutWidget->StartFadeIn(MoveTemp(OnEndFadeInAnimation));

	if (mMainBGM != nullptr)
	{
		mBgmComponent = UGameplayStatics::SpawnSound2D(GetWorld(), mMainBGM, 1.f);
		mBgmComponent->FadeIn(mBGMFadeInDuration);
	}
}

/**
 * @brief 페이드 레이어를 열고 다음 화면 전환 전에 화면을 검게 덮는 페이드아웃을 시작한다.
 *
 * @details
 * OpenUI()는 페이드 전환 레이어를 뷰포트에 올리는 공통 UI 생명주기만 처리한다.
 * 실제로 화면을 검게 덮는 일은 UFadeInOutWidget의 StartFadeOut()이 담당한다.
 * 이 함수는 방 전환 정책을 직접 알지 않으며, 페이드아웃 완료 뒤 이어질 작업은 대리자로 받는다.
 *
 * 역할 구분:
 * ARDGameModeBase는 페이드 위젯을 찾아 열고, 어떤 방향의 페이드를 시작할지만 요청한다.
 * UFadeInOutWidget은 그 결정을 받아 실제 화면을 어떻게 덮을지와 완료 시점을 처리한다.
 */
void ARDGameModeBase::StartFadeOutUI(FOnEndFadeOutAnimation OnEndFadeOutAnimation) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::FadeInOut) == true, TEXT("FadeInOut이 준비되지 않음"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	UFadeInOutWidget* FadeInOutWidget = WorldWidgetSubsystem->GetWorldWidget<UFadeInOutWidget>(EWorldWidgetType::FadeInOut);
	checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));

	auto TryToExecuteFadeOutCallback = [Callback = MoveTemp(OnEndFadeOutAnimation), FadeInOutWidget](const ARDGameModeBase* GameMode) {
		if (EnumHasAllFlags(GameMode->mFadeOutStateFlag, ERDFadeOutStateFlag::ReadyToCallback) == false)
		{
			return;
		}
		GameMode->mFadeOutStateFlag = ERDFadeOutStateFlag::None;
		Callback.ExecuteIfBound(FadeInOutWidget);
		};

	FadeInOutWidget->OpenUI();
	FadeInOutWidget->StartFadeOut(FOnEndFadeOutAnimation::CreateWeakLambda(this, [this, TryToExecuteFadeOutCallback](UFadeInOutWidget* Widget) {
		EnumAddFlags(mFadeOutStateFlag, ERDFadeOutStateFlag::FadeAnimationEnded);
		TryToExecuteFadeOutCallback(this);
		}));

	if (mBgmComponent != nullptr && mBgmComponent->IsPlaying() == true)
	{
		mBgmComponent->OnAudioFinishedNative.AddWeakLambda(this, [this, TryToExecuteFadeOutCallback](UAudioComponent* AudioComponent) {
			EnumAddFlags(mFadeOutStateFlag, ERDFadeOutStateFlag::FadeBGMEnded);
			TryToExecuteFadeOutCallback(this);
			});
		mBgmComponent->FadeOut(mBGMFadeOutDuration, 0.f);
	}
	else
	{
		EnumAddFlags(mFadeOutStateFlag, ERDFadeOutStateFlag::FadeBGMEnded);
		TryToExecuteFadeOutCallback(this);
	}
}

/**
 * @brief 방 진입 직후 검은 전환 레이어를 걷는다.
 *
 * @details
 * 이전 방에서 이미 화면을 검게 덮은 뒤 새 방으로 들어오므로, 새 방 BeginPlay에서는 페이드인을 시작해
 * 게임 화면을 다시 보여준다.
 * 이 흐름은 방 전환 완료 후의 특수 정리 단계이므로, 페이드인이 끝나면 전환 레이어를 CloseUI()로 닫는다.
 */
void ARDGameModeBase::StartFadeInUIForRoomTransition()
{
	StartFadeInUI(FOnEndFadeInAnimation::CreateWeakLambda(this, [](UFadeInOutWidget* FadeInOutWidget)
	{
		checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));
		FadeInOutWidget->CloseUI();
	}));
}

/**
 * @brief 방 전환 전 화면을 검게 덮고 전환 시스템에 외부 준비 완료를 알린다.
 *
 * @details
 * RequireExternalReady 전환에서는 에셋 로드가 끝나도 외부 준비 신호가 오기 전까지 실제 전환을 시작하지 않는다.
 * 페이드아웃이 끝난 시점에 MarkExternalReadyForTransition()을 호출해 전환 시작 조건을 해제한다.
 * StartFadeOutUI()에 직접 이 처리를 넣지 않는 이유는, 그 함수가 파생 GameMode에서도 사용할 수 있는
 * 범용 페이드아웃 진입점이기 때문이다.
 * 일반 방 이동, 새 스테이지 첫 방 이동, 프론트엔드 방 이동은 모두 페이드아웃이 끝난 뒤
 * 같은 방식으로 ExternalReady를 전달해야 한다.
 * 각 호출부가 같은 대리자 생성과 checkf 처리를 반복하면 전환 후속 작업이 바뀔 때 세 곳을 함께 고쳐야 하므로,
 * 이 함수가 전환 전용 콜백을 한 번만 구성한다.
 */
void ARDGameModeBase::StartFadeOutUIForRoomTransition()
{
	StartFadeOutUI(FOnEndFadeOutAnimation::CreateWeakLambda(this, [this](UFadeInOutWidget*)
	{
		if (mWaitExternalWorkOnTransition == false)
		{
			const bool bExternalReadyMarked = MarkExternalReadyForTransition();
			checkf(bExternalReadyMarked == true, TEXT("외부 준비 상태 전달 오류"));
		}
	}));
}

/**
 * @brief 로딩 알림을 표시하고 OpenUI 완료 콜백을 전달한다.
 *
 * @details
 * 로딩 알림의 최소 표시 시간이나 문구 연출은 위젯이 관리한다.
 * GameMode는 표시 요청과 완료 콜백만 연결해 방 전환 로직이 UI 구현에 의존하지 않게 한다.
 *
 * 왜 GameMode가 세부 연출을 모르는가:
 * 로딩 문구, 최소 표시 시간, 닫힘 연출은 화면 표현의 책임이고 방 전환은 게임 흐름의 책임이다.
 * 둘을 섞지 않아야 로딩 UI 디자인을 바꿔도 전환 로직을 다시 고치지 않는다.
 */
void ARDGameModeBase::OpenLoadingNotifyUI(FOnEndUIOpenAnimation OnEndUIOpenAnimation) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::LoadingNotify) == true, TEXT("LoadingNotify가 준비되지 않음"));

	/* 로딩 보여주기 */

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	URDUserWidget* LoadingNotifyWidget = WorldWidgetSubsystem->GetWorldWidget<URDUserWidget>(EWorldWidgetType::LoadingNotify);
	checkf(LoadingNotifyWidget != nullptr, TEXT("로딩 위젯 nullptr 오류"));

	LoadingNotifyWidget->OpenUI(MoveTemp(OnEndUIOpenAnimation));
}

/**
 * @brief 로딩 알림을 닫고 CloseUI 완료 콜백을 전달한다.
 *
 * @details
 * CloseUI()는 즉시 닫힐 수도 있고, 위젯 내부의 완료 표시/닫힘 연출 이후 끝날 수도 있다.
 * 전환 시작 같은 후속 작업은 이 콜백 뒤에 두어 사용자에게 보이는 로딩 흐름과 실제 레벨 전환 순서를 맞춘다.
 *
 * 왜 닫힘 콜백 뒤로 미루는가:
 * 로딩 UI가 아직 "완료"를 보여주는 중인데 레벨 이동을 시작하면 입력과 화면 상태가 끊겨 보인다.
 * CloseUI() 완료 뒤에 후속 작업을 두면 UI가 자기 흐름을 끝낸 다음 전환이 이어진다.
 */
void ARDGameModeBase::CloseLoadingNotifyUI(FOnEndUICloseAnimation OnEndUICloseAnimation) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::LoadingNotify) == true, TEXT("LoadingNotify가 준비되지 않음"));

	/* 로딩 보여주기 */

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	URDUserWidget* LoadingNotifyWidget = WorldWidgetSubsystem->GetWorldWidget<URDUserWidget>(EWorldWidgetType::LoadingNotify);
	checkf(LoadingNotifyWidget != nullptr, TEXT("로딩 위젯 nullptr 오류"));

	LoadingNotifyWidget->CloseUI(MoveTemp(OnEndUICloseAnimation));
}

bool ARDGameModeBase::PreloadAndTransitionRoomAsync(int32 RoomRowIndex, int32 RoomColumnIndex)
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("다음 방에 대한 Preload 중복 요청"));
		return false;
	}

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));

	mWasNextRoomPreloadRequested = true;
	UE_LOG(LogRDGameMode, Log, TEXT("다음 방 Preload 중"));

	const bool RequireExternalReady = mShowFadeOutUIOnTransition || mWaitExternalWorkOnTransition;
	const bool AutoTransition = mShowLoadingNotifyUIOnTransition == false;
	const bool bPreloadStarted = RoomTransitionSubsystem->PreloadRoomAsync(
		RoomRowIndex,
		RoomColumnIndex,
		FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
		FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
		RequireExternalReady,
		AutoTransition
	);
	checkf(bPreloadStarted == true, TEXT("다음 방 Preload 오류"));
	if (bPreloadStarted == false)
	{
		mWasNextRoomPreloadRequested = false;
		return false;
	}

	if (RequireExternalReady == true)
	{
		if (mShowFadeOutUIOnTransition == true)
		{
			/* 페이드 아웃 애니메이션 실행 */

			/*
			 * 호출부마다 "페이드아웃이 끝나면 MarkExternalReadyForTransition()" 콜백을 반복해서 만들지 않는다.
			 * 모든 방 전환 경로가 같은 후속 작업을 하므로, 그 콜백은 전환 전용 함수에 묶었다.
			 * 여기서 말하는 같은 후속 작업은 "페이드아웃 완료 -> 로딩 알림 표시 -> ExternalReady 전달 ->
			 * 준비 완료 콜백에서 로딩 알림 종료 -> TransitLoadedRoom()" 순서다.
			 * 일반 방, 새 스테이지 첫 방, 프론트엔드 방 전환 모두 이 순서를 공유한다.
			 * 여기서는 "방 전환용 페이드아웃을 시작한다"는 요청만 남긴다.
			 */
			StartFadeOutUIForRoomTransition();
		}
	}
	else if (mShowLoadingNotifyUIOnTransition == true)
	{
		OpenLoadingNotifyUI();
	}
	return true;
}

bool ARDGameModeBase::PreloadAndTransitionRoomAsync(EStageLevelType StageLevel)
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("다음 방에 대한 Preload 중복 요청"));
		return false;
	}

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));

	UE_LOG(LogRDGameMode, Log, TEXT("스테이지 및 첫번쨰 방 Preload 중"));
	mWasNextRoomPreloadRequested = true;

	const bool RequireExternalReady = mShowFadeOutUIOnTransition || mWaitExternalWorkOnTransition;
	const bool AutoTransition = mShowLoadingNotifyUIOnTransition == false;
	const bool bPreloadStarted = RoomTransitionSubsystem->MakeStageAndPreloadRoomAsync(
		StageLevel,
		FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
		FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
		RequireExternalReady,
		AutoTransition
	);
	checkf(bPreloadStarted == true, TEXT("스테이지 및 첫번쨰 방 Preload 오류"));
	if (bPreloadStarted == false)
	{
		mWasNextRoomPreloadRequested = false;
		return false;
	}

	if (RequireExternalReady == true)
	{
		if (mShowFadeOutUIOnTransition == true)
		{
			/* 페이드 아웃 애니메이션 실행 */

			/*
			 * 호출부마다 같은 페이드아웃 완료 콜백을 만들지 않도록, 전환 전용 함수가 후속 작업을 묶는다.
			 * 여기서는 "방 전환용 페이드아웃을 시작한다"는 요청만 남긴다.
			 */
			StartFadeOutUIForRoomTransition();
		}
	}
	else if (mShowLoadingNotifyUIOnTransition == true)
	{
		OpenLoadingNotifyUI();
	}
	return true;
}

bool ARDGameModeBase::PreloadAndTransitionFrontendRoomAsync()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("다음 방에 대한 Preload 중복 요청"));
		return false;
	}

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));

	UE_LOG(LogRDGameMode, Log, TEXT("타이틀 방 Preload 중"));
	mWasNextRoomPreloadRequested = true;

	const bool RequireExternalReady = mShowFadeOutUIOnTransition || mWaitExternalWorkOnTransition;
	const bool AutoTransition = mShowLoadingNotifyUIOnTransition == false;
	const bool bPreloadStarted = RoomTransitionSubsystem->PreloadFrontendRoomAsync(
		FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
		FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
		RequireExternalReady,
		AutoTransition
	);
	checkf(bPreloadStarted == true, TEXT("타이틀 방 Preload 오류"));
	if (bPreloadStarted == false)
	{
		mWasNextRoomPreloadRequested = false;
		return false;
	}

	if (RequireExternalReady == true)
	{
		if (mShowFadeOutUIOnTransition == true)
		{
			/* 페이드 아웃 애니메이션 실행 */

			/*
			 * 호출부마다 같은 페이드아웃 완료 콜백을 만들지 않도록, 전환 전용 함수가 후속 작업을 묶는다.
			 * 여기서는 "방 전환용 페이드아웃을 시작한다"는 요청만 남긴다.
			 */
			StartFadeOutUIForRoomTransition();
		}
	}
	else if (mShowLoadingNotifyUIOnTransition == true)
	{
		OpenLoadingNotifyUI();
	}
	return true;
}

bool ARDGameModeBase::MarkExternalReadyForTransition()
{
	if (mWasNextRoomPreloadRequested == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("다음 방에 대한 Preload 요청이 우선적으로 필요"));
		return false;
	}

	const bool RequireExternalReady = mShowFadeOutUIOnTransition || mWaitExternalWorkOnTransition;
	if (RequireExternalReady == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("전환 시스템의 옵션 상 외부 준비 상태를 대기하지 않음"));
		return false;
	}

	OpenLoadingNotifyUI();

	/* 외부 준비 상태 전달 */

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));
	
	const bool bExternalReadyMarked = RoomTransitionSubsystem->MarkExternalReady();
	checkf(bExternalReadyMarked == true, TEXT("로드된 방으로 전환 실패"));
	if (bExternalReadyMarked == false)
	{
		return false;
	}

	UE_LOG(LogRDGameMode, Log, TEXT("외부 준비 상태 전달 완료"));
	return true;
}

/**
 * @brief 프리로드 완료 알림을 받은 뒤 로딩 UI의 닫힘 완료를 기다린다.
 *
 * @details
 * 자동 전환을 쓰지 않는 경우 RoomTransitionSubsystem은 준비 완료 콜백까지만 호출한다.
 * 로딩 UI가 켜져 있다면 CloseUI() 완료 콜백에서 TransitLoadedRoom()을 호출해, 화면 표시가 먼저 정리된 뒤
 * 실제 레벨 이동이 시작되도록 한다.
 *
 * 왜 바로 TransitLoadedRoom()을 호출하지 않는가:
 * 준비 완료와 사용자에게 완료가 보이는 시점은 다를 수 있다. 사용자는 UI가 닫히는 흐름까지 보고 나서야
 * 전환이 자연스럽다고 느끼므로, 실제 레벨 이동은 CloseUI() 콜백 뒤로 둔다.
 */
void ARDGameModeBase::OnReadyToTransition(int32 RoomRowIndex, int32 RoomColumnIndex)
{
	if (mShowLoadingNotifyUIOnTransition == false)
	{
		return;
	}

	CloseLoadingNotifyUI(FOnEndUICloseAnimation::CreateWeakLambda(this, [this](UUserWidget*)
	{
		URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
		checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));

		const bool bTransitionStarted = RoomTransitionSubsystem->TransitLoadedRoom();
		checkf(bTransitionStarted == true, TEXT("방 전환 시작 오류"));
	}));
}

void ARDGameModeBase::OnPreTransition(int32 RoomRowIndex, int32 RoomColumnIndex)
{
}

const UUserPersistData* ARDGameModeBase::GetUserPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetUserPersistData();
}

UUserPersistData* ARDGameModeBase::GetUserPersistData()
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetUserPersistData();
}

URunPersistData* ARDGameModeBase::GetRunPersistData()
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetRunPersistData();
}

const URunPersistData* ARDGameModeBase::GetRunPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetRunPersistData();
}

void ARDGameModeBase::ClearRunPersistData()
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));

	GameProfileSubsystem->EndRun();
}

USoundBase* ARDGameModeBase::GetMainBGM() const
{
	return mMainBGM;
}

void ARDGameModeBase::SetMainBGM(USoundBase* BGM, bool IsOverride)
{
	if (mMainBGM == nullptr || IsOverride == true)
	{
		mMainBGM = BGM;
	}
}

