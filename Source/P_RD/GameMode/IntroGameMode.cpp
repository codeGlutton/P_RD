#include "GameMode/IntroGameMode.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"

#include "UI/CinematicWidget.h"

AIntroGameMode::AIntroGameMode()
{
	mWorldWidgets = { EWorldWidgetType::LoadingNotify, EWorldWidgetType::FadeInOut };

	mShowFadeInUIOnTransition = false;
	mShowFadeOutUIOnTransition = false;
	mShowLoadingNotifyUIOnTransition = true;
	mWaitExternalWorkOnTransition = true;
}

void AIntroGameMode::BeginRoom()
{
	Super::BeginRoom();

	const bool IsPreloadStarted = PreloadAndTransitionFrontendRoomAsync();
	checkf(IsPreloadStarted == true, TEXT("Intro -> Frontend 과정에서 Preload 실패"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UCinematicWidget* CinematicHUD = WorldWidgetSubsystem->GetHUD<UCinematicWidget>();
	checkf(CinematicHUD != nullptr, TEXT("인트로에 보여줄 Cinematic 위젯 nullptr"));

	// UI 열리는 애니메이션 시작
	CinematicHUD->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(this, [this](UUserWidget* OpenedWidget) {
		// 시네마틱 시작
		UCinematicWidget* OpenedCinematicWidget = Cast<UCinematicWidget>(OpenedWidget);
		checkf(OpenedCinematicWidget != nullptr, TEXT("시네마틱 위젯 nullptr"));
		OpenedCinematicWidget->PlayCinematic(FOnEndCinematicAnimation::CreateUObject(this, &AIntroGameMode::OnEndCinematicAnimation));

		// 파일 비동기 로드 시작
		USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
		checkf(SaveGameSubsystem != nullptr, TEXT("게임 저장 및 로드 서브시스템 nullptr"));
		SaveGameSubsystem->LoadUserAsync(FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AIntroGameMode::OnLoadUserData));
		SaveGameSubsystem->LoadRunAsync(FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AIntroGameMode::OnLoadRunData));
		SaveGameSubsystem->LoadOptionAsync(FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &AIntroGameMode::OnLoadOptionData));
		}));
}

void AIntroGameMode::OnEndCinematicAnimation(UCinematicWidget* CinematicWidget)
{
	StartFadeOutUI(FOnEndFadeOutAnimation::CreateWeakLambda(this, [this](UFadeInOutWidget* Widget) {
		EnumAddFlags(mStateFlag, EIntroGameModeStateFlag::CinematicAnimationEnded);
		TryToMarkExternalReady();
		}));
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

void AIntroGameMode::OnLoadOptionData(const FString& SlotName, int32 SlotIndex, USaveGame* SaveGame)
{
	EnumAddFlags(mStateFlag, EIntroGameModeStateFlag::OptionDataLoaded);
	TryToMarkExternalReady();
}

void AIntroGameMode::TryToMarkExternalReady()
{
	if (EnumHasAllFlags(mStateFlag, EIntroGameModeStateFlag::ReadyToTransition) == false)
	{
		return;
	}
	mStateFlag = EIntroGameModeStateFlag::None;

	// <시네마틱 && 런 데이터 로드 && 유저 데이터 로드>에 대한 대기 상태 모두 완료 알림
	const bool IsExternalReadyMarked = MarkExternalReadyForTransition();
	checkf(IsExternalReadyMarked == true, TEXT("대기 상태 모두 완료 알림 실패"));
}

