#include "GameMode/FrontendGameMode.h"

#include "GameFramework/PlayerController.h"

#include "Engine/AssetManager.h"
#include "Blueprint/UserWidget.h"
#include "DataAsset/PrimaryAssetType.h"

#include "DataAsset/StageSpawnData/StageLevelType.h"

#include "PCGStage/Room.h"
#include "PCGStage/Stage.h"

#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
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
		const FText JobText = GetPlayerJobName(JobType);
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
		NewOption.mRoleText = GetPlayerJobName(JobType);
		NewOption.mDescription = NSLOCTEXT("FrontendGameMode", "LockedCharacterDescription", "Character data is not ready");
		NewOption.mDisabledReason = NewOption.mDescription;
		NewOption.bSelectable = false;
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
 * BeginRoom()에서 OpenUI()로 표시한다. 반면 MsgNotify, FadeInOut, LoadingNotify는 여러 GameMode가 공유하는
 * 보조 UI라 mWorldWidgets에 넣어 WorldWidgetSubsystem이 생성/보관하게 한다.
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
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	checkf(CreateRunData(PlayerUnitId, Difficulty) == true, TEXT("런 데이터 생성 오류"));
	checkf(PreloadAndTransitionRoomAsync(EStageLevelType::Stage1) == true, TEXT("스테이지 1 처음 방으로 전환 실패"));
	return true;
}

bool AFrontendGameMode::AbandonRunFromTitle()
{
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
		NewOption.bSelectable = PlayerUnitData.IsValid() && !LoadedPlayerUnitData->mClass.IsNull();
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
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>();
	checkf(TitleMenuWidget != nullptr, TEXT("타이틀 HUD가 준비가 안됨"));

	TitleMenuWidget->OpenCharacterSelectFromTitle();
	return true;
}

bool AFrontendGameMode::CreateRunData(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty)
{
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

	GameProfileSubsystem->StartRun(PlayerUnitId, Difficulty);
	return true;
}

