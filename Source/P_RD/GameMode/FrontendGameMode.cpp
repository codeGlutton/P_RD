#include "GameMode/FrontendGameMode.h"

#include "GameFramework/PlayerController.h"

#include "Engine/AssetManager.h"
#include "Blueprint/UserWidget.h"
#include "DataAsset/PrimaryAssetType.h"

#include "DataAsset/StageSpawnData/StageLevelType.h"

#include "PCGStage/Stage.h"

#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"

#include "Setting/GamePlaySettings.h"
#include "DataAsset/RoomSpawnData/StaticFrontendRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "UI/RDUserWidget.h"

DEFINE_LOG_CATEGORY(LogFrontendGameMode);

namespace
{
	constexpr int32 DefaultDifficulty = 1;

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
		/*
		 * 실제 PlayerUnit DataAsset에서 특정 직업 카드가 내려왔는지 확인한다.
		 * 현재 Archer/Mage는 아직 실제 데이터가 준비되지 않은 경우가 있어, 없는 직업만 잠김 카드로 보강한다.
		 * 이렇게 해야 "데이터가 없어서 빈 칸"과 "잠긴 캐릭터로 보여주려는 의도"가 UI에서 구분된다.
		 */
		const FText JobText = GetPlayerJobName(JobType);
		return Options.ContainsByPredicate([&JobText](const FFrontendCharacterOption& Option)
		{
			return Option.mRoleText.EqualTo(JobText);
		});
	}

	void AppendLockedCharacterOption(TArray<FFrontendCharacterOption>& Options, EPlayerJobType JobType)
	{
		/*
		 * 아직 DataAsset이 준비되지 않은 캐릭터도 화면 슬롯은 유지한다.
		 * WBP 레이아웃 확인과 기획상 "추후 열릴 캐릭터" 표현을 위해 더미가 아니라 비활성 View 데이터로 추가한다.
		 * bSelectable=false와 DisabledReason을 같이 내려, CharacterSelectWidget이 Confirm을 막고 이유 문구를 표시할 수 있게 한다.
		 */
		FFrontendCharacterOption NewOption;
		NewOption.mIndex = Options.Num();
		NewOption.mDisplayName = GetPlayerJobName(JobType);
		NewOption.mRoleText = GetPlayerJobName(JobType);
		NewOption.mDescription = NSLOCTEXT("FrontendGameMode", "LockedCharacterDescription", "Character data is not ready");
		NewOption.mDisabledReason = NewOption.mDescription;
		NewOption.mSelectable = false;
		Options.Add(MoveTemp(NewOption));
	}

}

/**
 * @brief 프론트엔드 방에서 생성해둘 공용 월드 위젯 타입을 등록한다.
 *
 * @details
 * 여기서 등록한다는 말은 화면에 바로 띄운다는 뜻이 아니라,
 * BeginPlay()에서 WorldWidgetSubsystem::InitWorldWidget()이 해당 타입의 위젯 인스턴스를 미리 만들어 보관하게 한다는 뜻이다.
 * 실제로 화면에 보일 때는 HUD든 WorldWidget이든 모두 URDUserWidget::OpenUI()를 통한다.
 *
 * 타이틀 HUD는 프론트엔드 방의 메인 화면이라 mHUDClass와 InitHUD() 경로로 한 개만 생성하고,
 * BeginRoom()에서 OpenUI()로 표시한다. 반면 MsgNotify, FadeInOut, LoadingNotify, InGameSettings는
 * 여러 GameMode가 공유하거나 HUD 밖에서 뜨는 보조 UI라 mWorldWidgets에 넣어 WorldWidgetSubsystem이 생성/보관하게 한다.
 *
 * InGameSettings를 프론트엔드에도 준비하는 이유:
 * 타이틀 설정과 인게임 설정은 같은 WBP_SettingsPanel을 사용해야 하고, 차이는 PanelMode로 저장 후 종료/포기하기 영역만 숨기는 것이다.
 * 타이틀 WBP 안에 자체 설정 영역이 있더라도 실제 설정 화면은 이 공용 월드 위젯을 OpenUI()로 연다.
 *
 * 왜 생성 경로를 나누는가:
 * "누가 만들고 보관할지"와 "어떻게 화면에 열지"는 다른 문제다. 생성 책임은 HUD/WorldWidget으로 나누되,
 * 표시 책임은 OpenUI() 하나로 맞춰야 AddToViewport, Visibility, 애니메이션 완료 콜백 규칙이 통일된다.
 */
