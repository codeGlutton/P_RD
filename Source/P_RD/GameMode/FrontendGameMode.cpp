#include "GameMode/FrontendGameMode.h"

#include "Blueprint/UserWidget.h"
#include "DataAsset/BundleType.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "PCGStage/Room.h"
#include "PCGStage/Stage.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

namespace
{
	constexpr int32 DefaultDifficulty = 1;

	FText FrontendText(const TCHAR* Key, const TCHAR* Fallback)
	{
		return FText::FromString(Fallback != nullptr ? Fallback : Key);
	}

	FText GetRoomTitle(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Monster:
			return NSLOCTEXT("FrontendGameMode", "MonsterRoomTitle", "Monster");
		case ERoomType::EliteMonster:
			return NSLOCTEXT("FrontendGameMode", "EliteRoomTitle", "Elite");
		case ERoomType::BossMonster:
			return NSLOCTEXT("FrontendGameMode", "BossRoomTitle", "Boss");
		case ERoomType::Shop:
			return NSLOCTEXT("FrontendGameMode", "ShopRoomTitle", "Shop");
		case ERoomType::Treasure:
			return NSLOCTEXT("FrontendGameMode", "TreasureRoomTitle", "Treasure");
		default:
			return NSLOCTEXT("FrontendGameMode", "UnknownRoomTitle", "Unknown");
		}
	}

	FText GetRoomDescription(const FRoom& Room)
	{
		return FText::Format(
			NSLOCTEXT("FrontendGameMode", "MapRoomDescription", "Row {0}, Column {1}. Next routes: {2}"),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1),
			FText::AsNumber(Room.mNextRoomColumns.Num()));
	}

	FText GetStartPointDescription(const FRoom& Room)
	{
		return FText::Format(
			NSLOCTEXT("FrontendGameMode", "StartPointDescription", "Routes: {0}"),
			FText::AsNumber(Room.mNextRoomColumns.Num()));
	}

	FText GetPlayerJobText(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightJobText", "KNIGHT");
		case EPlayerJobType::Archer:
			return NSLOCTEXT("FrontendGameMode", "ArcherJobText", "ARCHER");
		case EPlayerJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageJobText", "MAGE");
		default:
			return NSLOCTEXT("FrontendGameMode", "UnknownJobText", "UNKNOWN");
		}
	}

	bool IsValidStageRoom(const FStage& Stage, int32 RowIndex, int32 ColumnIndex)
	{
		return Stage.mRoomRows.IsValidIndex(RowIndex)
			&& Stage.mRoomRows[RowIndex].mRooms.IsValidIndex(ColumnIndex)
			&& Stage.mRoomRows[RowIndex].mRooms[ColumnIndex].IsValid()
			&& Stage.mRoomRows[RowIndex].mRooms[ColumnIndex].Get<FRoom>().mType != ERoomType::None;
	}

	bool IsStageStartPoint(const FStage& Stage, int32 RowIndex, int32 ColumnIndex)
	{
		return RowIndex == 0 && ColumnIndex == Stage.mStartColumn;
	}

	bool IsNextRoomFromCurrentPath(const FStage& Stage, int32 CurrentRowIndex, int32 CurrentColumnIndex, int32 RowIndex, int32 ColumnIndex)
	{
		if (!IsValidStageRoom(Stage, CurrentRowIndex, CurrentColumnIndex))
		{
			return false;
		}

		const FRoom& CurrentRoom = Stage.mRoomRows[CurrentRowIndex].mRooms[CurrentColumnIndex].Get<FRoom>();
		return RowIndex == CurrentRowIndex + 1 && CurrentRoom.mNextRoomColumns.Contains(ColumnIndex);
	}

	EFrontendMapRoomState ResolveMapRoomState(
		const FStage& Stage,
		const FRoom& Room,
		int32 CurrentRowIndex,
		int32 CurrentColumnIndex)
	{
		if (Room.mRow == CurrentRowIndex && Room.mColumn == CurrentColumnIndex)
		{
			return EFrontendMapRoomState::Selected;
		}

		if (Room.mRow < CurrentRowIndex)
		{
			return EFrontendMapRoomState::Cleared;
		}

		if (IsNextRoomFromCurrentPath(Stage, CurrentRowIndex, CurrentColumnIndex, Room.mRow, Room.mColumn))
		{
			return EFrontendMapRoomState::Ready;
		}

		return EFrontendMapRoomState::Locked;
	}

	const UStaticPlayerUnitSpawnData* GetLoadedPlayerUnitSpawnData(const FPrimaryAssetId& PlayerUnitId)
	{
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager == nullptr || !PlayerUnitId.IsValid())
		{
			return nullptr;
		}

		if (const UStaticPlayerUnitSpawnData* PlayerUnitData = AssetManager->GetPrimaryAssetObject<UStaticPlayerUnitSpawnData>(PlayerUnitId))
		{
			return PlayerUnitData;
		}

		return Cast<UStaticPlayerUnitSpawnData>(AssetManager->GetPrimaryAssetPath(PlayerUnitId).TryLoad());
	}

	FText GetPlayerJobName(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightNameText", "Knight");
		case EPlayerJobType::Archer:
			return NSLOCTEXT("FrontendGameMode", "ArcherNameText", "Archer");
		case EPlayerJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageNameText", "Mage");
		default:
			return NSLOCTEXT("FrontendGameMode", "UnknownNameText", "Unknown");
		}
	}

	bool HasCharacterOptionForJob(const TArray<FFrontendCharacterOption>& Options, EPlayerJobType JobType)
	{
		const FText JobText = GetPlayerJobText(JobType);
		return Options.ContainsByPredicate([&JobText](const FFrontendCharacterOption& Option)
		{
			return Option.mRoleText.EqualTo(JobText);
		});
	}

	void AppendLockedCharacterOption(TArray<FFrontendCharacterOption>& Options, EPlayerJobType JobType)
	{
		FFrontendCharacterOption NewOption;
		NewOption.mIndex = Options.Num();
		NewOption.mDisplayName = GetPlayerJobName(JobType);
		NewOption.mRoleText = GetPlayerJobText(JobType);
		NewOption.mDescription = NSLOCTEXT("FrontendGameMode", "LockedCharacterDescription", "Character data is not ready");
		NewOption.mDisabledReason = NewOption.mDescription;
		NewOption.bSelectable = false;
		Options.Add(MoveTemp(NewOption));
	}
}

