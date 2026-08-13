#include "GameMode/RDGameModeBase.h"
#include "Engine/GameInstance.h"
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

bool ARDGameModeBase::SetCameraShakeEnabled(bool IsEnabled) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetCameraShakeEnabled(IsEnabled);

	return true;
}

bool ARDGameModeBase::SetEffectVFXEnabled(bool IsEnabled) const
{
	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로필 서브시스템 nullptr 오류"));
	GameProfileSubsystem->SetEffectVFXEnabled(IsEnabled);

	return true;
}

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

void ARDGameModeBase::StartFadeInUIForRoomTransition()
{
	StartFadeInUI(FOnEndFadeInAnimation::CreateWeakLambda(this, [](UFadeInOutWidget* FadeInOutWidget)
	{
		checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));
		FadeInOutWidget->CloseUI();
	}));
}

void ARDGameModeBase::StartFadeOutUIForRoomTransition()
{
	StartFadeOutUI(FOnEndFadeOutAnimation::CreateWeakLambda(this, [this](UFadeInOutWidget*)
	{
		if (mWaitExternalWorkOnTransition == false)
		{
			const bool IsExternalReadyMarked = MarkExternalReadyForTransition();
			checkf(IsExternalReadyMarked == true, TEXT("외부 준비 상태 전달 오류"));
		}
	}));
}

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
	const bool IsPreloadStarted = RoomTransitionSubsystem->PreloadRoomAsync(
		RoomRowIndex,
		RoomColumnIndex,
		FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
		FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
		RequireExternalReady,
		AutoTransition
	);
	checkf(IsPreloadStarted == true, TEXT("다음 방 Preload 오류"));
	if (IsPreloadStarted == false)
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
	const bool IsPreloadStarted = RoomTransitionSubsystem->MakeStageAndPreloadRoomAsync(
		StageLevel,
		FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
		FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
		RequireExternalReady,
		AutoTransition
	);
	checkf(IsPreloadStarted == true, TEXT("스테이지 및 첫번쨰 방 Preload 오류"));
	if (IsPreloadStarted == false)
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
	const bool IsPreloadStarted = RoomTransitionSubsystem->PreloadFrontendRoomAsync(
		FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
		FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
		RequireExternalReady,
		AutoTransition
	);
	checkf(IsPreloadStarted == true, TEXT("타이틀 방 Preload 오류"));
	if (IsPreloadStarted == false)
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
	
	const bool IsExternalReadyMarked = RoomTransitionSubsystem->MarkExternalReady();
	checkf(IsExternalReadyMarked == true, TEXT("로드된 방으로 전환 실패"));
	if (IsExternalReadyMarked == false)
	{
		return false;
	}

	UE_LOG(LogRDGameMode, Log, TEXT("외부 준비 상태 전달 완료"));
	return true;
}

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

		const bool IsTransitionStarted = RoomTransitionSubsystem->TransitLoadedRoom();
		checkf(IsTransitionStarted == true, TEXT("방 전환 시작 오류"));
	}));
}

void ARDGameModeBase::OnPreTransition(int32 RoomRowIndex, int32 RoomColumnIndex)
{
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

UOptionPersistData* ARDGameModeBase::GetOptionPersistData()
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetOptionPersistData();
}

const UUserPersistData* ARDGameModeBase::GetUserPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetUserPersistData();
}

const URunPersistData* ARDGameModeBase::GetRunPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetRunPersistData();
}

const UOptionPersistData* ARDGameModeBase::GetOptionPersistData() const
{
	UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->GetOptionPersistData();
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

void ARDGameModeBase::FadeOutMainBGM(const float FadeOutDurationSeconds) const
{
	if (IsValid(mBgmComponent.Get()) == false || mBgmComponent->IsPlaying() == false)
	{
		return;
	}

	if (FadeOutDurationSeconds <= KINDA_SMALL_NUMBER)
	{
		mBgmComponent->Stop();
		return;
	}

	mBgmComponent->FadeOut(FadeOutDurationSeconds, 0.f);
}

void ARDGameModeBase::SetMainBGM(USoundBase* BGM, bool IsOverride)
{
	if (mMainBGM == nullptr || IsOverride == true)
	{
		mMainBGM = BGM;
	}
}

