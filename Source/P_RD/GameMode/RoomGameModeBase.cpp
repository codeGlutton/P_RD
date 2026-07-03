#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "UI/RDUserWidget.h"

#include "Setting/RDWorldSettings.h"

DEFINE_LOG_CATEGORY(LogRoomGameMode);

/**
 * @brief 실제 방 GameMode 역할 요약
 *
 * @details
 * ARoomGameModeBase는 "현재 방 UI -> WorldMap -> 다음 방 선택 -> 저장/전환" 흐름을 담당한다.
 * 타이틀 START, 캐릭터 선택, 새 Run 생성, Continue 시작은 AFrontendGameMode 쪽 책임이다.
 *
 * 주요 흐름:
 * - InitializeCommonRoom(): 실제 방에 들어올 때 플레이어 유닛을 복원한다.
 * - RestorePlayerUnit(): PlayerUnitRestorationSubsystem으로 플레이어 유닛을 스폰/등록한다.
 * - BeginRoom(): 방에 들어오면 현재 Run 저장을 시작한다.
 * - SaveRunWithUIAsync(): 방 전환 직후 현재 Run을 저장한다. SaveNotify는 아직 보조 UI 연결 지점으로만 남아 있다.
 * - GetRunControlView(): WorldMap이 표시할 현재 Run 상태 DTO를 만든다.
 * - GetRunControlState(): 구조체 대신 개별 값으로 Run 상태를 받아야 하는 호출부용 호환 API다.
 * - GetMapRoomViews(): 현재 Run의 Stage/Room 데이터를 월드맵 노드 표시용 FMapRoomView 배열로 변환한다.
 * - ResolveRoomState(): 각 방의 Locked/Ready/Selected/Cleared UI 상태를 계산한다.
 * - IsNextRoomFromCurrentPath(): 현재 방에서 지정 방으로 이동 가능한지 row와 column을 모두 검사한다.
 * - IsStageStartPoint(): 지정 좌표가 Stage 시작 지점인지 계산한다.
 * - SelectNextRoom(): 월드맵에서 클릭한 다음 방 후보를 검증하고 선택 좌표로 저장한다.
 * - IsRoomSelectable(): 선택하려는 방이 실제 현재 방의 다음 경로인지 검증한다.
 * - HasSelectedRoom(): 현재 저장된 다음 방 선택 좌표가 있는지 확인한다.
 * - ClearSelectedRoom(): 저장된 다음 방 선택 좌표를 초기화한다.
 * - EnterSelectedRoom(): 이미 선택된 다음 방으로 입장 요청을 시작한다.
 * - PreloadAndTransitionSelectedRoomAsync(): 선택된 방 좌표를 최종 검증하고 프리로드/전환을 요청한다.
 * - AbandonRunFromRoom(): 방 안에서 Run을 포기하고 프론트엔드 방으로 돌아간다.
 * - GetPlayerUnitModel(): 방 안에서 복원된 플레이어 유닛 포인터를 제공한다.
 */
namespace
{
	/**
	 * @brief 지정 좌표가 현재 Stage의 시작 지점인지 계산한다.
	 *
	 * @details
	 * Stage 시작점은 첫 번째 행의 시작 열로 취급한다.
	 * 월드맵에서는 이 값을 이용해 시작 노드를 일반 방과 다르게 표시하고,
	 * 방 진입 직후 지도 화면을 시작 지점 기준으로 맞출지 판단한다.
	 */
	bool IsStageStartPoint(const FStage& Stage, int32 RowIndex, int32 ColumnIndex)
	{
		/*
		 * Stage 시작점은 "첫 번째 행의 시작 열"로 정의된다.
		 * 월드맵 UI에서는 이 노드를 일반 방과 다르게 START 표시로 그려 현재 런의 출발점을 알 수 있게 한다.
		 */
		return RowIndex == 0 && ColumnIndex == Stage.mStartColumn;
	}

