#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"
#include "Pawn/Player/PlayerUnit.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogRoomGameMode);

namespace
{
	bool IsStageStartPoint(const FStage& Stage, int32 RowIndex, int32 ColumnIndex)
	{
		return RowIndex == 0 && ColumnIndex == Stage.mStartColumn;
	}

	bool IsNextRoomFromCurrentPath(const FStage& Stage, int32 CurrentRowIndex, int32 CurrentColumnIndex, int32 RowIndex, int32 ColumnIndex)
	{
		if (Stage.HasRoom(CurrentRowIndex, CurrentColumnIndex) == false)
		{
			return false;
		}

		const FRoom& CurrentRoom = Stage.mRoomRows[CurrentRowIndex].mRooms[CurrentColumnIndex].Get<FRoom>();
		return CurrentRoom.mNextRoomColumns.Contains(ColumnIndex);
	}

	EFrontendMapRoomState ResolveRoomState(const FStage& Stage, const FRoom& Room, int32 CurrentRowIndex, int32 CurrentColumnIndex, int32 SelectedRowIndex, int32 SelectedColumnIndex)
	{
		if (Room.mWasSelected == true)
		{
			return EFrontendMapRoomState::Cleared;
		}

		if (IsNextRoomFromCurrentPath(Stage, CurrentRowIndex, CurrentColumnIndex, Room.mRow, Room.mColumn) == true)
		{
			if (Room.mRow == SelectedRowIndex && Room.mColumn == SelectedColumnIndex)
			{
				return EFrontendMapRoomState::Selected;
			}
			return EFrontendMapRoomState::Ready;
		}

		return EFrontendMapRoomState::Locked;
	}

	FText GetRoomTitle(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Monster:
			return NSLOCTEXT("RoomGameModeBase", "MonsterRoomTitle", "Monster");
		case ERoomType::EliteMonster:
			return NSLOCTEXT("RoomGameModeBase", "EliteRoomTitle", "Elite");
		case ERoomType::BossMonster:
			return NSLOCTEXT("RoomGameModeBase", "BossRoomTitle", "Boss");
		case ERoomType::Shop:
			return NSLOCTEXT("RoomGameModeBase", "ShopRoomTitle", "Shop");
		case ERoomType::Treasure:
			return NSLOCTEXT("RoomGameModeBase", "TreasureRoomTitle", "Treasure");
		default:
			return NSLOCTEXT("RoomGameModeBase", "UnknownRoomTitle", "Unknown");
		}
	}

	FText GetRoomDescription(const FRoom& Room)
	{
		return FText::Format(
			NSLOCTEXT("RoomGameModeBase", "MapRoomDescription", "Row {0}, Column {1}. Next routes: {2}"),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1),
			FText::AsNumber(Room.mNextRoomColumns.Num())
		);
	}

	FText GetStartPointDescription(const FRoom& Room)
	{
		return FText::Format(
			NSLOCTEXT("RoomGameModeBase", "StartPointDescription", "Routes: {0}"),
			FText::AsNumber(Room.mNextRoomColumns.Num())
		);
	}
}

ARoomGameModeBase::ARoomGameModeBase()
{
	mWorldWidgets = { 
		EWorldWidgetType::TopMenuBar, 
		EWorldWidgetType::MsgNotify, 
		EWorldWidgetType::SaveNotify,  
		EWorldWidgetType::FadeInOut,  
		EWorldWidgetType::LoadingNotify,  
	};

	mShowFadeInUIOnTransition = true;
	mShowFadeOutUIOnTransition = true;
	mShowLoadingNotifyUIOnTransition = true;
	mWaitExternalWorkOnTransition = false;
}

void ARoomGameModeBase::InitializeCommonRoom()
{
	Super::InitializeCommonRoom();

	// 플레이어 복원
	RestorePlayerUnit();
}

void ARoomGameModeBase::BeginRoom()
{
	Super::BeginRoom();

	// 방 전환 즉시 저장
	SaveRunWithUIAsync();

	/* 터치 세팅 */

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	checkf(PlayerController != nullptr, TEXT(""));

	PlayerController->ActivateTouchInterface(nullptr);

	FInputModeGameAndUI InputMode;
	PlayerController->SetInputMode(InputMode);
}

bool ARoomGameModeBase::SelectNextRoom(int32 RoomRow, int32 RoomColumn)
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	if (IsRoomSelectable(RoomRow, RoomColumn) == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("전환 불가능한 방 선택"));
		return false;
	}

	mSelectedRoomRow = RoomRow;
	mSelectedRoomColumn = RoomColumn;
	return true;
}

bool ARoomGameModeBase::EnterSelectedRoom()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	checkf(PreloadAndTransitionSelectedRoomAsync() == true, TEXT("다음 방으로 전환 실패"));
	return false;
}

bool ARoomGameModeBase::AbandonRunFromRoom()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	ClearRunPersistData();
	checkf(PreloadAndTransitionFrontendRoomAsync() == true, TEXT("게임 포기 이후, Frontend로 전환 실패"));

	return true;
}

