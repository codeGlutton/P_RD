#include "GameMode/RoomGameModeBase.h"
#include "Engine/GameInstance.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/RoomTransitionSubsystem.h"
#include "Singleton/InstanceSubsystem/PartyRestorationSubsystem.h"
#include "Actor/Party/PartyModel.h"
#include "Pawn/Player/PlayerUnitModel.h"

#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "UI/RDUserWidget.h"

#include "Setting/RDWorldSettings.h"

#include "Engine/AssetManager.h"
#include "Engine/Texture2D.h"
#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "DataAsset/ArtifactData/StaticArtifactData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "AttributeSet/PartyAttributeSet.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Component/ArtifactComponent/PartyArtifactComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailUIBuilder.h"

#include "TAS/Effect/Stat/TacticalEffect_HP.h"

DEFINE_LOG_CATEGORY(LogRoomGameMode);

/**
 * @brief 실제 방 GameMode 역할 요약
 *
 * @details
 * ARoomGameModeBase는 "현재 방 UI -> WorldMap -> 다음 방 선택 -> 저장/전환" 흐름을 담당한다.
 * 타이틀 START, 캐릭터 선택, 새 Run 생성, Continue 시작은 AFrontendGameMode 쪽 책임이다.
 */
namespace
{
	constexpr const TCHAR* RoomArtifactFallbackIconPath =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice");

	UTexture2D* ResolveRoomArtifactInventoryIcon(
		const UStaticArtifactData* Artifact)
	{
		if (Artifact != nullptr)
		{
			if (UTexture2D* Icon = Artifact->mIcon.LoadSynchronous())
			{
				return Icon;
			}
			UE_LOG(LogRoomGameMode, Verbose,
				TEXT("아티팩트 아이콘 미설정, 기본 아이콘 사용: %s"),
				*Artifact->GetPathName());
		}
		return LoadObject<UTexture2D>(nullptr, RoomArtifactFallbackIconPath);
	}

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

	FLinearColor GetInventoryRarityColor(ERarityType RarityType)
	{
		switch (RarityType)
		{
		case ERarityType::Rare:
			return FLinearColor(0.42f, 0.66f, 0.95f, 1.f);
		case ERarityType::Epic:
			return FLinearColor(0.72f, 0.46f, 0.92f, 1.f);
		case ERarityType::Common:
		default:
			return FLinearColor(0.72f, 0.78f, 0.75f, 1.f);
		}
	}

	/**
	 * @brief 용병 패널의 직업 표시명. RunOptionsRail 위젯에 있던 표를 생산자로 옮겼다.
	 * @details 위젯이 직업 enum 을 해석하면 표시 규칙이 화면마다 갈라진다 (PR #426 규칙).
	 */
	FText GetRosterJobName(const UPlayerUnitModel* Unit)
	{
		if (Unit == nullptr)
		{
			return FText::GetEmpty();
		}
		switch (Unit->GetUnitJobType())
		{
		case EUnitJobType::Knight: return NSLOCTEXT("RoomGameModeBase", "RosterJobKnight", "기사");
		case EUnitJobType::Mage: return NSLOCTEXT("RoomGameModeBase", "RosterJobMage", "마법사");
		case EUnitJobType::Ranger: return NSLOCTEXT("RoomGameModeBase", "RosterJobRanger", "궁수");
		case EUnitJobType::Rogue: return NSLOCTEXT("RoomGameModeBase", "RosterJobRogue", "도적");
		case EUnitJobType::Barbarian: return NSLOCTEXT("RoomGameModeBase", "RosterJobBarbarian", "야만전사");
		case EUnitJobType::Druid: return NSLOCTEXT("RoomGameModeBase", "RosterJobDruid", "드루이드");
		default: return Unit->GetBoardActorDisplayName();
		}
	}