	/**
	 * @brief 현재 방에서 바로 다음 행의 지정 방으로 이동할 수 있는지 확인한다.
	 *
	 * 왜 행까지 검사하는가:
	 * mNextRoomColumns는 "다음 행에서 갈 수 있는 열"만 저장한다.
	 * 열만 비교하면 더 먼 행에 같은 열 번호를 가진 방도 선택 가능하다고 오해할 수 있으므로,
	 * 현재 행의 바로 다음 행인지 먼저 확인한 뒤 열 연결을 검사한다.
	 */
	bool IsNextRoomFromCurrentPath(const FStage& Stage, int32 CurrentRowIndex, int32 CurrentColumnIndex, int32 RowIndex, int32 ColumnIndex)
	{
		if (Stage.HasRoom(CurrentRowIndex, CurrentColumnIndex) == false)
		{
			return false;
		}
		if (RowIndex != CurrentRowIndex + 1)
		{
			return false;
		}
		if (Stage.HasRoom(RowIndex, ColumnIndex) == false)
		{
			return false;
		}

		const FRoom& CurrentRoom = Stage.mRoomRows[CurrentRowIndex].mRooms[CurrentColumnIndex].Get<FRoom>();
		return CurrentRoom.mNextRoomColumns.Contains(ColumnIndex);
	}

	/**
	 * @brief 런 데이터의 방 상태를 월드맵 UI가 표시할 상태로 변환한다.
	 *
	 * 왜 여기서 변환하는가:
	 * UI가 FStage/FRoom의 진행 규칙을 직접 해석하면 지도 표시가 런 로직에 강하게 묶인다.
	 * GameMode가 View 상태로 바꿔주면 위젯은 Ready/Selected/Locked를 그리는 일만 맡는다.
	 */
	EMapRoomState ResolveRoomState(const FStage& Stage, const FRoom& Room, int32 CurrentRowIndex, int32 CurrentColumnIndex, int32 SelectedRowIndex, int32 SelectedColumnIndex)
	{
		if (Room.mWasSelected == true)
		{
			return EMapRoomState::Cleared;
		}

		if (IsNextRoomFromCurrentPath(Stage, CurrentRowIndex, CurrentColumnIndex, Room.mRow, Room.mColumn) == true)
		{
			if (Room.mRow == SelectedRowIndex && Room.mColumn == SelectedColumnIndex)
			{
				return EMapRoomState::Selected;
			}
			return EMapRoomState::Ready;
		}

		return EMapRoomState::Locked;
	}

	FText GetRoomTitle(ERoomType RoomType)
	{
		/*
		 * 지도 위젯이 ERoomType을 직접 해석하지 않도록 GameMode가 표시 문구로 변환한다.
		 * 최종 룸 이름/로컬라이징 규칙이 바뀌면 이 함수만 수정하면 된다.
		 */
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
		/*
		 * 현재는 임시 설명으로 행/열과 다음 경로 수를 보여준다.
		 * 최종 룸 설명 DataAsset이 생기면 여기서 설명 텍스트만 교체하고, 지도 DTO 구조는 유지한다.
		 */
		return FText::Format(
			NSLOCTEXT("RoomGameModeBase", "MapRoomDescription", "Row {0}, Column {1}. Next routes: {2}"),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1),
			FText::AsNumber(Room.mNextRoomColumns.Num())
		);
	}

	FText GetStartPointDescription(const FRoom& Room)
	{
		/*
		 * 시작점은 전투 방이 아니므로 룸 타입 설명 대신 앞으로 갈 수 있는 경로 수만 표시한다.
		 */
		return FText::Format(
			NSLOCTEXT("RoomGameModeBase", "StartPointDescription", "Routes: {0}"),
			FText::AsNumber(Room.mNextRoomColumns.Num())
		);
	}
}