AFrontendGameMode::AFrontendGameMode()
{
	mWorldWidgets.Empty();
}

bool AFrontendGameMode::StartNewRunFromTitle()
{
	return OpenTitleCharacterSelect();
}

bool AFrontendGameMode::StartRunWithPlayerUnit(FPrimaryAssetId PlayerUnitId)
{
	return PrepareRunMapWithPlayerUnit(PlayerUnitId);
}

bool AFrontendGameMode::GetPlayerUnitIds(TArray<FPrimaryAssetId>& OutPlayerUnitIds) const
{
	OutPlayerUnitIds.Reset();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager == nullptr)
	{
		return false;
	}

	AssetManager->GetPrimaryAssetIdList(UnitPrimaryAssetTypes::GetPlayerUnitType(), OutPlayerUnitIds);
	OutPlayerUnitIds.Sort([](const FPrimaryAssetId& Lhs, const FPrimaryAssetId& Rhs)
	{
		return Lhs.ToString() < Rhs.ToString();
	});
	return !OutPlayerUnitIds.IsEmpty();
}

bool AFrontendGameMode::GetCharacterOptions(TArray<FFrontendCharacterOption>& OutOptions) const
{
	OutOptions.Reset();

	TArray<FPrimaryAssetId> PlayerUnitIds;
	if (!GetPlayerUnitIds(OUT PlayerUnitIds))
	{
		return false;
	}

	bool bHasUnloadedData = false;
	for (const FPrimaryAssetId& PlayerUnitId : PlayerUnitIds)
	{
		const UStaticPlayerUnitSpawnData* PlayerUnitData = GetLoadedPlayerUnitSpawnData(PlayerUnitId);
		if (PlayerUnitData == nullptr)
		{
			bHasUnloadedData = true;
			continue;
		}

		FFrontendCharacterOption NewOption;
		NewOption.mIndex = OutOptions.Num();
		NewOption.mDisplayName = PlayerUnitData->mDisplayName.IsEmpty()
			? FText::FromName(PlayerUnitId.PrimaryAssetName)
			: PlayerUnitData->mDisplayName;
		NewOption.mRoleText = GetPlayerJobText(PlayerUnitData->mJobType);
		NewOption.mDescription = PlayerUnitData->mDescription;
		NewOption.mMaxHP = FMath::RoundToInt(PlayerUnitData->GetDefaultMaxHP(DefaultDifficulty));
		NewOption.mDice = PlayerUnitData->mDiceDatas.Num();
		NewOption.mGold = FMath::RoundToInt(PlayerUnitData->GetDefaultMoney(DefaultDifficulty));
		NewOption.mStatSummary = FText::Format(
			NSLOCTEXT("FrontendGameMode", "CharacterStatSummary", "HP {0} / Dice {1} / Gold {2}"),
			FText::AsNumber(NewOption.mMaxHP),
			FText::AsNumber(NewOption.mDice),
			FText::AsNumber(NewOption.mGold));
		NewOption.mPortrait = PlayerUnitData->mPortrait;
		NewOption.mIcon = PlayerUnitData->mIcon;
		NewOption.mPlayerUnitId = PlayerUnitId;
		NewOption.bSelectable = PlayerUnitId.IsValid() && !PlayerUnitData->mClass.IsNull();
		OutOptions.Add(MoveTemp(NewOption));
	}

	if (bHasUnloadedData)
	{
		const_cast<AFrontendGameMode*>(this)->RequestPlayerUnitSpawnDataPreload();
	}

	if (!HasCharacterOptionForJob(OutOptions, EPlayerJobType::Archer))
	{
		AppendLockedCharacterOption(OutOptions, EPlayerJobType::Archer);
	}
	if (!HasCharacterOptionForJob(OutOptions, EPlayerJobType::Mage))
	{
		AppendLockedCharacterOption(OutOptions, EPlayerJobType::Mage);
	}

	return !OutOptions.IsEmpty();
}