	/**
	 * @brief 용병 패널 초상 해석. 직업별 T_MB_HireIcon_* 우선, 없으면 보드 아이콘/초상화.
	 * @details RunOptionsRail 위젯의 해석 규칙을 그대로 생산자로 옮긴 것 -- 화면은 결과만 받는다.
	 */
	UTexture2D* GetRosterHeadPortrait(const UPlayerUnitModel* Unit)
	{
		if (Unit == nullptr)
		{
			return nullptr;
		}
		const TCHAR* Stem = nullptr;
		switch (Unit->GetUnitJobType())
		{
		case EUnitJobType::Knight: Stem = TEXT("Knight"); break;
		case EUnitJobType::Mage: Stem = TEXT("Mage"); break;
		case EUnitJobType::Ranger: Stem = TEXT("Ranger"); break;
		case EUnitJobType::Rogue: Stem = TEXT("Rogue"); break;
		case EUnitJobType::Barbarian: Stem = TEXT("Barbarian"); break;
		case EUnitJobType::Druid: Stem = TEXT("Druid"); break;
		default: break;
		}
		if (Stem != nullptr)
		{
			const FString AssetName = FString::Printf(TEXT("T_MB_HireIcon_%s"), Stem);
			const FString AssetPath = FString::Printf(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/%s.%s"),
				*AssetName, *AssetName);
			if (UTexture2D* Portrait = LoadObject<UTexture2D>(nullptr, *AssetPath))
			{
				return Portrait;
			}
		}
		return Unit->GetBoardActorIcon() != nullptr
			? Unit->GetBoardActorIcon() : Unit->GetBoardActorPortrait();
	}

}

ARoomGameModeBase::ARoomGameModeBase()
{
	/*
	 * 월드맵/설정/스킬 패널은 방 공통 팝업이다. 각 방 HUD에 팝업을 직접 넣지 않고
	 * WorldWidgetSubsystem에 등록해두면 전투/상점/보물 방이 모두 같은 OpenUI/CloseUI 규칙을 공유한다.
	 * (전투 HUD의 내비 버튼이 진입점이었는데, 전투 중에 무엇을 여는지가 안 정해져
	 *  옛 HUD와 함께 지웠다. 정해지면 새 HUD에 붙인다.)
	 */
	mWorldWidgets = {
		EWorldWidgetType::FadeInOut,
		EWorldWidgetType::LoadingNotify,
		EWorldWidgetType::WorldMap,
		EWorldWidgetType::InGameSettings,
	};

	/* 월드맵/설정/스킬 패널은 모든 방에서 같은 팝업으로 쓰이므로 HUD 자식이 아니라 WorldWidgetSubsystem이 준비한다. */
	mShowFadeInUIOnTransition = true;
	mShowFadeOutUIOnTransition = true;
	mShowLoadingNotifyUIOnTransition = true;
	mWaitExternalWorkOnTransition = false;
}

void ARoomGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FRoom& CurRoom = GetRunPersistData()->GetCurrentRoom();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	UStaticRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticRoomSpawnData>(CurRoom.mStaticRoomSpawnDataId);
	checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));
	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RD 월드 세팅 nullptr"));

	/* BGM 세팅 */

	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = StaticRoomData->mOverrideBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous());

	/* 방 세팅 */

	if (StaticRoomData->mUseRandomSpawnSetting == true)
	{
		FName SelectedRoomSpawnName = WorldSettings->GetRandomRoomSpawnSettingName(GetRunPersistData()->GetStageBuildStream());
		SetRoomSpawnSettingName(SelectedRoomSpawnName);
	}
	else
	{
		SetRoomSpawnSettingName(StaticRoomData->mDefaultSpawnSettingName);
	}
}