ARoomGameModeBase::ARoomGameModeBase()
{
	/*
	 * 월드맵/설정/주사위/스킬 패널은 방 공통 팝업이다. 각 방 HUD에 팝업을 직접 넣지 않고
	 * WorldWidgetSubsystem에 등록해두면 전투/상점/보물 방이 모두 같은 OpenUI/CloseUI 규칙을 공유한다.
	 * (패널을 여는 진입점은 전투 HUD의 내비 버튼 — CombatTileMapHUDWidget_Nav.cpp)
	 */
	mWorldWidgets = { 
		EWorldWidgetType::MsgNotify, 
		EWorldWidgetType::SaveNotify,  
		EWorldWidgetType::FadeInOut,  
		EWorldWidgetType::LoadingNotify,  
		EWorldWidgetType::WorldMap,
		EWorldWidgetType::InGameSettings,
		EWorldWidgetType::DicePanel,
		EWorldWidgetType::SkillPanel,
	};

	/* 월드맵/설정/주사위/스킬 패널은 모든 방에서 같은 팝업으로 쓰이므로 HUD 자식이 아니라 WorldWidgetSubsystem이 준비한다. */
	mShowFadeInUIOnTransition = true;
	mShowFadeOutUIOnTransition = true;
	mShowLoadingNotifyUIOnTransition = true;
	mWaitExternalWorkOnTransition = false;
}

AActor* ARoomGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	AActor* PlayerStartActor = Super::ChoosePlayerStart_Implementation(Player);

	ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	if (WorldSettings != nullptr)
	{
		AActor* SettingPointActor = WorldSettings->GetMainCameraPoint();
		if (SettingPointActor != nullptr)
		{
			PlayerStartActor = SettingPointActor;
		}
	}

	return PlayerStartActor;
}

/**
 * @brief 실제 방 공통 초기화에서 플레이어 유닛을 복원한다.
 *
 * @details
 * RoomGameModeBase 계열 방은 이미 생성된 RunPersistData를 기준으로 실제 플레이 방을 구성한다.
 * 따라서 공통 방 초기화가 끝난 뒤 PlayerUnitRestorationSubsystem을 통해 플레이어 유닛을 스폰/등록한다.
 */
void ARoomGameModeBase::InitializeCommonRoom()
{
	Super::InitializeCommonRoom();

	// 플레이어 복원
	RestorePlayerUnit();
}

/**
 * @brief 실제 방에 들어오면 현재 Run 저장을 시작한다.
 *
 * @details
 * 방 공통 팝업(WorldMap/Settings/Dice/Skill)은 WorldWidgetSubsystem이 준비하고, 여는 것은 HUD 내비 버튼이 담당한다.
 */
void ARoomGameModeBase::BeginRoom()
{
	Super::BeginRoom();

	// 방 전환 즉시 저장
	SaveRunWithUIAsync();

	/* 터치 세팅 */

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	checkf(PlayerController != nullptr, TEXT(""));

#if PLATFORM_DESKTOP
	PlayerController->SetShowMouseCursor(true);
#endif
#if PLATFORM_ANDROID
	PlayerController->ActivateTouchInterface(nullptr);
#endif

	FInputModeGameAndUI InputMode;
	PlayerController->SetInputMode(InputMode);
}

/**
 * @brief 월드맵에서 다음 방 후보를 선택한다.
 *
 * @details
 * 선택 가능한 방인지 GameMode 기준으로 다시 검사한 뒤, ENTER 버튼이 사용할 선택 좌표만 저장한다.
 *
 * 왜 UI 클릭만 믿지 않는가:
 * 지도 UI가 버튼 비활성화를 해도 런 상태는 전환 중이거나 이미 다른 방을 선택한 뒤일 수 있다.
 * GameMode가 최종 선택 가능 여부를 확인해야 런 데이터와 화면 입력이 어긋나도 잘못된 전환을 막을 수 있다.
 */
bool ARoomGameModeBase::SelectNextRoom(int32 RoomRow, int32 RoomColumn)
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	if (IsRoomSelectable(RoomRow, RoomColumn) == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("전환 불가능한 방 선택"));
		return false;
	}

	mSelectedRoomRow = RoomRow;
	mSelectedRoomColumn = RoomColumn;
	return true;
}

/**
 * @brief 현재 선택된 다음 방으로 입장을 요청한다.
 *
 * @details
 * 선택 자체는 SelectNextRoom()에서 처리하고, 이 함수는 이미 선택된 방에 대한 프리로드/전환만 시작한다.
 *
 * 왜 선택과 입장을 나누는가:
 * 지도에서 노드를 눌러 미리 선택해보고, ENTER 버튼으로 확정하는 UX를 만들기 위해서다.
 * 두 단계를 나누면 잘못 눌렀을 때 바로 전환되지 않고, UI가 선택 상태를 먼저 보여줄 수 있다.
 */