AFrontendGameMode::AFrontendGameMode()
{
	mWorldWidgets = {
		EWorldWidgetType::MsgNotify,
		EWorldWidgetType::FadeInOut,
		EWorldWidgetType::LoadingNotify,
		EWorldWidgetType::InGameSettings,
	};

	mShowFadeInUIOnTransition = true;
	mShowFadeOutUIOnTransition = true;
	mShowLoadingNotifyUIOnTransition = true;
	mWaitExternalWorkOnTransition = false;
}

/**
 * @brief 프론트엔드 방 진입 후 타이틀 HUD를 공통 UI 생명주기로 연다.
 *
 * @details
 * RDUserWidget 기반 HUD는 InitHUD()에서 생성되더라도 자동으로 화면에 표시되지 않는다.
 * 실제 표시 시점은 GameMode가 방 진입 준비를 끝낸 뒤 OpenUI()로 명시한다.
 *
 * 여기서 UTitleMenuWidget으로 구체 타입을 확인하는 이유는 프론트엔드 시작 화면이 단순한 HUD 베이스가 아니라
 * 캐릭터 선택, 설정, 이어하기/새 런 시작 흐름을 가진 타이틀 메뉴여야 하기 때문이다.
 * 공통 베이스 포인터만 받아 열면 잘못된 HUD 클래스가 들어와도 늦게 발견되므로, 방 시작 시점에 바로 검증한다.
 *
 * 왜 InitHUD()에서 바로 AddToViewport 하지 않는가:
 * HUD 생성은 "준비"이고 OpenUI()는 "보여주기"다. 두 단계를 분리해야 타이틀 HUD도 다른 위젯처럼
 * 열기 애니메이션과 완료 콜백 규칙을 공유하고, 잘못된 HUD 클래스도 OpenUI 호출 전에 확인할 수 있다.
 */
void AFrontendGameMode::BeginRoom()
{
	Super::BeginRoom();

	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UTitleMenuWidget* TitleHUD = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>();
	checkf(TitleHUD != nullptr, TEXT("타이틀 HUD 위젯 nullptr"));
	TitleHUD->OpenUI();

	/* 터치 세팅 */

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	checkf(PlayerController != nullptr, TEXT(""));

	PlayerController->ActivateTouchInterface(nullptr);

	FInputModeGameAndUI InputMode;
	PlayerController->SetInputMode(InputMode);
}

bool AFrontendGameMode::CreateNewRunFromTitle()
{
	/*
	 * 타이틀 START는 아직 런을 만들지 않는다.
	 * 캐릭터 선택이 끝나야 PlayerUnitId와 난이도가 확정되므로, 여기서는 캐릭터 선택 화면으로 넘기는 것만 담당한다.
	 */
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogRDGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	checkf(OpenTitleCharacterSelect() == true, TEXT("캐릭터 선택창 열기 오류"));

	return true;
}

bool AFrontendGameMode::StartNewRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty)
{
	/*
	 * 캐릭터 선택 화면에서 Confirm이 눌린 뒤의 실제 시작 지점이다.
	 * 먼저 선택된 캐릭터로 RunPersistData를 만들고, 그 다음 Stage1 첫 방을 프리로드/전환한다.
	 * UI는 "시작 요청"만 하고, 저장 데이터 생성과 방 전환 순서는 GameMode가 고정한다.
	 */
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	checkf(CreateRunData(PlayerUnitId, Difficulty) == true, TEXT("런 데이터 생성 오류"));
	checkf(PreloadAndTransitionRoomAsync(EStageLevelType::Stage1) == true, TEXT("스테이지 1 처음 방으로 전환 실패"));
	return true;
}

bool AFrontendGameMode::ContinueRunFromTitle()
{
	/*
	 * 타이틀에서는 더 이상 지도 미리보기를 열지 않는다.
	 * 활성 Run이 있으면 현재 저장된 방 위치로 바로 전환하고, 월드맵 조회/다음 방 선택은 방 입장 후 RoomGameMode가 맡는다.
	 */
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || RunPersistData->IsActive() == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("이어갈 런 데이터가 없음"));
		return false;
	}

	int32 CurrentRowIndex = INDEX_NONE;
	int32 CurrentColumnIndex = INDEX_NONE;
	RunPersistData->GetCurrentRoomIndex(OUT CurrentRowIndex, OUT CurrentColumnIndex);

	const FStage& Stage = RunPersistData->GetStage();
	if (Stage.HasRoom(CurrentRowIndex, CurrentColumnIndex) == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("현재 런의 방 위치가 Stage에 없음"));
		return false;
	}

	checkf(PreloadAndTransitionRoomAsync(CurrentRowIndex, CurrentColumnIndex) == true, TEXT("이어하기 방 전환 실패"));
	return true;
}

