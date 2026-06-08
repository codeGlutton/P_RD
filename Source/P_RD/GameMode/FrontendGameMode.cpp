#include "GameMode/FrontendGameMode.h"

#include "Blueprint/UserWidget.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "PCGStage/Room.h"
#include "PCGStage/Stage.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

namespace
{
	/**
	 * @brief 프론트엔드 런 preview에서 사용하는 임시 난이도
	 *
	 * @details
	 * 현재 UI/map 브랜치는 타이틀 -> 캐릭터 선택 -> 지도 -> 룸 입장 흐름을 먼저 검증하는 단계라
	 * 난이도 선택 화면이 아직 없다. 그래서 캐릭터 시작 스탯 표시와 GameProfileSubsystem::StartRun()
	 * 호출에 같은 기본 난이도를 사용한다.
	 *
	 * 이 값은 게임 규칙을 UI 파트가 결정한다는 뜻이 아니며, 난이도/프로필 선택 API가 생기면
	 * 그 결과를 받아 PM Run API에 전달하는 방식으로 교체해야 한다.
	 */
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
		int32 CurrentColumnIndex,
		int32 SelectedRowIndex,
		int32 SelectedColumnIndex)
	{
		if (Room.mRow == SelectedRowIndex
			&& Room.mColumn == SelectedColumnIndex
			&& IsNextRoomFromCurrentPath(Stage, CurrentRowIndex, CurrentColumnIndex, Room.mRow, Room.mColumn))
		{
			return EFrontendMapRoomState::Selected;
		}

		if ((Room.mRow == CurrentRowIndex && Room.mColumn == CurrentColumnIndex) || Room.mRow < CurrentRowIndex)
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

	for (const FPrimaryAssetId& PlayerUnitId : PlayerUnitIds)
	{
		const UStaticPlayerUnitSpawnData* PlayerUnitData = GetLoadedPlayerUnitSpawnData(PlayerUnitId);
		if (PlayerUnitData == nullptr)
		{
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

bool AFrontendGameMode::PrepareRunMapWithPlayerUnit(FPrimaryAssetId PlayerUnitId)
{
	return StartRunAndEnterFirstRoomWithPlayerUnit(PlayerUnitId);
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
			const EFrontendMapRoomState RoomState = ResolveMapRoomState(
				Stage,
				Room,
				CurrentRowIndex,
				CurrentColumnIndex,
				mSelectedMapRoomRow,
				mSelectedMapRoomColumn);

			FFrontendMapRoomView NewView;
			NewView.mRow = RowIndex;
			NewView.mColumn = ColumnIndex;
			NewView.mType = Room.mType;
			NewView.mState = RoomState;
			NewView.mTitle = bIsStartPoint ? NSLOCTEXT("FrontendGameMode", "StartPointTitle", "Start") : GetRoomTitle(Room.mType);
			NewView.mDescription = bIsStartPoint ? GetStartPointDescription(Room) : GetRoomDescription(Room);
			NewView.mNextRoomColumns = Room.mNextRoomColumns;
			NewView.mPositionOffsetRate = Room.mPositionOffsetRate;
			NewView.bSelectable = RoomState == EFrontendMapRoomState::Ready;
			NewView.bSelected = RoomState == EFrontendMapRoomState::Selected;
			NewView.bVisited = RoomState == EFrontendMapRoomState::Cleared;
			NewView.bCanEnter = RoomState == EFrontendMapRoomState::Selected;
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

bool AFrontendGameMode::CanAbandonRun() const
{
	return HasActiveRun() && !bStartRunRequested;
}

bool AFrontendGameMode::GetRunControlView(FFrontendRunControlView& OutView) const
{
	OutView = FFrontendRunControlView();
	OutView.bHasActiveRun = HasActiveRun();
	OutView.bCanSaveRun = false;
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

	ClearSelectedMapRoom();
	bStartRunRequested = false;
	return true;
}

bool AFrontendGameMode::SelectMapRoom(int32 RowIndex, int32 ColumnIndex)
{
	const URunPersistData* RunPersistData = GetRunPersistData();
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
	if (!IsNextRoomFromCurrentPath(Stage, CurrentRowIndex, CurrentColumnIndex, RowIndex, ColumnIndex))
	{
		ShowTitleMessage(FrontendText(TEXT("LockedMapRoom"), TEXT("This room is locked")));
		return false;
	}

	mSelectedMapRoomRow = RowIndex;
	mSelectedMapRoomColumn = ColumnIndex;
	bSelectedMapRoomPreloadRequested = false;
	return PreloadSelectedMapRoom();
}

bool AFrontendGameMode::EnterSelectedMapRoom()
{
	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || !RunPersistData->IsActive())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	if (!HasSelectedMapRoom())
	{
		ShowTitleMessage(FrontendText(TEXT("NoMapRoomSelected"), TEXT("Select a room")));
		return false;
	}

	const FStage& Stage = RunPersistData->GetStage();
	if (!IsValidStageRoom(Stage, mSelectedMapRoomRow, mSelectedMapRoomColumn))
	{
		ClearSelectedMapRoom();
		ShowTitleMessage(FrontendText(TEXT("MissingStage"), TEXT("Map is not ready")));
		return false;
	}

	int32 CurrentRowIndex = 0;
	int32 CurrentColumnIndex = 0;
	RunPersistData->GetCurrentRoomIndex(OUT CurrentRowIndex, OUT CurrentColumnIndex);
	if (!IsNextRoomFromCurrentPath(Stage, CurrentRowIndex, CurrentColumnIndex, mSelectedMapRoomRow, mSelectedMapRoomColumn))
	{
		ClearSelectedMapRoom();
		ShowTitleMessage(FrontendText(TEXT("LockedMapRoom"), TEXT("This room is locked")));
		return false;
	}

	if (!bSelectedMapRoomPreloadRequested)
	{
		ShowTitleMessage(FrontendText(TEXT("SelectedRoomNotPreloaded"), TEXT("Selected room is not preloaded")));
		return false;
	}

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	if (RoomTransitionSubsystem == nullptr)
	{
		return false;
	}

	ShowTitleMessage(FrontendText(TEXT("EnteringSelectedRoom"), TEXT("Entering selected room")));
	RoomTransitionSubsystem->TransitLoadedRoomAsync();
	return true;
}

void AFrontendGameMode::BeginRoom()
{
	LoadOrCreateFrontendUserProfile();

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->ActivateTouchInterface(nullptr);
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}

	ShowTitleHUD();
}

void AFrontendGameMode::LoadOrCreateFrontendUserProfile()
{
	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	if (SaveGameSubsystem != nullptr)
	{
		SaveGameSubsystem->LoadUser();
	}

	const UUserPersistData* UserPersistData = GetUserPersistData();
	if (UserPersistData != nullptr && UserPersistData->IsActive())
	{
		return;
	}

	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	if (GameProfileSubsystem == nullptr)
	{
		return;
	}

	// 프로필 생성 UI가 들어오기 전까지만 쓰는 기본 유저다. Run 시작 책임과 섞이지 않도록
	// 타이틀 부트스트랩 단계에서 PM Profile API를 통해 만든다.
	GameProfileSubsystem->MakeUser(FrontendText(TEXT("DefaultUserName"), TEXT("Guest")));
	if (SaveGameSubsystem != nullptr)
	{
		SaveGameSubsystem->SaveUser();
	}
}

void AFrontendGameMode::ShowTitleHUD()
{
	if (bTitleHUDShown)
	{
		return;
	}

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>()
		: nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		return;
	}

	UUserWidget* TitleHUD = WorldWidgetSubsystem->GetHUD();
	if (TitleHUD == nullptr)
	{
		return;
	}

	if (!TitleHUD->IsInViewport())
	{
		TitleHUD->AddToViewport();
	}

	TitleHUD->SetVisibility(ESlateVisibility::Visible);
	bTitleHUDShown = true;
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

void AFrontendGameMode::ClearSelectedMapRoom()
{
	mSelectedMapRoomRow = INDEX_NONE;
	mSelectedMapRoomColumn = INDEX_NONE;
	bSelectedMapRoomPreloadRequested = false;
}

bool AFrontendGameMode::HasSelectedMapRoom() const
{
	return mSelectedMapRoomRow != INDEX_NONE && mSelectedMapRoomColumn != INDEX_NONE;
}

bool AFrontendGameMode::PreloadSelectedMapRoom()
{
	if (!HasSelectedMapRoom())
	{
		return false;
	}

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	if (RoomTransitionSubsystem == nullptr)
	{
		return false;
	}

	ShowTitleMessage(FrontendText(TEXT("PreloadingSelectedRoom"), TEXT("Preloading selected room")));
	RoomTransitionSubsystem->PreloadRoomAsync(mSelectedMapRoomRow, mSelectedMapRoomColumn);
	bSelectedMapRoomPreloadRequested = true;
	return true;
}

bool AFrontendGameMode::StartRunAndEnterFirstRoomWithPlayerUnit(FPrimaryAssetId PlayerUnitId)
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

	const UUserPersistData* UserPersistData = GetUserPersistData();
	if (UserPersistData == nullptr || !UserPersistData->IsActive())
	{
		ShowTitleMessage(FrontendText(TEXT("MissingUserData"), TEXT("User data is not ready")));
		return false;
	}

	URoomTransitionSubsystem* RoomTransitionSubsystem = GetGameInstance()->GetSubsystem<URoomTransitionSubsystem>();
	if (RoomTransitionSubsystem == nullptr)
	{
		return false;
	}

	bStartRunRequested = true;
	ClearSelectedMapRoom();

	GameProfileSubsystem->StartRun(PlayerUnitId, DefaultDifficulty);

	// PM 구조에서 새 Stage의 row 0 중앙 StartRoom이 첫 방이다. 전환 Subsystem이 Stage 생성,
	// StartRoom preload, 로드 완료 후 transition까지 한 요청으로 소유하게 둔다.
	RoomTransitionSubsystem->MakeStageAndPreloadRoomAsync(EStageLevelType::Stage1, FOnPreTransitNextRoom(), true);
	return true;
}

void AFrontendGameMode::ShowTitleMessage(const FText& Message) const
{
	if (!Message.IsEmpty())
	{
		UE_LOG(LogRD, Display, TEXT("FrontendGameMode: %s"), *Message.ToString());
	}
}
