#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

#include "UI/RDUserWidget.h"

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

/**
 * @brief 방 진입 직후 페이드아웃 상태의 전환 레이어를 다시 닫는다.
 *
 * @details
 * 페이드 위젯은 OpenUI()에서 검은 화면으로 덮고, CloseUI()에서 다시 화면을 보여준다.
 * 현재 방에 들어온 뒤에는 전환을 기다릴 필요가 없으므로 OpenUI 완료 콜백에서 바로 CloseUI()를 호출한다.
 *
 * 왜 이렇게 하는가:
 * 이전 방에서 이미 검은 화면까지 덮은 뒤 새 방으로 들어오므로, 새 방이 준비된 다음에는 그 덮개를 걷어야 한다.
 * GameMode가 직접 Visibility를 만지지 않고 CloseUI()로 맡기면 페이드 연출과 닫힘 완료 처리가 같은 규칙을 따른다.
 *
 * 역할 구분:
 * ARDGameModeBase는 방 생명주기와 전환 타이밍을 알고 있으므로 "지금 페이드 덮개를 닫아야 한다"는 결정을 내린다.
 * UFadeInOutWidget은 그 결정을 받아 실제 화면을 어떻게 드러낼지와 애니메이션 완료 시점을 처리한다.
 */
void ARDGameModeBase::StartFadeInUI() const
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::FadeInOut) == true, TEXT("FadeInOut이 준비되지 않음"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	URDUserWidget* FadeInOutWidget = WorldWidgetSubsystem->GetWorldWidget<URDUserWidget>(EWorldWidgetType::FadeInOut);
	checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));

	FadeInOutWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(this, [FadeInOutWidget](UUserWidget*)
	{
		FadeInOutWidget->CloseUI();
	}));
}

/**
 * @brief 다음 방으로 넘어가기 전 화면을 먼저 검게 덮는다.
 *
 * @details
 * RequireExternalReady 전환에서는 에셋 로드가 끝나도 외부 준비 신호가 오기 전까지 실제 전환을 시작하지 않는다.
 * 이 함수는 페이드아웃이 끝난 시점에 MarkExternalReadyForTransition()을 호출해 전환 시작 조건을 해제한다.
 *
 * 왜 화면을 먼저 덮는가:
 * 레벨 전환이 먼저 시작되면 이전 방의 마지막 프레임이나 아직 덜 준비된 새 방이 잠깐 보일 수 있다.
 * 그래서 검은 화면이 완전히 올라온 뒤에만 외부 준비 완료를 알려, 사용자가 보는 화면과 실제 전환 순서를 맞춘다.
 *
 * 역할 구분:
 * ARDGameModeBase는 로드/전환 시스템과 연결되어 있으므로 페이드 완료 후 MarkExternalReadyForTransition()을 호출한다.
 * UFadeInOutWidget은 GameMode의 전환 규칙을 모르고, OpenUI()가 요청된 페이드아웃 연출과 완료 콜백만 책임진다.
 */
void ARDGameModeBase::StartFadeOutUI()
{
	checkf(mWorldWidgets.Contains(EWorldWidgetType::FadeInOut) == true, TEXT("FadeInOut이 준비되지 않음"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	URDUserWidget* FadeInOutWidget = WorldWidgetSubsystem->GetWorldWidget<URDUserWidget>(EWorldWidgetType::FadeInOut);
	checkf(FadeInOutWidget != nullptr, TEXT("페이드 인 앤 아웃 위젯 nullptr 오류"));

	FadeInOutWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(this, [this](UUserWidget*)
	{
		checkf(MarkExternalReadyForTransition() == true, TEXT("외부 준비 상태 전달 오류"));
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
	checkf(
		RoomTransitionSubsystem->PreloadRoomAsync(
			RoomRowIndex, 
			RoomColumnIndex,
			FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
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

			/* FadeOut 완료 콜백은 StartFadeOutUI() 내부에서 MarkExternalReadyForTransition()까지 이어준다. */
			StartFadeOutUI();
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
			FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
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

			/* FadeOut 완료 콜백은 StartFadeOutUI() 내부에서 MarkExternalReadyForTransition()까지 이어준다. */
			StartFadeOutUI();
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
			FOnReadyToTransition::CreateUObject(this, &ARDGameModeBase::OnReadyToTransition),
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

			/* FadeOut 완료 콜백은 StartFadeOutUI() 내부에서 MarkExternalReadyForTransition()까지 이어준다. */
			StartFadeOutUI();
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

		checkf(RoomTransitionSubsystem->TransitLoadedRoom(), TEXT("방 전환 시작 오류"));
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