AActor* ARoomGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	AActor* PlayerStartActor = Super::ChoosePlayerStart_Implementation(Player);

	const ARDWorldSettings* WorldSettings = Cast<ARDWorldSettings>(GetWorld()->GetWorldSettings());
	checkf(WorldSettings != nullptr, TEXT("RD 월드 세팅 nullptr"));

	AActor* SettingPointActor = WorldSettings->GetMainCameraPoint(mSelectedRoomSpawnSettingName);
	if (SettingPointActor != nullptr)
	{
		PlayerStartActor = SettingPointActor;
	}

	return PlayerStartActor;
}

APawn* ARoomGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	FRotator StartRotation = StartSpot->GetActorRotation();
	FVector StartLocation = StartSpot->GetActorLocation();

	FTransform Transform = FTransform(StartRotation, StartLocation);
	return SpawnDefaultPawnAtTransform(NewPlayer, Transform);
}

void ARoomGameModeBase::InitializeCommonRoom()
{
	Super::InitializeCommonRoom();

	// 플레이어 복원
	RestorePlayerUnit();
	// 방 전환 즉시 저장
	SaveRunWithUIAsync();

	// 카메라를 갱신하여 ProjMat을 갱신해줍니다.
	GetWorld()->GetFirstPlayerController()->PlayerCameraManager->UpdateCamera(0.0f);
}

void ARoomGameModeBase::BeginRoom()
{
	Super::BeginRoom();

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

bool ARoomGameModeBase::EnterSelectedRoom()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	const bool IsTransitionStarted = PreloadAndTransitionSelectedRoomAsync();
	checkf(IsTransitionStarted == true, TEXT("다음 방으로 전환 실패"));
	return IsTransitionStarted;
}

bool ARoomGameModeBase::EnterNextStage()
{
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	ApplyStageClearHeal();

	const URunPersistData* RunPersistData = GetRunPersistData();
	const FStage& Stage = RunPersistData->GetStage();

	EStageLevelType CurStageLevel = Stage.mStageLevel;
	EStageLevelType NextStageLevel = StaticCast<EStageLevelType>(StaticCast<uint8>(CurStageLevel) + 1);
	if (CurStageLevel == EStageLevelType::Stage3)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("게임 클리어로 다음 스테이지 접근 불가"));
		return false;
	}

	const bool IsTransitionStarted = PreloadAndTransitionRoomAsync(NextStageLevel);
	checkf(IsTransitionStarted == true, TEXT("스테이지 처음 방으로 전환 실패"));
	return IsTransitionStarted;
}

bool ARoomGameModeBase::AbandonRunFromRoom()
{
	if (mSaveAndExitPending || mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	ClearRunPersistData();
	const bool IsTransitionStarted = PreloadAndTransitionFrontendRoomAsync();
	checkf(IsTransitionStarted == true, TEXT("게임 포기 이후, Frontend로 전환 실패"));

	return IsTransitionStarted;
}

void ARoomGameModeBase::SaveAndExitRunFromRoomAsync(
	FOnRoomSaveAndExitComplete Completion)
{
	if (mSaveAndExitPending || mWasNextRoomPreloadRequested || !HasActiveRun())
	{
		Completion.ExecuteIfBound(false);
		return;
	}

	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<USaveGameSubsystem>() : nullptr;
	if (SaveGameSubsystem == nullptr)
	{
		Completion.ExecuteIfBound(false);
		return;
	}

	mSaveAndExitPending = true;
	SaveGameSubsystem->SaveRunAsync(FAsyncSaveGameToSlotDelegate::CreateWeakLambda(
		this,
		[this, MovedCompletion = MoveTemp(Completion)](
			const FString& SlotName, int32 UserIndex, bool bSaveSucceeded) mutable
		{
			const bool bTransitionStarted = bSaveSucceeded
				&& PreloadAndTransitionFrontendRoomAsync();
			if (!bTransitionStarted)
			{
				mSaveAndExitPending = false;
			}
			MovedCompletion.ExecuteIfBound(bTransitionStarted);
		}));
}

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
	// OutView.mPlayerLevel = RunPersistData->GetPlayerLevel();
	OutView.mDifficulty = RunPersistData->GetDifficulty();
	return true;
}

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