bool ARoomGameModeBase::EnterSelectedRoom()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	checkf(PreloadAndTransitionSelectedRoomAsync() == true, TEXT("다음 방으로 전환 실패"));
	return true;
}

/**
 * @brief 방 안에서 활성 Run을 포기하고 프론트엔드 방으로 돌아간다.
 *
 * @details
 * RoomGameModeBase는 실제 방 안에 있는 상태이므로, 런 포기 후에는 RunPersistData를 비우고
 * PreloadAndTransitionFrontendRoomAsync()를 통해 타이틀/캐릭터 선택 흐름이 있는 프론트엔드 방으로 전환한다.
 */
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

bool ARoomGameModeBase::SaveRunAndExitToFrontendAsync()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	// 런을 남긴 채(이어하기 가능) 저장만 하고 나간다. 전환은 저장 완료 후에 시작한다.
	GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->SaveRunAsync(
		FAsyncSaveGameToSlotDelegate::CreateWeakLambda(this, [this](const FString& SlotName, int32 UserIndex, bool IsSuccussed) {
			checkf(IsSuccussed == true, TEXT("저장 후 종료 시점 저장 실패"));
			checkf(PreloadAndTransitionFrontendRoomAsync() == true, TEXT("저장 후 종료 이후, Frontend로 전환 실패"));
		}));

	return true;
}

/**
 * @brief 현재 런의 스테이지 정보를 월드맵 표시용 View 배열로 변환한다.
 *
 * @details
 * FStage/FRoom의 진행 상태를 UI가 바로 읽지 않도록, 화면에 필요한 좌표/상태/설명만 FMapRoomView로 내려준다.
 *
 * 왜 View DTO로 내보내는가:
 * UI가 런 데이터 구조를 직접 알면 Stage 생성 규칙이 바뀔 때마다 위젯 코드도 같이 흔들린다.
 * GameMode가 표시용 데이터로 변환하면 월드맵은 그래프를 그리는 역할에 집중할 수 있다.
 */
bool ARoomGameModeBase::GetMapRoomViews(TArray<FMapRoomView>& OutRooms) const
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
			const EMapRoomState RoomState = ResolveRoomState(
				Stage,
				Room,
				CurrentRowIndex,
				CurrentColumnIndex,
				mSelectedRoomRow,
				mSelectedRoomColumn
			);

			FMapRoomView NewView;
			NewView.mRow = RowIndex;
			NewView.mColumn = ColumnIndex;
			NewView.mType = Room.mType;
			NewView.mState = RoomState;
			NewView.mTitle = bIsStartPoint ? NSLOCTEXT("RoomGameModeBase", "StartPointTitle", "Start") : GetRoomTitle(Room.mType);
			NewView.mDescription = bIsStartPoint ? GetStartPointDescription(Room) : GetRoomDescription(Room);
			NewView.mNextRoomColumns = Room.mNextRoomColumns;
			NewView.mPositionOffsetRate = Room.mPositionOffsetRate;
			NewView.mSelectable = RoomState == EMapRoomState::Ready;
			NewView.mSelected = RoomState == EMapRoomState::Selected;
			NewView.mVisited = RoomState == EMapRoomState::Cleared;
			NewView.mCanEnter = RoomState == EMapRoomState::Selected;
			NewView.mIsStartPoint = bIsStartPoint;
			OutRooms.Add(MoveTemp(NewView));
		}
	}

	return OutRooms.IsEmpty() == false;
}

/**
 * @brief WorldMap이 표시할 현재 Run 상태 DTO를 만든다.
 *
 * @details
 * 위젯이 URunPersistData를 직접 읽지 않도록,
 * 현재 row/column, 플레이어 레벨, 난이도, Stage 시작 지점 여부, 포기 가능 여부를 FRunControlView로 모아 내려준다.
 * 즉 UI는 이 값만 보고 제목/요약/버튼 상태를 표시하고, 실제 Run 규칙 해석은 GameMode에 남긴다.
 */