bool AFrontendGameMode::IsCharacterOptionsLoading() const
{
	return mPlayerUnitSpawnDataPreloadHandle.IsValid() && !mPlayerUnitSpawnDataPreloadHandle->HasLoadCompleted();
}

bool AFrontendGameMode::PrepareRunMapWithPlayerUnit(FPrimaryAssetId PlayerUnitId)
{
	return StartRunPreviewWithPlayerUnit(PlayerUnitId);
}

bool AFrontendGameMode::GetMapRoomViews(TArray<FFrontendMapRoomView>& OutRooms) const
{
	OutRooms.Reset();

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || !RunPersistData->IsActive())
	{
		return false;
	}

	int32 CurrentRowIndex = 0;
	int32 CurrentColumnIndex = 0;
	RunPersistData->GetCurrentRoomIndex(OUT CurrentRowIndex, OUT CurrentColumnIndex);

	const FStage& Stage = RunPersistData->GetStage();
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

			const bool bIsStartPoint = IsStageStartPoint(Stage, RowIndex, ColumnIndex);
			const bool bIsCurrentRoom = RowIndex == CurrentRowIndex && ColumnIndex == CurrentColumnIndex;
			const EFrontendMapRoomState RoomState = ResolveMapRoomState(
				Stage,
				Room,
				CurrentRowIndex,
				CurrentColumnIndex);

			FFrontendMapRoomView NewView;
			NewView.mRow = RowIndex;
			NewView.mColumn = ColumnIndex;
			NewView.mType = Room.mType;
			NewView.mState = RoomState;
			NewView.mTitle = bIsStartPoint ? NSLOCTEXT("FrontendGameMode", "StartPointTitle", "Start") : GetRoomTitle(Room.mType);
			NewView.mDescription = bIsStartPoint ? GetStartPointDescription(Room) : GetRoomDescription(Room);
			NewView.mNextRoomColumns = Room.mNextRoomColumns;
			NewView.mPositionOffsetRate = Room.mPositionOffsetRate;
			NewView.bSelectable = bIsCurrentRoom;
			NewView.bSelected = bIsCurrentRoom;
			NewView.bVisited = RoomState == EFrontendMapRoomState::Cleared;
			NewView.bCanEnter = bIsCurrentRoom;
			NewView.bIsStartPoint = bIsStartPoint;
			OutRooms.Add(MoveTemp(NewView));
		}
	}

	return !OutRooms.IsEmpty();
}

