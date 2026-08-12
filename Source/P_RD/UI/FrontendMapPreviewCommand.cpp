/**
 * @file FrontendMapPreviewCommand.cpp
 * @brief 저장 상태를 바꾸지 않고 현재 런 지도를 조회 모드로 여는 개발 명령.
 */

#include "RDMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/Paths.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "Slate/WidgetRenderer.h"
#include "UI/FrontendMapWidget.h"

#if !UE_BUILD_SHIPPING

namespace FrontendMapPreview
{
	TWeakObjectPtr<UFrontendMapWidget> ShownWidget;
	TWeakObjectPtr<UFrontendMapWidget> DataPreviewWidget;

	FText RoomTitle(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Monster:      return NSLOCTEXT("FrontendMapPreview", "Monster", "Monster");
		case ERoomType::EliteMonster: return NSLOCTEXT("FrontendMapPreview", "Elite", "Elite");
		case ERoomType::BossMonster:  return NSLOCTEXT("FrontendMapPreview", "Boss", "Boss");
		case ERoomType::Shop:         return NSLOCTEXT("FrontendMapPreview", "Shop", "Shop");
		case ERoomType::Treasure:     return NSLOCTEXT("FrontendMapPreview", "Treasure", "Treasure");
		default:                      return NSLOCTEXT("FrontendMapPreview", "Unknown", "Unknown");
		}
	}

	bool BuildCurrentRunViews(UWorld* World, TArray<FMapRoomView>& OutRooms,
		bool& bOutAtStageStart)
	{
		OutRooms.Reset();
		bOutAtStageStart = false;
		if (World == nullptr || World->GetGameInstance() == nullptr)
		{
			return false;
		}

		const UPersistentDataSubsystem* Persistent =
			World->GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
		const URunPersistData* Run = Persistent != nullptr
			? Persistent->GetRunPersistData() : nullptr;
		if (Run == nullptr || Run->IsActive() == false)
		{
			return false;
		}

		int32 CurrentRow = INDEX_NONE;
		int32 CurrentColumn = INDEX_NONE;
		Run->GetCurrentRoomIndex(OUT CurrentRow, OUT CurrentColumn);
		const FStage& Stage = Run->GetStage();
		bOutAtStageStart = CurrentRow == 0 && CurrentColumn == Stage.mStartColumn;

		const FRoom* CurrentRoom = Stage.HasRoom(CurrentRow, CurrentColumn)
			? &Stage.mRoomRows[CurrentRow].mRooms[CurrentColumn].Get<FRoom>() : nullptr;
		for (int32 RowIndex = 0; RowIndex < Stage.mRoomRows.Num(); ++RowIndex)
		{
			const FRoomRow& RoomRow = Stage.mRoomRows[RowIndex];
			for (int32 ColumnIndex = 0; ColumnIndex < RoomRow.mRooms.Num(); ++ColumnIndex)
			{
				if (Stage.HasRoom(RowIndex, ColumnIndex) == false)
				{
					continue;
				}

				const FRoom& Room = RoomRow.mRooms[ColumnIndex].Get<FRoom>();
				const bool bReady = CurrentRoom != nullptr
					&& RowIndex == CurrentRow + 1
					&& CurrentRoom->mNextRoomColumns.Contains(ColumnIndex);
				const EMapRoomState State = Room.mWasSelected
					? EMapRoomState::Cleared
					: (bReady ? EMapRoomState::Ready : EMapRoomState::Locked);

				FMapRoomView& View = OutRooms.AddDefaulted_GetRef();
				View.mRow = RowIndex;
				View.mColumn = ColumnIndex;
				View.mType = Room.mType;
				View.mState = State;
				View.mTitle = RowIndex == 0 && ColumnIndex == Stage.mStartColumn
					? NSLOCTEXT("FrontendMapPreview", "Start", "Start") : RoomTitle(Room.mType);
				View.mNextRoomColumns = Room.mNextRoomColumns;
				View.mPositionOffsetRate = Room.mPositionOffsetRate;
				View.mSelectable = false;
				View.mSelected = false;
				View.mVisited = State == EMapRoomState::Cleared;
				View.mCanEnter = false;
				View.mIsStartPoint = RowIndex == 0 && ColumnIndex == Stage.mStartColumn;
			}
		}
		return OutRooms.IsEmpty() == false;
	}

	void ShowCurrentRunData(UWorld* World)
	{
		if (World == nullptr || World->IsGameWorld() == false)
		{
			return;
		}

		TArray<FMapRoomView> Rooms;
		bool bAtStageStart = false;
		if (BuildCurrentRunViews(World, OUT Rooms, OUT bAtStageStart) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview.Data: 복구된 런 데이터가 아직 없습니다."));
			return;
		}

		UFrontendMapWidget* MapWidget = DataPreviewWidget.Get();
		if (MapWidget == nullptr)
		{
			UClass* MapClass = LoadClass<UFrontendMapWidget>(nullptr,
				TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape."
					"WBP_FrontendMapLandscape_C"));
			MapWidget = MapClass != nullptr
				? CreateWidget<UFrontendMapWidget>(World, MapClass) : nullptr;
			if (MapWidget == nullptr)
			{
				UE_LOG(LogRD, Warning, TEXT("RD.MapPreview.Data: 가로 지도 WBP 생성 실패."));
				return;
			}
			MapWidget->AddToViewport(10000);
			DataPreviewWidget = MapWidget;
		}

		MapWidget->SetRoomSelectionEnabled(false);
		MapWidget->SetPreviewRoomsForDebug(Rooms, bAtStageStart);
		MapWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		MapWidget->RefreshMap();
		UE_LOG(LogRD, Display, TEXT("RD.MapPreview.Data: 격리 런 지도 %d개 방을 표시했습니다."), Rooms.Num());
	}

	void CaptureCurrentRunData(const TArray<FString>& Args, UWorld* World)
	{
		ShowCurrentRunData(World);
		UFrontendMapWidget* MapWidget = DataPreviewWidget.Get();
		if (MapWidget == nullptr)
		{
			return;
		}

		int32 CaptureWidth = 1672;
		int32 CaptureHeight = 941;
		if (Args.Num() > 0)
		{
			if (Args.Num() < 2)
			{
				UE_LOG(LogRD, Warning,
					TEXT("사용법: RD.MapPreview.DataCapture [폭 높이]"));
				return;
			}
			CaptureWidth = FMath::Clamp(FCString::Atoi(*Args[0]), 320, 4096);
			CaptureHeight = FMath::Clamp(FCString::Atoi(*Args[1]), 320, 4096);
		}

		const FVector2D CaptureSize(
			StaticCast<float>(CaptureWidth), StaticCast<float>(CaptureHeight));
		MapWidget->ForceLayoutPrepass();
		FWidgetRenderer Renderer(true, true);
		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
		RenderTarget->ClearColor = FLinearColor::Transparent;
		RenderTarget->InitCustomFormat(
			CaptureWidth, CaptureHeight, PF_B8G8R8A8, true);
		RenderTarget->UpdateResourceImmediate(true);
		Renderer.DrawWidget(RenderTarget, MapWidget->TakeWidget(), CaptureSize, 0.f);

		const FString OutputDirectory = FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("WindowsEditor"));
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);
		const FString OutputName = Args.Num() > 0
			? FString::Printf(TEXT("WBP_FrontendMapLandscape_RunData_%dx%d_20260812.png"),
				CaptureWidth, CaptureHeight)
			: TEXT("WBP_FrontendMapLandscape_RunData_20260812.png");
		UKismetRenderingLibrary::ExportRenderTarget(
			World, RenderTarget, OutputDirectory, OutputName);
		UE_LOG(LogRD, Display, TEXT("RD.MapPreview.DataCapture: %s"),
			*FPaths::Combine(OutputDirectory, OutputName));
	}

	void Toggle(UWorld* World)
	{
		if (ShownWidget.IsValid() && ShownWidget->IsOpened())
		{
			ShownWidget->CloseUI();
			ShownWidget.Reset();
			UE_LOG(LogRD, Display, TEXT("RD.MapPreview: 지도를 닫았습니다."));
			return;
		}

		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview: 게임 월드를 찾지 못했습니다."));
			return;
		}

		UWorldWidgetSubsystem* WidgetSubsystem =
			World->GetSubsystem<UWorldWidgetSubsystem>();
		if (WidgetSubsystem == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview: 월드 위젯 서브시스템이 없습니다."));
			return;
		}

		UFrontendMapWidget* MapWidget =
			WidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
				EWorldWidgetType::WorldMap);
		if (MapWidget == nullptr)
		{
			WidgetSubsystem->InitWorldWidget(EWorldWidgetType::WorldMap);
			MapWidget = WidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
				EWorldWidgetType::WorldMap);
		}
		if (MapWidget == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.MapPreview: 지도 월드 위젯을 만들지 못했습니다."));
			return;
		}

		MapWidget->SetRoomSelectionEnabled(false);
		MapWidget->ClearMapStatusOverride();
		MapWidget->OpenUI();
		MapWidget->RefreshMap();
		ShownWidget = MapWidget;

		UE_LOG(LogRD, Display,
			TEXT("RD.MapPreview: 저장 변경 없이 현재 런 지도를 열었습니다."));
	}

	FAutoConsoleCommandWithWorld PreviewCommand(
		TEXT("RD.MapPreview"),
		TEXT("Toggle the current run map in read-only preview mode."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Toggle));

	FAutoConsoleCommandWithWorld DataPreviewCommand(
		TEXT("RD.MapPreview.Data"),
		TEXT("Show restored run data without entering a room or writing the source save."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowCurrentRunData));

	FAutoConsoleCommandWithWorldAndArgs DataCaptureCommand(
		TEXT("RD.MapPreview.DataCapture"),
		TEXT("Render the restored run map WBP to PNG. Optional: <width> <height>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CaptureCurrentRunData));
}

#endif
