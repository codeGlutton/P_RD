#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

#include "UI/ToggleableWidget.h"

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

	/* 페이드 인 애니메이션 실행 */

	if (mShowFadeInUIOnTransition == true)
	{
		StartFadeInUI();
	}

	/* 전용 방 로직 */
	
	InitializeRoom();
	WorldWidgetSubsystem->InitHUD(mHUDClass);

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

void ARDGameModeBase::StartFadeInUI(/* FOnEndFadeInAnimation OnEndUIOpenAnimation*/) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::FadeInOut) == true, TEXT("FadeInOut이 준비되지 않음"));

	/*UFadeInOutWidget* FadeInOutWidget = WorldWidgetSubsystem->GetWorldWidget<UFadeInOutWidget>(EWorldWidgetType::FadeInOut);
	checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));

	FadeInOutWidget->OpenUI();
	FadeInOutWidget->StartFadeIn(MoveTemp(OnEndUIOpenAnimation));*/
}

void ARDGameModeBase::StartFadeOutUI(/* FOnEndFadeOutAnimation OnEndFadeOutAnimation*/) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::FadeInOut) == true, TEXT("FadeInOut이 준비되지 않음"));

	/*UFadeInOutWidget* FadeInOutWidget = WorldWidgetSubsystem->GetWorldWidget<UFadeInOutWidget>(EWorldWidgetType::FadeInOut);
	checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));

	FadeInOutWidget->OpenUI();
	FadeInOutWidget->StartFadeOut(MoveTemp(OnEndFadeOutAnimation));*/
}

void ARDGameModeBase::OpenLoadingNotifyUI(/* FOnOpenUIAnimation OnOpenUIAnimation = FOnOpenUIAnimation() */) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::LoadingNotify) == true, TEXT("LoadingNotify가 준비되지 않음"));

	/* 로딩 보여주기 */

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	UToggleableWidget* LoadingNotifyWidget = WorldWidgetSubsystem->GetWorldWidget<UToggleableWidget>(EWorldWidgetType::LoadingNotify);
	checkf(LoadingNotifyWidget != nullptr, TEXT("로딩 위젯 nullptr 오류"));

	LoadingNotifyWidget->OpenUI(/*MoveTemp(OnOpenUIAnimation)*/);
}

void ARDGameModeBase::CloseLoadingNotifyUI(/* FOnEndUICloseAnimation OnEndUICloseAnimation = FOnEndUICloseAnimation() */) const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::LoadingNotify) == true, TEXT("LoadingNotify가 준비되지 않음"));

	/* 로딩 보여주기 */

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	UToggleableWidget* LoadingNotifyWidget = WorldWidgetSubsystem->GetWorldWidget<UToggleableWidget>(EWorldWidgetType::LoadingNotify);
	checkf(LoadingNotifyWidget != nullptr, TEXT("로딩 위젯 nullptr 오류"));

	LoadingNotifyWidget->CloseUI(/*MoveTemp(OnEndUICloseAnimation)*/);
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
	checkf(
		RoomTransitionSubsystem->PreloadRoomAsync(
			RoomRowIndex, 
			RoomColumnIndex,
			FOnReadyToTransition::CreateLambda(this, &ARDGameModeBase::OnReadyToTransition),
			FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
			RequireExternalReady, 
			AutoTransition
		) == true,
		TEXT("다음 방 Preload 오류")
	);

	if (RequireExternalReady == true)
	{
		if (mShowFadeOutUIOnTransition == true)
		{
			/* 페이드 아웃 애니메이션 실행 */

			StartFadeOutUI(/*FOnEndFadeOutAnimation::CreateWeakLambda(this, [this](UFadeInOutWidget* FadeInOutWidget) {
				checkf(MarkExternalReadyForTransition() == true, TEXT("외부 준비 상태 전달 오류"));*/
			);
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
	checkf(
		RoomTransitionSubsystem->MakeStageAndPreloadRoomAsync(
			StageLevel,
			FOnReadyToTransition::CreateLambda(this, &ARDGameModeBase::OnReadyToTransition),
			FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
			RequireExternalReady,
			AutoTransition
		) == true,
		TEXT("스테이지 및 첫번쨰 방 Preload 오류")
	);

	if (RequireExternalReady == true)
	{
		if (mShowFadeOutUIOnTransition == true)
		{
			/* 페이드 아웃 애니메이션 실행 */

			StartFadeOutUI(/*FOnEndFadeOutAnimation::CreateWeakLambda(this, [this](UFadeInOutWidget* FadeInOutWidget) {
				checkf(MarkExternalReadyForTransition() == true, TEXT("외부 준비 상태 전달 오류"));*/
			);
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
	checkf(
		RoomTransitionSubsystem->PreloadFrontendRoomAsync(
			FOnReadyToTransition::CreateLambda(this, &ARDGameModeBase::OnReadyToTransition),
			FOnPreTransitNextRoom::CreateUObject(this, &ARDGameModeBase::OnPreTransition),
			RequireExternalReady,
			AutoTransition
		) == true,
		TEXT("타이틀 방 Preload 오류")
	);

	if (RequireExternalReady == true)
	{
		if (mShowFadeOutUIOnTransition == true)
		{
			/* 페이드 아웃 애니메이션 실행 */

			StartFadeOutUI(/*FOnEndFadeOutAnimation::CreateWeakLambda(this, [this](UFadeInOutWidget* FadeInOutWidget) {
				checkf(MarkExternalReadyForTransition() == true, TEXT("외부 준비 상태 전달 오류"));*/
			);
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
	
	checkf(RoomTransitionSubsystem->MarkExternalReady() == true, TEXT("로드된 방으로 전환 실패"));

	UE_LOG(LogRDGameMode, Log, TEXT("외부 준비 상태 전달 완료"));
	return true;
}

void ARDGameModeBase::OnReadyToTransition(int32 RoomRowIndex, int32 RoomColumnIndex)
{
	if (mShowLoadingNotifyUIOnTransition == false)
	{
		return;
	}

	CloseLoadingNotifyUI(/* FOnEndUICloseAnimation::CreateWeakLambda(this, [this]() {

		URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
		checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));

		// 로딩 위젯이 닫힐 때 Transition 전환 시작
		checkf(RoomTransitionSubsystem->TransitLoadedRoom(), TEXT("방 전환 시작 오류"));

		}) */);
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