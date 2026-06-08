#include "GameMode/IntroGameMode.h"

#include <type_traits>

#include "Blueprint/UserWidget.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/CinematicWidget.h"

namespace
{
	template<typename T>
	auto TryPreloadFrontendRoomAsync(T* RoomTransitionSubsystem, int)
		-> decltype(RoomTransitionSubsystem->PreloadFrontendRoomAsync(), bool())
	{
		using TResult = decltype(RoomTransitionSubsystem->PreloadFrontendRoomAsync());
		if constexpr (std::is_same_v<TResult, bool>)
		{
			return RoomTransitionSubsystem->PreloadFrontendRoomAsync();
		}
		else
		{
			RoomTransitionSubsystem->PreloadFrontendRoomAsync();
			return true;
		}
	}

	bool TryPreloadFrontendRoomAsync(URoomTransitionSubsystem* RoomTransitionSubsystem, ...)
	{
		RoomTransitionSubsystem->PreloadTitleRoomAsync();
		return true;
	}
}

void AIntroGameMode::BeginRoom()
{
	Super::BeginRoom();

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("룸 전환 서브시스템 nullptr"));
	checkf(TryPreloadFrontendRoomAsync(RoomTransitionSubsystem, 0), TEXT("Intro -> Frontend preload 실패"));

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
			TransitionLoadedFrontendRoomAsync();
		}));
	}));
}

void AIntroGameMode::TransitionLoadedFrontendRoomAsync() const
{
	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	checkf(RoomTransitionSubsystem != nullptr, TEXT("룸 전환 서브시스템 nullptr"));
	RoomTransitionSubsystem->TransitLoadedRoomAsync();
}
