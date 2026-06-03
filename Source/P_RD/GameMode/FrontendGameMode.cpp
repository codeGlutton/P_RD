#include "GameMode/FrontendGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Setting/GamePlaySettings.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

namespace
{
	/**
	 * @brief 타이틀 맵에서 UI 배경으로 사용할 카메라 태그
	 *
	 * @details
	 * L_Title 맵에 이 태그가 붙은 CameraActor를 두면 FrontendGameMode가 BeginRoom()에서
	 * 플레이어 컨트롤러의 ViewTarget을 해당 카메라로 바꾼다.
	 * 타이틀 맵은 플레이어가 움직이는 방이 아니므로, 전투용 폰 카메라 대신 고정 카메라를 쓰기 위한 약속이다.
	 */
	const FName TitleCameraTag = TEXT("TitleCamera");
}

AFrontendGameMode::AFrontendGameMode()
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	if (GamePlaySettings != nullptr && GamePlaySettings->mFrontendHUDClass.IsNull() == false)
	{
		mHUDClass = GamePlaySettings->mFrontendHUDClass.LoadSynchronous();
	}

	if (mHUDClass == nullptr)
	{
		mHUDClass = UTitleMenuWidget::StaticClass();
	}

	DefaultPawnClass = nullptr;
	mWorldWidgets.Empty();
}

void AFrontendGameMode::InitializeCommonRoom()
{
}

void AFrontendGameMode::BeginRoom()
{
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UUserWidget* TitleHUD = WorldWidgetSubsystem->GetHUD())
		{
			TitleHUD->AddToViewport();
			TitleHUD->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		TArray<AActor*> TitleCameraActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), TitleCameraTag, OUT TitleCameraActors);
		if (TitleCameraActors.IsEmpty() == false)
		{
			PlayerController->SetViewTarget(TitleCameraActors[0]);
		}

		PlayerController->ActivateTouchInterface(nullptr);
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}