bool AFrontendGameMode::HasActiveRun() const
{
	const URunPersistData* RunPersistData = GetRunPersistData();
	return RunPersistData != nullptr && RunPersistData->IsActive();
}

bool AFrontendGameMode::CanSaveRun() const
{
	return HasActiveRun() && !bStartRunRequested;
}

bool AFrontendGameMode::CanAbandonRun() const
{
	return HasActiveRun() && !bStartRunRequested;
}

bool AFrontendGameMode::GetRunControlView(FFrontendRunControlView& OutView) const
{
	OutView = FFrontendRunControlView();
	OutView.bHasActiveRun = HasActiveRun();
	OutView.bCanSaveRun = CanSaveRun();
	OutView.bCanAbandonRun = CanAbandonRun();

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || !RunPersistData->IsActive())
	{
		return false;
	}

	RunPersistData->GetCurrentRoomIndex(OUT OutView.mRow, OUT OutView.mColumn);
	OutView.bIsAtStageStart = IsStageStartPoint(RunPersistData->GetStage(), OutView.mRow, OutView.mColumn);
	OutView.mPlayerLevel = RunPersistData->GetPlayerLevel();
	OutView.mDifficulty = RunPersistData->GetDifficulty();
	return true;
}

bool AFrontendGameMode::GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const
{
	FFrontendRunControlView RunView;
	if (!GetRunControlView(OUT RunView))
	{
		RowIndex = 0;
		ColumnIndex = 0;
		PlayerLevel = 0;
		Difficulty = 0;
		return false;
	}

	RowIndex = RunView.mRow;
	ColumnIndex = RunView.mColumn;
	PlayerLevel = RunView.mPlayerLevel;
	Difficulty = RunView.mDifficulty;
	return true;
}

bool AFrontendGameMode::SaveRunFromTitleAsync(FAsyncSaveGameToSlotDelegate Callback) const
{
	if (!CanSaveRun())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	if (SaveGameSubsystem == nullptr)
	{
		return false;
	}

	SaveGameSubsystem->SaveRunAsync(MoveTemp(Callback));
	return true;
}

bool AFrontendGameMode::AbandonRunFromTitle()
{
	if (!CanAbandonRun())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	if (GameProfileSubsystem == nullptr)
	{
		return false;
	}

	GameProfileSubsystem->EndRun();
	if (USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
	{
		SaveGameSubsystem->ClearRun();
	}

	bStartRunRequested = false;
	return true;
}

bool AFrontendGameMode::SelectMapRoom(int32 RowIndex, int32 ColumnIndex)
{
	URunPersistData* RunPersistData = GetRunMutableData();
	if (RunPersistData == nullptr || !RunPersistData->IsActive())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	const FStage& Stage = RunPersistData->GetStage();
	if (!IsValidStageRoom(Stage, RowIndex, ColumnIndex))
	{
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	int32 CurrentRowIndex = 0;
	int32 CurrentColumnIndex = 0;
	RunPersistData->GetCurrentRoomIndex(OUT CurrentRowIndex, OUT CurrentColumnIndex);
	if (RowIndex != CurrentRowIndex || ColumnIndex != CurrentColumnIndex)
	{
		ShowTitleMessage(FrontendText(TEXT("LockedMapRoom"), TEXT("This room is locked")));
		return false;
	}

	RunPersistData->SetCurrentRoomIndex(RowIndex, ColumnIndex);
	return true;
}

bool AFrontendGameMode::EnterSelectedMapRoom()
{
	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || !RunPersistData->IsActive())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	int32 RowIndex = 0;
	int32 ColumnIndex = 0;
	RunPersistData->GetCurrentRoomIndex(OUT RowIndex, OUT ColumnIndex);

	ShowTitleMessage(FrontendText(TEXT("LoadingSelectedRoom"), TEXT("Loading selected room")));
	PreloadRoomAsync(RowIndex, ColumnIndex);
	TransitionLoadedRoomAsync();
	return true;
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
		PlayerController->ActivateTouchInterface(nullptr);
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}
}

