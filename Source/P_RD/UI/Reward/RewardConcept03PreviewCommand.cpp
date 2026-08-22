/*****************************************************************//**
 * @file   RewardConcept03PreviewCommand.cpp
 * @brief  실제 게임 월드에서 신규 전투 보상 WBP를 검증하는 개발 명령.
 *********************************************************************/

#include "RDMinimal.h"

#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UI/Reward/RewardConcept03Widget.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING

namespace RewardConcept03Preview
{
	constexpr TCHAR FourStepWidgetPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_Frameless.WBP_RewardConcept03_Frameless_C");
	constexpr TCHAR ThreeStepWidgetPath[] =
		TEXT("/Game/UI/RewardConcept03New/WBP_RewardConcept03_New_NoArtifact.WBP_RewardConcept03_New_NoArtifact_C");

	TWeakObjectPtr<URewardConcept03Widget> ShownWidget;
	FTSTicker::FDelegateHandle CaptureTickerHandle;
	float CaptureElapsed = 0.f;
	int32 CaptureIndex = 0;
	FString CaptureDirectory;
	constexpr float CaptureTimes[] = { 1.05f, 2.15f, 4.82f, 7.12f };
	const TCHAR* CaptureNames[] = {
		TEXT("RewardConcept03_InGame_01_ChestOpening.png"),
		TEXT("RewardConcept03_InGame_02_TripleGoldBurst.png"),
		TEXT("RewardConcept03_InGame_03_CenteredGold.png"),
		TEXT("RewardConcept03_InGame_04_ArtifactChoice.png")
	};
	FTSTicker::FDelegateHandle VideoTickerHandle;
	float VideoElapsed = 0.f;
	int32 VideoFrameIndex = 0;
	int32 VideoWarmupFramesRemaining = 0;
	bool bVideoPresentationStarted = false;
	FString VideoFrameDirectory;
	constexpr int32 VideoFrameRate = 30;
	constexpr int32 VideoFrameCount = 240;

	void RemoveShownWidget()
	{
		if (ShownWidget.IsValid())
		{
			ShownWidget->RemoveFromParent();
			ShownWidget.Reset();
		}
	}

	bool TickCaptureAndQuit(const float DeltaSeconds)
	{
		CaptureElapsed += DeltaSeconds;
		if (CaptureIndex < UE_ARRAY_COUNT(CaptureTimes)
			&& CaptureElapsed >= CaptureTimes[CaptureIndex])
		{
			const FString OutputPath = FPaths::Combine(
				CaptureDirectory, CaptureNames[CaptureIndex]);
			FScreenshotRequest::RequestScreenshot(
				OutputPath, true, false, false);
			UE_LOG(LogRD, Display,
				TEXT("RD_REWARD_CONCEPT03_INGAME_CAPTURE requested index=%d path=%s"),
				CaptureIndex, *OutputPath);
			++CaptureIndex;
		}

		if (CaptureIndex >= UE_ARRAY_COUNT(CaptureTimes)
			&& CaptureElapsed >= CaptureTimes[UE_ARRAY_COUNT(CaptureTimes) - 1] + 1.f)
		{
			const FString FinalPath = FPaths::Combine(
				CaptureDirectory, CaptureNames[UE_ARRAY_COUNT(CaptureNames) - 1]);
			UE_LOG(LogRD, Display,
				TEXT("RD_REWARD_CONCEPT03_INGAME_CAPTURE complete final=%s exists=%d"),
				*FinalPath, IFileManager::Get().FileExists(*FinalPath) ? 1 : 0);
			FPlatformMisc::RequestExit(false);
			CaptureTickerHandle.Reset();
			return false;
		}
		return true;
	}

