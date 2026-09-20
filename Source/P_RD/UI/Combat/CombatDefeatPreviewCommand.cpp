/*****************************************************************//**
 * @file   CombatDefeatPreviewCommand.cpp
 * @brief  저장 데이터에 손대지 않고 패배 결과 WBP를 띄우는 개발 명령.
 *********************************************************************/

#include "RDMinimal.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "TimerManager.h"
#include "GameMode/FrontendGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UI/CombatResultOverlayWidget.h"

#if !UE_BUILD_SHIPPING

namespace CombatDefeatPreview
{
	constexpr TCHAR WidgetPath[] =
		TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C");

	TWeakObjectPtr<UCombatResultOverlayWidget> ShownWidget;

	void CloseShownWidget()
	{
		if (ShownWidget.IsValid())
		{
			ShownWidget->CloseUI();
			ShownWidget.Reset();
		}
	}

	void Show(UWorld* World)
	{
		CloseShownWidget();
		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.DefeatPreview: 게임 월드를 찾지 못했습니다."));
			return;
		}

		UClass* WidgetClass = LoadClass<UCombatResultOverlayWidget>(nullptr, WidgetPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.DefeatPreview: %s 클래스를 찾지 못했습니다."), WidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		UCombatResultOverlayWidget* Widget = Controller != nullptr
			? CreateWidget<UCombatResultOverlayWidget>(Controller, WidgetClass)
			: CreateWidget<UCombatResultOverlayWidget>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.DefeatPreview: 위젯 생성에 실패했습니다."));
			return;
		}

		FCombatResultUI Result;
		Result.mLocationName = NSLOCTEXT("CombatDefeatPreview", "Location", "잊힌 성채");
		Result.mRound = 7;
		Result.mDefeatedMonsterCount = 12;
		Result.mGoldGained = 0;
		Result.mExpGained = 0;

		Widget->ShowDefeatResult(Result,
			FSimpleDelegate::CreateStatic(&CloseShownWidget));
		Widget->OpenUI();
		ShownWidget = Widget;
		UE_LOG(LogRD, Display, TEXT("RD.DefeatPreview: 패배 결과 WBP를 열었습니다."));
	}

	// The title level requires the intro's asynchronous asset preload. This command
	// waits across that level transition instead of bypassing the normal boot path.
	void ShowAfterStartup()
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[Started = FPlatformTime::Seconds()](float)
			{
				if (FPlatformTime::Seconds() - Started > 120.0) return false;
				if (!GEngine) return true;
				for (const FWorldContext& Context : GEngine->GetWorldContexts())
				{
					UWorld* World = Context.World();
					if (!World || !World->IsGameWorld() || !Cast<AFrontendGameMode>(World->GetAuthGameMode())
						|| World->GetTimeSeconds() < 3.f) continue;
					Show(World);
					FTimerHandle CaptureTimer;
					World->GetTimerManager().SetTimer(CaptureTimer, FTimerDelegate::CreateLambda([]()
					{
						FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() /
							TEXT("UI/DefeatClassic/preview-viewport.png"), true, false);
					}), 2.f, false);
					return false;
				}
				return true;
			}), .25f);
	}

	FAutoConsoleCommand StartupCommand(
		TEXT("RD.DefeatPreview.AfterStartup"),
		TEXT("Open the defeat preview after the normal intro and title preload."),
		FConsoleCommandDelegate::CreateStatic(&ShowAfterStartup));

	FAutoConsoleCommandWithWorld ShowCommand(
		TEXT("RD.DefeatPreview"),
		TEXT("저장 데이터를 바꾸지 않고 패배 결과 WBP를 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Show));
}

#endif
