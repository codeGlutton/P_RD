#include "GameMode/IntroGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/CinematicWidget.h"

void AIntroGameMode::BeginRoom()
{
	Super::BeginRoom();

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("룸 전환 서브시스템 nullptr"));
	//checkf(RoomTransitionSubsystem->PreloadFrontendRoomAsync(), TEXT("Intro -> Frontend preload 실패"));

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UCinematicWidget* CinematicHUD = WorldWidgetSubsystem->GetHUD<UCinematicWidget>();
	checkf(CinematicHUD != nullptr, TEXT("인트로에 보여줄 Cinematic 위젯 nullptr"));

	CinematicHUD->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(this, [this](UUserWidget* OpenedWidget)
	{
		UCinematicWidget* OpenedCinematicWidget = CastChecked<UCinematicWidget>(OpenedWidget);
		OpenedCinematicWidget->PlayCinematic(FOnEndCinematicAnimation::CreateWeakLambda(this, [this](UCinematicWidget* CinematicWidget)
		{
			(void)CinematicWidget;
			URoomTransitionSubsystem* TransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
			checkf(TransitionSubsystem != nullptr, TEXT("룸 전환 서브시스템 nullptr"));
			TransitionSubsystem->TransitLoadedRoomAsync();
		}));
	}));
}
