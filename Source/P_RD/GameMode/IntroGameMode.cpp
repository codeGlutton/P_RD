#include "GameMode/IntroGameMode.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

#include "UI/CinematicWidget.h"

AIntroGameMode::AIntroGameMode()
{
	mWorldWidgets = { };

	mShowFadeInUIOnTransition = false;
	mShowFadeOutUIOnTransition = false;
	mShowLoadingNotifyUIOnTransition = false;
	mWaitExternalWorkOnTransition = true;
}

void AIntroGameMode::BeginRoom()
{
	Super::BeginRoom();

	checkf(PreloadAndTransitionFrontendRoomAsync() == true, TEXT("Intro -> Frontend 과정에서 Preload 실패"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UCinematicWidget* CinematicHUD = WorldWidgetSubsystem->GetHUD<UCinematicWidget>();
	checkf(CinematicHUD != nullptr, TEXT("인트로에 보여줄 Cinematic 위젯 nullptr"));
	mCinematicHUD = CinematicHUD;
	
	// UI 열리는 애니메이션 시작
	CinematicHUD->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(this, [this](UUserWidget* OpenedWidget) {
		// 시네마틱 시작
		UCinematicWidget* OpenedCinematicWidget = Cast<UCinematicWidget>(OpenedWidget);
		checkf(OpenedCinematicWidget != nullptr, TEXT("열린 인트로 위젯 타입 오류"));
		OpenedCinematicWidget->PlayCinematic(FOnEndCinematicAnimation::CreateUObject(this, &AIntroGameMode::OnEndCinematicAnimation));
		
		// 파일 비동기 로드 시작
		USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
		checkf(SaveGameSubsystem != nullptr, TEXT("게임 저장 및 로드 서브시스템 nullptr"));
		SaveGameSubsystem->LoadUserAsync(FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AIntroGameMode::OnLoadUserData));
		SaveGameSubsystem->LoadRunAsync(FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AIntroGameMode::OnLoadRunData));
		}));
}

void AIntroGameMode::OnEndCinematicAnimation(UCinematicWidget* CinematicWidget)
{
	mCinematicHUD = CinematicWidget;

	EnumAddFlags(mStateFlag, EIntroGameModeStateFlag::CinematicAnimationEnded);
	if (EnumHasAllFlags(mStateFlag, EIntroGameModeStateFlag::ReadyToTransition) == false && CinematicWidget != nullptr)
	{
		StartLoadingWaitDelay();
	}
	TryToMarkExternalReady();
}

void AIntroGameMode::OnLoadUserData(const FString& SlotName, int32 SlotIndex, USaveGame* SaveGame)
{
	EnumAddFlags(mStateFlag, EIntroGameModeStateFlag::UserDataLoaded);
	TryToMarkExternalReady();
}

void AIntroGameMode::OnLoadRunData(const FString& SlotName, int32 SlotIndex, USaveGame* SaveGame)
{
	EnumAddFlags(mStateFlag, EIntroGameModeStateFlag::RunDataLoaded);
	TryToMarkExternalReady();
}

void AIntroGameMode::TryToMarkExternalReady()
{
	if (EnumHasAllFlags(mStateFlag, EIntroGameModeStateFlag::ReadyToTransition) == false)
	{
		return;
	}
	ClearLoadingWaitDelay();
	mStateFlag = EIntroGameModeStateFlag::None;

	MarkIntroExternalReady();
}

void AIntroGameMode::StartLoadingWaitDelay()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	ClearLoadingWaitDelay();
	World->GetTimerManager().SetTimer(
		mLoadingWaitDelayTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			ShowLoadingWaitScreenIfStillNeeded();
		}),
		mLoadingWaitDelaySeconds,
		false
	);
}

void AIntroGameMode::ClearLoadingWaitDelay()
{
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(mLoadingWaitDelayTimerHandle);
	}
	mLoadingWaitDelayTimerHandle.Invalidate();
}

void AIntroGameMode::ShowLoadingWaitScreenIfStillNeeded()
{
	if (EnumHasAllFlags(mStateFlag, EIntroGameModeStateFlag::ReadyToTransition) == true)
	{
		return;
	}

	if (UCinematicWidget* CinematicHUD = mCinematicHUD.Get())
	{
		CinematicHUD->FadeToLoadingWaitScreen();
	}
}

void AIntroGameMode::MarkIntroExternalReady()
{
	/*
	 * 인트로는 영상 자체가 로딩 화면 역할을 한다.
	 * 그래서 공통 MarkExternalReadyForTransition()을 쓰면 LoadingNotify가 한 번 더 열려
	 * "인트로 종료 -> 별도 로딩 화면"으로 보인다.
	 * 여기서는 시네마틱 위젯을 닫은 뒤 전환 서브시스템에 ExternalReady만 직접 전달한다.
	 */
	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("방 전환 서브시스템 nullptr 오류"));
	checkf(RoomTransitionSubsystem->MarkExternalReady() == true, TEXT("대기 상태 모두 완료 알림 실패"));
	StartLoadingWaitDelay();
}