bool ARoomGameModeBase::GetRunControlView(FRunControlView& OutView) const
{
	/*
	 * FrontendMapWidget이 현재 런 요약을 표시할 때 쓰는 UI DTO다.
	 * 위젯이 URunPersistData를 직접 읽지 않게 하고, GameMode가 "화면에 보여줄 값"만 골라 내려준다.
	 */
	OutView = FRunControlView();
	OutView.mHasActiveRun = HasActiveRun();
	OutView.mCanSaveRun = false;
	OutView.mCanAbandonRun = CanAbandonRun();

	if (OutView.mHasActiveRun == false)
	{
		return false;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	RunPersistData->GetCurrentRoomIndex(OUT OutView.mRow, OUT OutView.mColumn);
	OutView.mIsAtStageStart = IsStageStartPoint(RunPersistData->GetStage(), OutView.mRow, OutView.mColumn);
	OutView.mPlayerLevel = RunPersistData->GetPlayerLevel();
	OutView.mDifficulty = RunPersistData->GetDifficulty();
	return true;
}

/**
 * @brief 구조체 대신 개별 값으로 현재 Run 상태를 받아야 하는 호출부용 호환 API다.
 *
 * @details
 * 표시 데이터 계산 기준은 GetRunControlView() 하나로 유지한다.
 * 오래된 BP/WBP 또는 구조체를 바로 쓰기 어려운 호출부가 있으면 이 함수가 FRunControlView를 풀어서 전달한다.
 */
bool ARoomGameModeBase::GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const
{
	/*
	 * BP/WBP나 기존 호출부가 구조체 대신 개별 값으로 현재 런 상태를 받아야 할 때 쓰는 호환 API다.
	 * 내부 기준은 GetRunControlView() 하나로 유지해, 표시 데이터 계산이 두 군데로 갈라지지 않게 한다.
	 */
	FRunControlView RunView;
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

/**
 * @brief 선택된 다음 방 좌표를 실제 프리로드/전환 요청으로 확정한다.
 *
 * @details
 * 월드맵에서 저장된 선택 좌표가 아직 유효한지 다시 확인한 뒤,
 * 기존 방 전환 공통 흐름인 PreloadAndTransitionRoomAsync(row, column)으로 넘긴다.
 * 선택 시점과 입장 확정 시점 사이에 Run 상태가 바뀔 수 있으므로 여기서 최종 검증을 한 번 더 수행한다.
 */
bool ARoomGameModeBase::PreloadAndTransitionSelectedRoomAsync()
{
	/*
	 * 월드맵에서 선택한 방을 실제 전환 요청으로 바꾸는 마지막 단계다.
	 * SelectNextRoom()으로 저장된 좌표가 여전히 유효한지 다시 확인한 뒤,
	 * 기존 방 전환 시스템의 PreloadAndTransitionRoomAsync(row, column) 흐름으로 넘긴다.
	 */
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	if (HasSelectedRoom() == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 인덱스 미선택"));
		return false;
	}

	if (IsRoomSelectable(mSelectedRoomRow, mSelectedRoomColumn) == false)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("전환 불가능한 방 선택"));
		return false;
	}

	checkf(PreloadAndTransitionRoomAsync(mSelectedRoomRow, mSelectedRoomColumn) == true, TEXT("선택된 방에 대한 Preload 및 Auto Transition 실패"));
	return true;
}

/**
 * @brief 방 진입 직후 현재 Run을 저장하고, 추후 SaveNotify UI와 연결될 저장 흐름을 담당한다.
 *
 * @details
 * 저장 자체는 SaveGameSubsystem->SaveRunAsync()가 수행한다.
 * SaveNotify는 저장 진행/완료를 보여주기 위한 보조 UI라 현재 OpenUI()/CloseUI() 호출은 비활성화되어 있고,
 * 저장 성공 여부 검증은 비동기 저장 완료 콜백에서 처리한다.
 */