FText ARoomGameModeBase::GetCurrentRoomDisplayName() const
{
	const URunPersistData* RunPersistData = GetRunPersistData();
	return RunPersistData != nullptr ? RunPersistData->GetCurrentRoom().GetDisplayName() : FText::GetEmpty();
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

	const bool IsTransitionStarted = PreloadAndTransitionRoomAsync(mSelectedRoomRow, mSelectedRoomColumn);
	checkf(IsTransitionStarted == true, TEXT("선택된 방에 대한 Preload 및 Auto Transition 실패"));
	return IsTransitionStarted;
}

void ARoomGameModeBase::SaveRunWithUIAsync() const
{
	GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->SaveRunAsync(FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, int32 UserIndex, bool IsSuccussed) {
		checkf(IsSuccussed == true, TEXT("방 전환 시점 저장 실패"));
		}));
}

void ARoomGameModeBase::ApplyStageClearHeal() const
{
	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		return;
	}

	TArray<UAttributeSetComponentModel*> AttributeSetCompModels;
	for (const TObjectPtr<UPlayerUnitModel>& PlayerUnitModel : PartyModel->GetPlayerUnitModels())
	{
		UPlayerUnitModel* UnitModel = PlayerUnitModel.Get();
		if (UnitModel == nullptr)
		{
			continue;
		}

		UAttributeSetComponentModel* Attributes = UnitModel->GetAttributeComponentModel();
		if (Attributes == nullptr)
		{
			return;
		}

		AttributeSetCompModels.Add(Attributes);
	}

	for (UAttributeSetComponentModel* AttributeSetCompModel : AttributeSetCompModels)
	{
		UTacticalEffectContext* Context = AttributeSetCompModel->MakeEffectContext();
		TSharedPtr<FTacticalEffectSpec> Spec = AttributeSetCompModel->MakeOutgoingSpec(UTacticalEffect_StageClearHeal::StaticClass(), Context);

		AttributeSetCompModel->ApplyTacticalEffectSpecToSelf(*Spec);
	}
}

void ARoomGameModeBase::RestorePlayerUnit()
{
	UPartyRestorationSubsystem* PartyRestorationSubsystem = GetGameInstance()->GetSubsystem<UPartyRestorationSubsystem>();
	checkf(PartyRestorationSubsystem != nullptr, TEXT("플레이어 유닛 복원 서브시스템 nullptr 오류"));

	UPartyModel* PartyModel = PartyRestorationSubsystem->RestorePartyFromPersistData(GetWorld());
	checkf(PartyModel != nullptr, TEXT("파티 모델 복원 오류"));

	mPartyModel = PartyModel;
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

UPartyModel* ARoomGameModeBase::GetPartyModel() const
{
	return mPartyModel.Get();
}

UPlayerUnitModel* ARoomGameModeBase::GetPlayerUnitModel(int32 PlayerIndex) const
{
	if (mPartyModel.IsValid() == false)
	{
		return nullptr;
	}

	return mPartyModel->GetPlayerUnitModel(PlayerIndex);
}

TArray<TObjectPtr<UPlayerUnitModel>>& ARoomGameModeBase::GetPlayerUnitModels() const
{
	return mPartyModel->GetPlayerUnitModels();
}

const FName& ARoomGameModeBase::GetRoomSpawnSettingName() const
{
	return mSelectedRoomSpawnSettingName;
}

void ARoomGameModeBase::SetRoomSpawnSettingName(const FName& Name)
{
	mSelectedRoomSpawnSettingName = Name;
}

bool ARoomGameModeBase::BuildPartyUnitSkillDetailUI(int32 MemberIndex, int32 SkillIndex,
	FSkillDetailUI& OutDetail) const
{
	OutDetail = FSkillDetailUI();
	OutDetail.mSkillIndex = SkillIndex;

	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		return false;
	}
	const TArray<TObjectPtr<UPlayerUnitModel>>& Members = PartyModel->GetPlayerUnitModels();
	UPlayerUnitModel* Member = Members.IsValidIndex(MemberIndex)
		? Members[MemberIndex].Get() : nullptr;
	USkillComponentModel* SkillComponent = Member != nullptr
		? Member->GetSkillComponentModel() : nullptr;
	const FSkillEntry* SkillEntry = SkillComponent != nullptr
		? SkillComponent->GetSkill(SkillIndex) : nullptr;
	if (SkillEntry == nullptr || SkillEntry->IsValid() == false)
	{
		return false;
	}

	SkillDetailUIBuilder::FillFromSkillData(SkillEntry->mData.Get(), OutDetail);
	/* 쿨다운은 컴포넌트가 진짜다 -- 전투의 FillSkillDetailUIData와 같은 규칙. */
	OutDetail.mCooldownTurns = FMath::Max(
		SkillComponent->GetStaticCooldownDuration(SkillIndex), 0);
	return true;
}