bool ARoomGameModeBase::GetMapRoomViews(TArray<FFrontendMapRoomView>& OutRooms) const
{
	OutRooms.Reset();

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || RunPersistData->IsActive() == false)
	{
		UE_LOG(LogRD, Warning, TEXT("런 데이터 미존재로 Map Room View 표기 불가"));

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
			if (Stage.HasRoom(RowIndex, ColumnIndex) == false)
			{
				continue;
			}

			const FRoom& Room = RoomRow.mRooms[ColumnIndex].Get<FRoom>();

			const bool bIsStartPoint = IsStageStartPoint(Stage, RowIndex, ColumnIndex);
			const EFrontendMapRoomState RoomState = ResolveRoomState(
				Stage,
				Room,
				CurrentRowIndex,
				CurrentColumnIndex,
				mSelectedRoomRow,
				mSelectedRoomColumn
			);

			FFrontendMapRoomView NewView;
			NewView.mRow = RowIndex;
			NewView.mColumn = ColumnIndex;
			NewView.mType = Room.mType;
			NewView.mState = RoomState;
			NewView.mTitle = bIsStartPoint ? NSLOCTEXT("RoomGameModeBase", "StartPointTitle", "Start") : GetRoomTitle(Room.mType);
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

	return OutRooms.IsEmpty() == false;
}

bool ARoomGameModeBase::GetRunControlView(FFrontendRunControlView& OutView) const
{
	OutView = FFrontendRunControlView();
	OutView.bHasActiveRun = HasActiveRun();
	OutView.bCanSaveRun = false;
	OutView.bCanAbandonRun = CanAbandonRun();

	if (OutView.bHasActiveRun == false)
	{
		return false;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	RunPersistData->GetCurrentRoomIndex(OUT OutView.mRow, OUT OutView.mColumn);
	OutView.bIsAtStageStart = IsStageStartPoint(RunPersistData->GetStage(), OutView.mRow, OutView.mColumn);
	OutView.mPlayerLevel = RunPersistData->GetPlayerLevel();
	OutView.mDifficulty = RunPersistData->GetDifficulty();
	return true;
}

bool ARoomGameModeBase::GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const
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

bool ARoomGameModeBase::PreloadAndTransitionSelectedRoomAsync()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	if (IsRoomSelectable(mSelectedRoomRow, mSelectedRoomColumn) == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("전환 불가능한 방 선택"));
		return false;
	}

	checkf(PreloadAndTransitionRoomAsync(mSelectedRoomRow, mSelectedRoomColumn) == true, TEXT("선택된 방에 대한 Preload 및 Auto Transition 실패"));
	return true;
}

void ARoomGameModeBase::SaveRunWithUIAsync() const
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	UUserWidget* SaveNotifyWidget = WorldWidgetSubsystem->GetWorldWidget(EWorldWidgetType::SaveNotify);
	checkf(SaveNotifyWidget != nullptr, TEXT("세이브 알림 위젯 nullptr 오류"));

	//SaveNotifyWidget->OpenUI();
	// 시작 애니메이션
	GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->SaveRunAsync(FAsyncSaveGameToSlotDelegate::CreateLambda([SaveNotifyWidget](const FString& SlotName, int32 UserIndex, bool IsSuccussed) {
		checkf(IsSuccussed == true, TEXT("방 전환 시점 저장 실패"));
		//SaveNotifyWidget->CloseUI();
		// 종료 애니메이션
		}));
}

void ARoomGameModeBase::RestorePlayerUnit()
{
	UPlayerUnitRestorationSubsystem* PlayerUnitRestorationSubsystem = GetGameInstance()->GetSubsystem<UPlayerUnitRestorationSubsystem>();
	checkf(PlayerUnitRestorationSubsystem != nullptr, TEXT("플레이어 유닛 복원 서브시스템 nullptr 오류"));

	APlayerUnit* PlayerUnit = PlayerUnitRestorationSubsystem->SpawnPlayerUnit(GetWorld());
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	PlayerUnitRestorationSubsystem->RegisterPlayerUnit(PlayerUnit);
	mPlayerUnit = PlayerUnit;
}

void ARoomGameModeBase::ClearSelectedRoom()
{
	mSelectedRoomRow = INDEX_NONE;
	mSelectedRoomColumn = INDEX_NONE;
}

bool ARoomGameModeBase::HasSelectedRoom() const
{
	return mSelectedRoomRow != INDEX_NONE && mSelectedRoomColumn != INDEX_NONE;
}

bool ARoomGameModeBase::IsRoomSelectable(int32 RoomRow, int32 RoomColumn) const
{
	if (HasSelectedRoom() == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 인덱스 미선택"));
		return false;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	const FStage& Stage = RunPersistData->GetStage();
	if (Stage.HasRoom(RoomRow, RoomColumn) == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("잘못된 방 선택"));
		return false;
	}

	int32 CurrentRoomRow = 0;
	int32 CurrentRoomColumn = 0;
	RunPersistData->GetCurrentRoomIndex(OUT CurrentRoomRow, OUT CurrentRoomColumn);
	if (IsNextRoomFromCurrentPath(Stage, CurrentRoomRow, CurrentRoomColumn, RoomRow, RoomColumn) == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("도달할 수 없는 방 선택"));
		return false;
	}

	return true;
}

AUnit* ARoomGameModeBase::GetPlayerUnit() const
{
	return mPlayerUnit.Get();
}