	void StartCaptureAndQuit()
	{
		if (CaptureTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(CaptureTickerHandle);
		}
		CaptureDirectory = FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("UI"), TEXT("RewardConcept03New"),
			TEXT("InGame"));
		IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
		CaptureElapsed = 0.f;
		CaptureIndex = 0;
		CaptureTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			TEXT("RewardConcept03InGameCapture"), 0.f,
			[](const float DeltaSeconds)
			{
				return TickCaptureAndQuit(DeltaSeconds);
			});
	}

	bool TickVideoCaptureAndQuit(const float DeltaSeconds)
	{
		if (!bVideoPresentationStarted)
		{
			--VideoWarmupFramesRemaining;
			if (VideoWarmupFramesRemaining <= 0 && ShownWidget.IsValid())
			{
				ShownWidget->ResetRewardFlow();
				ShownWidget->SetRewardPresentationManualTick(true);
				ShownWidget->AdvanceRewardFlow();
				ShownWidget->OpenRewardChest();
				VideoElapsed = 0.f;
				bVideoPresentationStarted = true;
			}
			return true;
		}

		if (VideoFrameIndex < VideoFrameCount
			&& !FScreenshotRequest::IsScreenshotRequested())
		{
			ShownWidget->AdvanceRewardPresentation(
				1.f / static_cast<float>(VideoFrameRate));
			const FString OutputPath = FPaths::Combine(VideoFrameDirectory,
				FString::Printf(TEXT("Frame_%04d.png"), VideoFrameIndex));
			FScreenshotRequest::RequestScreenshot(
				OutputPath, true, false, false);
			++VideoFrameIndex;
		}

		if (VideoFrameIndex >= VideoFrameCount && !FScreenshotRequest::IsScreenshotRequested())
		{
			const FString FinalPath = FPaths::Combine(VideoFrameDirectory,
				FString::Printf(TEXT("Frame_%04d.png"), VideoFrameCount - 1));
			UE_LOG(LogRD, Display,
				TEXT("RD_REWARD_CONCEPT03_INGAME_VIDEO complete frames=%d dir=%s final_exists=%d"),
				VideoFrameIndex, *VideoFrameDirectory,
				IFileManager::Get().FileExists(*FinalPath) ? 1 : 0);
			FPlatformMisc::RequestExit(false);
			VideoTickerHandle.Reset();
			return false;
		}
		return true;
	}

	void StartVideoCaptureAndQuit()
	{
		if (VideoTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(VideoTickerHandle);
		}
		VideoFrameDirectory = FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("UI"), TEXT("RewardConcept03New"),
			TEXT("InGameVideoFrames"),
			FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
		IFileManager::Get().MakeDirectory(*VideoFrameDirectory, true);
		VideoElapsed = 0.f;
		VideoFrameIndex = 0;
		VideoWarmupFramesRemaining = 8;
		bVideoPresentationStarted = false;
		// PNG 쓰기가 한 프레임을 지연시켜도 WBP 연출은 정확히 30fps로 진행한다.
		FApp::SetUseFixedTimeStep(true);
		FApp::SetFixedDeltaTime(1.0 / static_cast<double>(VideoFrameRate));
		VideoTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			TEXT("RewardConcept03InGameVideoCapture"), 0.f,
			[](const float DeltaSeconds)
			{
				return TickVideoCaptureAndQuit(DeltaSeconds);
			});
		UE_LOG(LogRD, Display,
			TEXT("RD_REWARD_CONCEPT03_INGAME_VIDEO begin fps=%d frames=%d dir=%s"),
			VideoFrameRate, VideoFrameCount, *VideoFrameDirectory);
	}

	void Show(UWorld* World, const TCHAR* WidgetPath, const bool bStartOpening)
	{
		RemoveShownWidget();
		if (World == nullptr || !World->IsGameWorld())
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.RewardConcept03Preview: 게임 월드에서만 실행할 수 있습니다."));
			return;
		}

		UClass* WidgetClass = LoadClass<URewardConcept03Widget>(nullptr, WidgetPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.RewardConcept03Preview: WBP 클래스를 찾지 못했습니다: %s"),
				WidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		URewardConcept03Widget* Widget = Controller != nullptr
			? CreateWidget<URewardConcept03Widget>(Controller, WidgetClass)
			: CreateWidget<URewardConcept03Widget>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.RewardConcept03Preview: WBP 생성에 실패했습니다."));
			return;
		}

		Widget->AddToViewport(10000);
		Widget->ResetRewardFlow();
		ShownWidget = Widget;

		if (Controller != nullptr)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(Widget->TakeWidget());
			Controller->SetInputMode(InputMode);
			Controller->SetShowMouseCursor(true);
		}

		if (bStartOpening)
		{
			Widget->AdvanceRewardFlow();
			Widget->OpenRewardChest();
		}

		UE_LOG(LogRD, Display,
			TEXT("RD_REWARD_CONCEPT03_PREVIEW shown path=%s auto=%d"),
			WidgetPath, bStartOpening ? 1 : 0);
	}

	void ShowFourStep(UWorld* World)
	{
		Show(World, FourStepWidgetPath, false);
	}

	void ShowFourStepAuto(UWorld* World)
	{
		Show(World, FourStepWidgetPath, true);
	}

	void ShowThreeStep(UWorld* World)
	{
		Show(World, ThreeStepWidgetPath, false);
	}

	void ShowThreeStepAuto(UWorld* World)
	{
		Show(World, ThreeStepWidgetPath, true);
	}

	void CaptureFourStepAndQuit(UWorld* World)
	{
		Show(World, FourStepWidgetPath, true);
		if (ShownWidget.IsValid())
		{
			StartCaptureAndQuit();
		}
	}

	void RecordFourStepAndQuit(UWorld* World)
	{
		Show(World, FourStepWidgetPath, false);
		if (ShownWidget.IsValid())
		{
			StartVideoCaptureAndQuit();
		}
	}

	FAutoConsoleCommandWithWorld ShowFourStepCommand(
		TEXT("RD.RewardConcept03Preview"),
		TEXT("실제 게임 월드에 4단계 신규 전투 보상 WBP를 표시한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowFourStep));

	FAutoConsoleCommandWithWorld ShowFourStepAutoCommand(
		TEXT("RD.RewardConcept03Preview.Auto"),
		TEXT("4단계 신규 전투 보상 WBP를 표시하고 상자 개봉 연출을 시작한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowFourStepAuto));

	FAutoConsoleCommandWithWorld ShowThreeStepCommand(
		TEXT("RD.RewardConcept03Preview.NoArtifact"),
		TEXT("실제 게임 월드에 무아티팩트 3단계 전투 보상 WBP를 표시한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowThreeStep));

	FAutoConsoleCommandWithWorld ShowThreeStepAutoCommand(
		TEXT("RD.RewardConcept03Preview.NoArtifact.Auto"),
		TEXT("무아티팩트 3단계 WBP를 표시하고 상자 개봉 연출을 시작한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowThreeStepAuto));

	FAutoConsoleCommandWithWorld CaptureFourStepAndQuitCommand(
		TEXT("RD.RewardConcept03Preview.CaptureAndQuit"),
		TEXT("4단계 런타임 연출을 게임 월드에서 4장 캡처하고 종료한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&CaptureFourStepAndQuit));

	FAutoConsoleCommandWithWorld RecordFourStepAndQuitCommand(
		TEXT("RD.RewardConcept03Preview.RecordAndQuit"),
		TEXT("4단계 런타임 연출을 게임 월드에서 30fps PNG 시퀀스로 저장하고 종료한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&RecordFourStepAndQuit));
}

#endif
