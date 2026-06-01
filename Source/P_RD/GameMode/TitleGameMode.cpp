#include "GameMode/TitleGameMode.h"

#include "Blueprint/UserWidget.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "PCGStage/Room.h"
#include "PCGStage/Stage.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

namespace
{
	constexpr int32 DefaultDifficulty = 1;

	FText GetRoomTitle(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Monster:
			return NSLOCTEXT("TitleGameMode", "MonsterRoomTitle", "MONSTER");
		case ERoomType::EliteMonster:
			return NSLOCTEXT("TitleGameMode", "EliteRoomTitle", "ELITE");
		case ERoomType::BossMonster:
			return NSLOCTEXT("TitleGameMode", "BossRoomTitle", "BOSS");
		case ERoomType::Shop:
			return NSLOCTEXT("TitleGameMode", "ShopRoomTitle", "SHOP");
		case ERoomType::Treasure:
			return NSLOCTEXT("TitleGameMode", "TreasureRoomTitle", "TREASURE");
		default:
			return NSLOCTEXT("TitleGameMode", "UnknownRoomTitle", "ROOM");
		}
	}

	FText GetRoomDescription(const FRoom& Room)
	{
		return FText::Format(
			NSLOCTEXT("TitleGameMode", "MapRoomDescription", "Room {0}-{1} | Next routes: {2}"),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1),
			FText::AsNumber(Room.mNextRoomColumns.Num()));
	}

	void BuildMapRoomViews(const FStage& Stage, TArray<FTitleMapRoomView>& OutRooms)
	{
		OutRooms.Reset();

		for (int32 RowIndex = 0; RowIndex < Stage.mRoomRows.Num(); ++RowIndex)
		{
			const FRoomRow& RoomRow = Stage.mRoomRows[RowIndex];
			for (int32 ColumnIndex = 0; ColumnIndex < RoomRow.mRooms.Num(); ++ColumnIndex)
			{
				if (!RoomRow.mRooms[ColumnIndex].IsValid())
				{
					continue;
				}

				const FRoom& Room = RoomRow.mRooms[ColumnIndex].Get<FRoom>();
				if (Room.mType == ERoomType::None)
				{
					continue;
				}

				FTitleMapRoomView NewView;
				NewView.mRow = RowIndex;
				NewView.mColumn = ColumnIndex;
				NewView.mType = Room.mType;
				NewView.mTitle = GetRoomTitle(Room.mType);
				NewView.mDescription = GetRoomDescription(Room);
				NewView.mNextRoomColumns = Room.mNextRoomColumns;
				NewView.mPositionOffsetRate = Room.mPositionOffsetRate;
				NewView.bSelectable = RowIndex == Stage.mCurRow && ColumnIndex == Stage.mCurColumn;
				NewView.bSelected = NewView.bSelectable;
				NewView.bVisited = RowIndex < Stage.mCurRow;
				OutRooms.Add(MoveTemp(NewView));
			}
		}
	}
}

ATitleGameMode::ATitleGameMode()
{
	if (TSubclassOf<UUserWidget> TitleWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_TitleMenu.WBP_TitleMenu_C")))
	{
		mHUDClass = TitleWidgetClass;
	}
	else
	{
		mHUDClass = UTitleMenuWidget::StaticClass();
	}

	mWorldWidgets = { EWorldWidgetType::TopMenuBar, EWorldWidgetType::MsgNotify, EWorldWidgetType::SaveNotify };
}

void ATitleGameMode::InitializeCommonRoom()
{
}

void ATitleGameMode::BeginRoom()
{
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		WorldWidgetSubsystem->OpenHUD();
		mTitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>();
	}

	if (UTitleMenuWidget* TitleMenuWidget = mTitleMenuWidget.Get())
	{
		TitleMenuWidget->OnConfirmRequested.AddUniqueDynamic(this, &ATitleGameMode::HandleTitleConfirmRequested);
		TitleMenuWidget->OnContinueRequested.AddUniqueDynamic(this, &ATitleGameMode::HandleTitleContinueRequested);
	}

	RefreshTitleMapFromRun();

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

void ATitleGameMode::HandleTitleConfirmRequested()
{
	if (StartPreviewRun())
	{
		if (UTitleMenuWidget* TitleMenuWidget = mTitleMenuWidget.Get())
		{
			TitleMenuWidget->OpenMapFromTopBar();
		}
	}
}

void ATitleGameMode::HandleTitleContinueRequested()
{
	RefreshTitleMapFromRun();
}

void ATitleGameMode::HandleStageCreated(const FStage& NewStage)
{
	(void)NewStage;
	RefreshTitleMapFromRun();
}

void ATitleGameMode::RefreshTitleMapFromRun()
{
	UTitleMenuWidget* TitleMenuWidget = mTitleMenuWidget.Get();
	if (TitleMenuWidget == nullptr)
	{
		return;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || RunPersistData->IsActive() == false)
	{
		TitleMenuWidget->SetRunControlsVisible(false);
		TitleMenuWidget->SetMapRoomViews(TArray<FTitleMapRoomView>(), false);
		return;
	}

	TArray<FTitleMapRoomView> RoomViews;
	BuildMapRoomViews(RunPersistData->GetStage(), RoomViews);
	TitleMenuWidget->SetMapRoomViews(RoomViews, true);
}

bool ATitleGameMode::StartPreviewRun()
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager == nullptr)
	{
		return false;
	}

	TArray<FPrimaryAssetId> PlayerUnitIds;
	AssetManager->GetPrimaryAssetIdList(UnitPrimaryAssetTypes::GetPlayerUnitType(), PlayerUnitIds);
	if (PlayerUnitIds.IsEmpty())
	{
		return false;
	}

	PlayerUnitIds.Sort([](const FPrimaryAssetId& Lhs, const FPrimaryAssetId& Rhs) {
		return Lhs.ToString() < Rhs.ToString();
		});

	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	if (GameProfileSubsystem == nullptr)
	{
		return false;
	}

	GameProfileSubsystem->MakeUser(NSLOCTEXT("TitleGameMode", "DefaultUserName", "Player"));
	GameProfileSubsystem->StartRun(PlayerUnitIds[0], DefaultDifficulty);

	URunPersistData* RunPersistData = GetRunMutableData();
	if (RunPersistData == nullptr)
	{
		return false;
	}

	RunPersistData->MakeStageAsync(EStageLevelType::Stage1, FOnCreateStage::CreateUObject(this, &ATitleGameMode::HandleStageCreated));
	return true;
}