bool AFrontendGameMode::AbandonRunFromTitle()
{
	/*
	 * 타이틀 설정 화면에서 기존 런을 포기할 때 호출된다.
	 * 여기서는 전투 방이 아니므로 방 전환을 일으키지 않고 RunPersistData만 비운다.
	 * 이후 타이틀 메뉴가 RefreshMainMenuState()를 호출하면 Continue 버튼이 사라진다.
	 */
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	if (CanAbandonRun() == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("맵 준비가 되지 않음"));
		return false;
	}

	ClearRunPersistData();
	return true;
}

bool AFrontendGameMode::GetCharacterOptions(TArray<FFrontendCharacterOption>& OutOptions) const
{
	/*
	 * CharacterSelectWidget은 PlayerUnit DataAsset 구조를 직접 알지 않는다.
	 * 이 함수가 DataAsset을 UI 전용 DTO로 바꿔주면, WBP 쪽은 이름/직업/스탯/초상화/선택 가능 여부만 보고 카드를 그릴 수 있다.
	 */
	OutOptions.Reset();

	const TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>>& PlayerUnitDatas = GetPlayerUnitDatas();
	for (const TSoftObjectPtr<UStaticPlayerUnitSpawnData>& PlayerUnitData : PlayerUnitDatas)
	{
		const UStaticPlayerUnitSpawnData* LoadedPlayerUnitData = PlayerUnitData.Get();
		if (LoadedPlayerUnitData == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("선택 가능한 Player Unit 로드 실패"));
			continue;
		}

		FFrontendCharacterOption NewOption;
		NewOption.mIndex = OutOptions.Num();
		NewOption.mDisplayName = LoadedPlayerUnitData->mDisplayName.IsEmpty()
			? FText::FromName(LoadedPlayerUnitData->GetPrimaryAssetId().PrimaryAssetName)
			: LoadedPlayerUnitData->mDisplayName;
		NewOption.mRoleText = GetPlayerJobName(LoadedPlayerUnitData->mJobType);
		NewOption.mDescription = LoadedPlayerUnitData->mDescription;
		NewOption.mMaxHP = FMath::RoundToInt(LoadedPlayerUnitData->GetDefaultMaxHP(DefaultDifficulty));
		NewOption.mDice = LoadedPlayerUnitData->mDiceDatas.Num();
		NewOption.mGold = FMath::RoundToInt(LoadedPlayerUnitData->GetDefaultMoney(DefaultDifficulty));
		NewOption.mStatSummary = FText::Format(
			NSLOCTEXT("FrontendGameMode", "CharacterStatSummary", "HP {0} / Dice {1} / Gold {2}"),
			FText::AsNumber(NewOption.mMaxHP),
			FText::AsNumber(NewOption.mDice),
			FText::AsNumber(NewOption.mGold));
		NewOption.mPortrait = LoadedPlayerUnitData->mPortrait;
		NewOption.mIcon = LoadedPlayerUnitData->mIcon;
		NewOption.mPlayerUnitId = LoadedPlayerUnitData->GetPrimaryAssetId();
		NewOption.mSelectable = PlayerUnitData.IsValid() && !LoadedPlayerUnitData->mClass.IsNull();
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

	return OutOptions.IsEmpty() == false;
}

const TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>>& AFrontendGameMode::GetPlayerUnitDatas() const
{
	/*
	 * 선택 가능한 캐릭터 목록은 FrontendRoom DataAsset(DA_TestFrontend)의 mPlayableUnits가 기준이다.
	 * Intro/FrontendRoom 프리로드 단계에서 FrontendRoom과 같은 번들의 PlayerUnit들이 메모리에 올라와 있어야 하고,
	 * 여기서는 이미 로드된 PrimaryAsset 객체에서 SoftObjectPtr 목록만 꺼내 캐시한다.
	 *
	 * 왜 CSV나 별도 임시 목록을 쓰지 않는가:
	 * 프론트엔드 방 정보 안에 "이 화면에서 선택 가능한 캐릭터"가 이미 들어 있으므로,
	 * UI 쪽에서 새 데이터 소스를 만들면 에셋 설정과 화면 표시가 갈라진다.
	 */
	if (mPlayerUnitDataCache.IsEmpty() == true)
	{
		UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
		if (AssetManager != nullptr)
		{
			const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
			if (GamePlaySettings != nullptr)
			{
				TObjectPtr<UStaticFrontendRoomSpawnData> FrontendRoomSpawnData = AssetManager->GetPrimaryAssetObject<UStaticFrontendRoomSpawnData>(GamePlaySettings->mFrontendRoomId);
				mPlayerUnitDataCache = FrontendRoomSpawnData->mPlayableUnits;
			}
		}
	}
	return mPlayerUnitDataCache;
}

bool AFrontendGameMode::IsPlayerUnitIdValid(const FPrimaryAssetId& PlayerUnitId) const
{
	/*
	 * Confirm 버튼이 보낸 PlayerUnitId가 실제 프론트엔드 방의 선택 후보 안에 있는지 마지막으로 확인한다.
	 * UI에서 비활성 카드를 막아도, 런 생성 직전 GameMode가 다시 검증해야 잘못된 ID로 StartRun()이 호출되지 않는다.
	 */
	if (PlayerUnitId.IsValid() == false)
	{
		return false;
	}

	const TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>>& PlayerUnitDatas = GetPlayerUnitDatas();
	bool IsFound = PlayerUnitDatas.ContainsByPredicate([&PlayerUnitId](const TSoftObjectPtr<UStaticPlayerUnitSpawnData>& PlayerUnitData) {
		const UStaticPlayerUnitSpawnData* LoadedPlayerUnitData = PlayerUnitData.Get();
		if (LoadedPlayerUnitData == nullptr)
		{
			return false;
		}
		return LoadedPlayerUnitData->GetPrimaryAssetId() == PlayerUnitId;
		});

	return IsFound;
}

bool AFrontendGameMode::IsDifficultyValid(int32 Difficulty) const
{
	// TODO : 유효성 검사

	return true;
}

bool AFrontendGameMode::OpenTitleCharacterSelect()
{
	/*
	 * 타이틀 HUD 내부 화면 전환은 TitleMenuWidget이 맡는다.
	 * GameMode가 ScreenSwitcher를 직접 만지지 않고, 타이틀 위젯에 "캐릭터 선택 화면을 열어 달라"고 요청하는 구조다.
	 */
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>();
	checkf(TitleMenuWidget != nullptr, TEXT("타이틀 HUD가 준비가 안됨"));

	TitleMenuWidget->OpenCharacterSelectFromTitle();
	return true;
}

bool AFrontendGameMode::CreateRunData(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty)
{
	/*
	 * RunPersistData 생성은 캐릭터/난이도 검증이 끝난 뒤 GameProfileSubsystem에 위임한다.
	 * 이 함수가 UI DTO나 WBP 상태를 읽지 않는 이유는, 런 생성 규칙을 화면 구조와 분리하기 위해서다.
	 */
	if (IsPlayerUnitIdValid(PlayerUnitId) == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("플레이어 유닛 데이터를 찾을 수 없음"));
		return false;
	}
	if (IsDifficultyValid(Difficulty) == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("불가능한 난이도 선택"));
		return false;
	}

	UGameProfileSubsystem* GameProfileSubsystem = GetGameInstance()->GetSubsystem<UGameProfileSubsystem>();
	checkf(GameProfileSubsystem != nullptr, TEXT("게임 프로파일 서브시스템 nullptr 오류"));

	if (GetUserPersistData()->IsActive() == false)
	{
		/*
		 * 세이브가 없는 첫 실행은 Intro에서 유저 데이터 로드 실패 후 빈 UserData로 프론트엔드에 들어온다.
		 * 새 런 생성은 유저 프로필을 전제로 하므로, 첫 런 시작 시점에 기본 유저를 먼저 만든다.
		 */
		GameProfileSubsystem->MakeUser(NSLOCTEXT("FrontendGameMode", "DefaultUserName", "Player"));
	}

	GameProfileSubsystem->StartRun(PlayerUnitId, Difficulty);
	return true;
}

