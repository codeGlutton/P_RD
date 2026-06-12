#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PlayerUnitRestorationSubsystem.h"
#include "Pawn/Player/PlayerUnit.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "UI/RDUserWidget.h"

DEFINE_LOG_CATEGORY(LogRoomGameMode);

namespace
{
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
	 * 실제 방에서는 TopMenuBar가 월드맵/설정/주사위/스킬 패널을 여는 공통 진입점이다.
	 * 각 방 HUD에 팝업을 직접 넣지 않고 WorldWidgetSubsystem에 등록해두면,
	 * 전투/상점/보물 방이 모두 같은 OpenUI/CloseUI 규칙을 공유한다.
	 */
	mWorldWidgets = { 
		EWorldWidgetType::TopMenuBar, 
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

void ARoomGameModeBase::InitializeCommonRoom()
{
	Super::InitializeCommonRoom();

	// 플레이어 복원
	RestorePlayerUnit();
}

void ARoomGameModeBase::BeginRoom()
{
	Super::BeginRoom();

	/**
	 * @brief 실제 방에 진입하면 상단 메뉴를 공통 OpenUI 흐름으로 표시한다.
	 *
	 * @details
	 * TopMenuBar는 월드맵/설정 패널을 여는 인게임 공통 진입점이다.
	 * 각 방 GameMode가 별도 HUD를 직접 만들지 않고 WorldWidgetSubsystem에 등록된 위젯을 열어
	 * 전투/상점/보물 방에서 같은 UI 경로를 공유하게 한다.
	 *
	 * 왜 BeginRoom()에서 여는가:
	 * InitWorldWidget()은 위젯을 준비만 하고, 실제 표시 여부는 방 시작 흐름이 결정해야 한다.
	 * 방 전용 초기화가 끝난 뒤 OpenUI()를 호출해야 탑바가 현재 방 정보를 읽고 올바른 상태로 보인다.
	 */
	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (URDUserWidget* TopMenuBar = WorldWidgetSubsystem->GetWorldWidget<URDUserWidget>(EWorldWidgetType::TopMenuBar))
		{
			TopMenuBar->OpenUI();
		}
	}

	// 방 전환 즉시 저장
	SaveRunWithUIAsync();

	/* 터치 세팅 */

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	checkf(PlayerController != nullptr, TEXT(""));

	PlayerController->ActivateTouchInterface(nullptr);

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

/**
 * @brief 현재 런의 스테이지 정보를 월드맵 표시용 View 배열로 변환한다.
 *
 * @details
 * FStage/FRoom의 진행 상태를 UI가 바로 읽지 않도록, 화면에 필요한 좌표/상태/설명만 FFrontendMapRoomView로 내려준다.
 *
 * 왜 View DTO로 내보내는가:
 * UI가 런 데이터 구조를 직접 알면 Stage 생성 규칙이 바뀔 때마다 위젯 코드도 같이 흔들린다.
 * GameMode가 표시용 데이터로 변환하면 월드맵은 그래프를 그리는 역할에 집중할 수 있다.
 */
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
			NewView.mSelectable = RoomState == EFrontendMapRoomState::Ready;
			NewView.mSelected = RoomState == EFrontendMapRoomState::Selected;
			NewView.mVisited = RoomState == EFrontendMapRoomState::Cleared;
			NewView.mCanEnter = RoomState == EFrontendMapRoomState::Selected;
			NewView.mIsStartPoint = bIsStartPoint;
			OutRooms.Add(MoveTemp(NewView));
		}
	}

	return OutRooms.IsEmpty() == false;
}

bool ARoomGameModeBase::GetRunControlView(FFrontendRunControlView& OutView) const
{
	/*
	 * TopMenuBar와 FrontendMapWidget이 현재 런 요약을 표시할 때 쓰는 UI DTO다.
	 * 위젯이 URunPersistData를 직접 읽지 않게 하고, GameMode가 "화면에 보여줄 값"만 골라 내려준다.
	 */
	OutView = FFrontendRunControlView();
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

bool ARoomGameModeBase::GetRunControlState(OUT int32& RowIndex, OUT int32& ColumnIndex, OUT int32& PlayerLevel, OUT int32& Difficulty) const
{
	/*
	 * BP/WBP나 기존 호출부가 구조체 대신 개별 값으로 현재 런 상태를 받아야 할 때 쓰는 호환 API다.
	 * 내부 기준은 GetRunControlView() 하나로 유지해, 표시 데이터 계산이 두 군데로 갈라지지 않게 한다.
	 */
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