bool ARoomGameModeBase::BuildPartyArtifactDetailUI(int32 ArtifactIndex,
	FCombatArtifactUI& OutDetail) const
{
	OutDetail = FCombatArtifactUI();

	UPartyModel* PartyModel = GetPartyModel();
	const UPartyArtifactComponentModel* PartyArtifacts = PartyModel != nullptr
		? PartyModel->GetPartyArtifactComponentModel() : nullptr;
	if (PartyArtifacts == nullptr)
	{
		return false;
	}
	const TArray<TObjectPtr<UStaticArtifactData>>& Artifacts =
		PartyArtifacts->GetPartyArtifacts();
	if (Artifacts.IsValidIndex(ArtifactIndex) == false
		|| Artifacts[ArtifactIndex] == nullptr)
	{
		return false;
	}

	SkillDetailUIBuilder::FillFromArtifactData(Artifacts[ArtifactIndex], OutDetail);
	return true;
}

bool ARoomGameModeBase::GetPartyRosterView(FPartyRosterView& OutView) const
{
	/*
	 * 인벤토리(GetInventoryView)와 같은 규칙 -- 밀지 않고 물어보게 둔다.
	 * 직업명/초상/스킬 아이콘 해석까지 여기서 끝내고, 위젯은 받은 값만 그린다.
	 */
	OutView = FPartyRosterView();

	UPartyModel* PartyModel = GetPartyModel();
	if (PartyModel == nullptr)
	{
		return false;
	}

	const TArray<TObjectPtr<UPlayerUnitModel>>& Members = PartyModel->GetPlayerUnitModels();
	for (int32 MemberIndex = 0; MemberIndex < Members.Num(); ++MemberIndex)
	{
		const UPlayerUnitModel* Member = Members[MemberIndex].Get();
		if (Member == nullptr)
		{
			// 빈 파티 슬롯은 명단에 싣지 않는다. 상세 왕복은 mMemberIndex 로 한다.
			continue;
		}

		FPartyRosterMemberView& Row = OutView.mMembers.AddDefaulted_GetRef();
		Row.mMemberIndex = MemberIndex;
		Row.mJobName = GetRosterJobName(Member);
		Row.mLevel = Member->GetPlayerLevel();
		Row.mPortrait = GetRosterHeadPortrait(Member);

		if (const UAttributeSetComponentModel* Attributes = Member->GetAttributeComponentModel())
		{
			Row.mHP = FMath::RoundToInt(Attributes->GetAttributeCurrentValue(
				UPlayerUnitAttributeSet::GetHPAttribute()));
			Row.mMaxHP = FMath::RoundToInt(Attributes->GetAttributeCurrentValue(
				UPlayerUnitAttributeSet::GetMaxHPAttribute()));
			Row.mAP = FMath::RoundToInt(Attributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetActionPointAttribute()));
			Row.mMaxAP = FMath::RoundToInt(Attributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetRechargeActionPointAttribute()));
			Row.mSpeed = FMath::RoundToInt(Attributes->GetAttributeCurrentValue(
				UUnitAttributeSet::GetRechargeSpeedPointAttribute()));
		}

		if (const USkillComponentModel* SkillComponent = Member->GetSkillComponentModel())
		{
			const TArray<FSkillEntry>& Entries = SkillComponent->GetSkills();
			for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
			{
				const UStaticSkillData* Skill = Entries[EntryIndex].mData;
				if (Skill == nullptr)
				{
					continue;
				}
				FPartyRosterSkillView& SkillRow = Row.mSkills.AddDefaulted_GetRef();
				SkillRow.mName = Skill->mName;
				SkillRow.mIcon = Skill->mIcon.LoadSynchronous();
				/* 상세 조회는 컴포넌트 원본 슬롯 index 로 해야 한다. */
				SkillRow.mSlotIndex = EntryIndex;
				if (const UStaticUnitSkillData* UnitSkill = Cast<UStaticUnitSkillData>(Skill))
				{
					SkillRow.mActionPointCost = FMath::Max(UnitSkill->mRequiredActionPoint, 0);
				}
				if (SkillRow.mIcon == nullptr)
				{
					// 아이콘 미설정 DA 는 화면이 이름 글자로 폴백한다. 데이터 소유자가
					// 알아챌 수 있게 생산자에서 한 번씩 흔적을 남긴다.
					UE_LOG(LogTemp, Verbose,
						TEXT("파티 명단 스킬 아이콘 미설정: %s (Member=%d, Slot=%d)"),
						*Skill->GetName(), MemberIndex, EntryIndex);
				}
			}
		}
	}

	/*
	 * 파티 공용 골드/아티팩트도 같은 판에 싣는다. 레일 인벤토리 페이지가
	 * 이 값만 보고 그린다 -- 조립 규칙(희귀도 색·이름 폴백)은 인벤토리와 같다.
	 */
	if (const UAttributeSetComponentModel* PartyAttributes =
		PartyModel->GetAttributeComponentModel())
	{
		OutView.mGold = FMath::RoundToInt(PartyAttributes->GetAttributeCurrentValue(
			UPartyAttributeSet::GetMoneyAttribute()));
	}
	if (const UPartyArtifactComponentModel* PartyArtifacts =
		PartyModel->GetPartyArtifactComponentModel())
	{
		const TArray<TObjectPtr<UStaticArtifactData>>& Artifacts =
			PartyArtifacts->GetPartyArtifacts();
		OutView.mArtifacts.Reserve(Artifacts.Num());
		for (int32 ArtifactIndex = 0; ArtifactIndex < Artifacts.Num(); ++ArtifactIndex)
		{
			const UStaticArtifactData* Artifact = Artifacts[ArtifactIndex];
			if (Artifact == nullptr)
			{
				continue;
			}

			FPartyRosterArtifactView& ArtifactRow = OutView.mArtifacts.AddDefaulted_GetRef();
			ArtifactRow.mArtifactIndex = ArtifactIndex;
			ArtifactRow.mName = Artifact->mName.IsEmpty() == false
				? Artifact->mName
				: FText::Format(
					NSLOCTEXT("RoomGameModeBase", "ArtifactFallbackName", "Artifact {0}"),
					FText::AsNumber(ArtifactIndex + 1));
			ArtifactRow.mIcon = ResolveRoomArtifactInventoryIcon(Artifact);
			ArtifactRow.mRarityColor = GetInventoryRarityColor(Artifact->mRarityType);
			ArtifactRow.mDetail = NSLOCTEXT(
				"RoomGameModeBase", "PartyArtifact", "Party-wide effect");
		}
	}

	// 빈 파티(전원 공석)도 판 자체는 유효하다 -- 거짓은 파티 모델 부재뿐.
	return true;
}