void AFrontendGameMode::RequestPlayerUnitSpawnDataPreload()
{
	if (bPlayerUnitSpawnDataPreloadRequested || IsCharacterOptionsLoading())
	{
		return;
	}

	TArray<FPrimaryAssetId> PlayerUnitIds;
	if (!GetPlayerUnitIds(OUT PlayerUnitIds))
	{
		return;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager == nullptr)
	{
		return;
	}

	bPlayerUnitSpawnDataPreloadRequested = true;
	TArray<FName> Bundles = { BUNDLE_UI };
	FAssetManagerLoadParams LoadParams;
	LoadParams.OnComplete.BindUObject(this, &AFrontendGameMode::HandlePlayerUnitSpawnDataPreloaded);
	mPlayerUnitSpawnDataPreloadHandle = AssetManager->PreloadPrimaryAssets(PlayerUnitIds, Bundles, true, MoveTemp(LoadParams));
}

void AFrontendGameMode::HandlePlayerUnitSpawnDataPreloaded(TSharedPtr<FStreamableHandle> AssetHandle)
{
	bPlayerUnitSpawnDataPreloadRequested = false;
	mPlayerUnitSpawnDataPreloadHandle = AssetHandle;

	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
		{
			TitleMenuWidget->RefreshCharacterOptionsFromGameMode();
		}
	}
}

bool AFrontendGameMode::OpenTitleCharacterSelect()
{
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
		{
			TitleMenuWidget->OpenCharacterSelectFromTitle();
			return true;
		}
	}

	ShowTitleMessage(FrontendText(TEXT("MissingTitleMenu"), TEXT("Title menu is not ready")));
	return false;
}

bool AFrontendGameMode::StartRunPreviewWithPlayerUnit(FPrimaryAssetId PlayerUnitId)
{
	if (bStartRunRequested)
	{
		ShowTitleMessage(FrontendText(TEXT("StartRunBlocked"), TEXT("Run is already preparing")));
		return false;
	}

	if (!PlayerUnitId.IsValid())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingPlayerUnit"), TEXT("Character data is missing")));
		return false;
	}

	TArray<FPrimaryAssetId> PlayerUnitIds;
	if (!GetPlayerUnitIds(OUT PlayerUnitIds) || !PlayerUnitIds.Contains(PlayerUnitId))
	{
		ShowTitleMessage(FrontendText(TEXT("MissingPlayerUnit"), TEXT("Character data is missing")));
		return false;
	}

	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	if (GameProfileSubsystem == nullptr)
	{
		return false;
	}

	bStartRunRequested = true;

	GameProfileSubsystem->MakeUser(NSLOCTEXT("FrontendGameMode", "DefaultUserName", "Player"));
	GameProfileSubsystem->StartRun(PlayerUnitId, DefaultDifficulty);

	URunPersistData* RunPersistData = GetRunMutableData();
	if (RunPersistData == nullptr)
	{
		bStartRunRequested = false;
		return false;
	}

	// 지도 미리보기는 "새 Stage를 만들되 아직 방 레벨로 전환하지 않는" 흐름이다.
	// PM 브랜치의 MakeStageAndPreloadRoomAsync()는 방 전환 준비용 API라 여기서 쓰면
	// 첫 방 preload/transition 상태까지 섞인다. 그래서 Stage 생성 공식 API인
	// URunPersistData::MakeStageAsync()만 호출하고, 지도 입장은 EnterSelectedMapRoom()에서
	// RoomGameModeBase의 PreloadRoomAsync()/TransitionLoadedRoomAsync() 흐름으로 넘긴다.
	RunPersistData->MakeStageAsync(EStageLevelType::Stage1, FOnCreateStage::CreateUObject(this, &AFrontendGameMode::HandleStageCreated));
	return true;
}

void AFrontendGameMode::HandleStageCreated(const FStage& NewStage)
{
	bStartRunRequested = false;

	if (URunPersistData* RunPersistData = GetRunMutableData())
	{
		const FRoom& StartRoom = NewStage.GetStartRoom();
		RunPersistData->SetCurrentRoomIndex(StartRoom.mRow, StartRoom.mColumn);
	}
}

void AFrontendGameMode::ShowTitleMessage(const FText& Message) const
{
	if (!Message.IsEmpty())
	{
		UE_LOG(LogRD, Display, TEXT("FrontendGameMode: %s"), *Message.ToString());
	}
}