void ARoomGameModeBase::SaveRunWithUIAsync() const
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr 오류"));

	UUserWidget* SaveNotifyWidget = WorldWidgetSubsystem->GetWorldWidget(EWorldWidgetType::SaveNotify);

	/*
	 * SaveNotify는 저장 성공/실패 자체의 필수 조건이 아니라, 저장 진행을 보여주는 보조 UI다.
	 * 현재 알림 표시 애니메이션 호출이 비활성화되어 있으므로 위젯 설정이 빠져도 방 전환 저장은 계속 진행한다.
	 * 알림 UI를 실제로 다시 열고 닫는 시점에는 OpenUI()/CloseUI() 흐름과 함께 필수 바인딩 검사를 되살린다.
	 */
    // TODO: 구현되면 주석 풀기
	// if (SaveNotifyWidget) SaveNotifyWidget->OpenUI();
	// 시작 애니메이션
	GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->SaveRunAsync(FAsyncSaveGameToSlotDelegate::CreateLambda([SaveNotifyWidget](const FString& SlotName, int32 UserIndex, bool IsSuccussed) {
		checkf(IsSuccussed == true, TEXT("방 전환 시점 저장 실패"));
		// TODO: 구현되면 주석 풀기
		// if (SaveNotifyWidget) SaveNotifyWidget->CloseUI();
		// 종료 애니메이션
		}));
}

/**
 * @brief RunPersistData 기준으로 방 안에서 사용할 플레이어 유닛을 스폰/등록한다.
 *
 * @details
 * 실제 스폰과 데이터 복원은 PlayerUnitRestorationSubsystem이 담당한다.
 * RoomGameModeBase는 복원된 유닛 포인터를 mPlayerUnit에 보관해 방 내부 시스템이 현재 플레이어 유닛을 조회할 수 있게 한다.
 */
void ARoomGameModeBase::RestorePlayerUnit()
{
	UPlayerUnitRestorationSubsystem* PlayerUnitRestorationSubsystem = GetGameInstance()->GetSubsystem<UPlayerUnitRestorationSubsystem>();
	checkf(PlayerUnitRestorationSubsystem != nullptr, TEXT("플레이어 유닛 복원 서브시스템 nullptr 오류"));

	UPlayerUnitModel* PlayerUnit = PlayerUnitRestorationSubsystem->SpawnPlayerUnit(GetWorld());
	checkf(PlayerUnit != nullptr, TEXT("플레이어 유닛 스폰 오류"));

	PlayerUnitRestorationSubsystem->RegisterPlayerUnit(PlayerUnit);
	mPlayerUnit = PlayerUnit;
}

/**
 * @brief 월드맵에서 저장해둔 다음 방 선택 좌표를 초기화한다.
 *
 * @details
 * 선택 좌표는 "노드를 눌러 임시 선택한 값"이므로,
 * 선택 취소나 지도 상태 재계산이 필요한 시점에는 INDEX_NONE으로 되돌려 선택된 방이 없음을 명확히 한다.
 */
void ARoomGameModeBase::ClearSelectedRoom()
{
	mSelectedRoomRow = INDEX_NONE;
	mSelectedRoomColumn = INDEX_NONE;
}

/**
 * @brief 월드맵에서 확정 대기 중인 다음 방 선택 좌표가 있는지 확인한다.
 */
bool ARoomGameModeBase::HasSelectedRoom() const
{
	return mSelectedRoomRow != INDEX_NONE && mSelectedRoomColumn != INDEX_NONE;
}

/**
 * @brief 지정 방이 현재 방에서 실제로 이동 가능한 다음 방인지 검증한다.
 *
 * @details
 * 방 좌표가 Stage 안에 존재하는지 먼저 확인하고,
 * 현재 방의 mNextRoomColumns 기준으로 바로 다음 행에 연결된 방인지 검사한다.
 * UI에서 Ready 상태로 보이는 방이라도 전환 직전 GameMode가 다시 확인해 잘못된 방 입장을 막는다.
 */
bool ARoomGameModeBase::IsRoomSelectable(int32 RoomRow, int32 RoomColumn) const
{
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

/**
 * @brief 방 안에서 복원된 현재 플레이어 유닛 포인터를 제공한다.
 *
 * @details
 * InitializeCommonRoom()에서 RestorePlayerUnit()이 성공하면 mPlayerUnit에 등록된다.
 * 전투/상점/보상 처리처럼 현재 플레이어 유닛이 필요한 시스템은 이 접근자를 통해 GameMode가 보관한 유닛을 조회한다.
 */
UPlayerUnitModel* ARoomGameModeBase::GetPlayerUnitModel() const
{
	return mPlayerUnit.Get();
}
