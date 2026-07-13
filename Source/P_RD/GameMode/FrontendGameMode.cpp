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
#include "UI/CharacterSelectWidget.h"

#include "Setting/GamePlaySettings.h"
#include "DataAsset/RoomSpawnData/StaticFrontendRoomSpawnData.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "UI/RDUserWidget.h"

#include "AttributeSet/UnitAttributeSet.h"

DEFINE_LOG_CATEGORY(LogFrontendGameMode);

/** @brief 프론트엔드 방 GameMode 역할 요약 */
// AFrontendGameMode는 "타이틀 -> 캐릭터 선택 -> 새 Run 생성/이어하기 시작" 흐름만 담당한다.
// 실제 방 안에서의 WorldMap, 다음 방 선택, 저장 후 전환은 ARoomGameModeBase 쪽 책임이다.
// 주요 흐름:
// - BeginRoom(): 프론트엔드 방에 들어오면 TitleMenuWidget HUD를 OpenUI()로 열고 입력 모드를 설정한다.
// - RequestCharacterSelectFromTitle(): 타이틀 START 입력을 받아 캐릭터 선택 화면 진입만 요청한다.
// - OpenTitleCharacterSelect(): TitleMenuWidget에 캐릭터 선택 화면을 열라고 전달한다.
// - GetCharacterOptions(): 캐릭터 선택 카드에 표시할 FFrontendCharacterOption 목록을 만든다.
// - GetPlayerUnitDatas(): DA_TestFrontend.mPlayableUnits에서 선택 가능한 캐릭터 DataAsset 목록을 가져온다.
// - IsPlayerUnitIdValid(): 선택된 캐릭터가 실제 선택 후보 안에 있는지 검증한다.
// - IsDifficultyValid(): 난이도 검증 자리다. 현재는 난이도 기능이 없어 항상 true를 반환한다.
// - StartNewRun(): 캐릭터 선택 Confirm 이후 새 Run을 만들고 Stage1 첫 방 전환을 요청한다.
// - CreateRunData(): GameProfileSubsystem->StartRun()으로 RunPersistData 생성을 위임한다.
// - ContinueRunFromTitle(): 타이틀 CONTINUE 입력을 받아 저장된 Run의 현재 방 row/column으로 입장을 요청한다.
// - AbandonRunFromTitle(): 타이틀/설정 화면에서 기존 Run 포기를 처리하고 RunPersistData만 비운다.
namespace
{
	// [합의필요] 난이도 선택 UI가 들어오기 전까지 새 Run 생성은 프론트엔드 기본 난이도 1로 고정한다.
	constexpr int32 DefaultDifficulty = 1;

	/** @brief UI 표시용 직업명(한글)을 GameMode에서 확정해 WBP가 enum 문자열을 직접 해석하지 않게 한다. */
	FText GetPlayerJobName(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightNameText", "기사");
		case EPlayerJobType::Archer:
			return NSLOCTEXT("FrontendGameMode", "ArcherNameText", "도적");
		case EPlayerJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageNameText", "마법사");
		default:
			return NSLOCTEXT("FrontendGameMode", "UnknownNameText", "알 수 없음");
		}
	}

	/** @brief 이름과 구분되는 역할 한 줄 문구. 카드 상세에서 이름 아래 부제로 쓴다. */
	FText GetPlayerJobRole(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightRoleText", "방패 탱커 · 근접");
		case EPlayerJobType::Archer:
			return NSLOCTEXT("FrontendGameMode", "ArcherRoleText", "기습 암살자 · 민첩");
		case EPlayerJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageRoleText", "주문 술사 · 원거리");
		default:
			return FText::GetEmpty();
		}
	}

	/** @brief 직업별 설명 문구(DataAsset 설명이 비어 있을 때 폴백). 설명 스크림에 표시된다. */
	FText GetPlayerJobDescription(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightDesc", "두꺼운 갑옷과 방패로 전열을 지키는 근접 수호자.\n높은 체력으로 적의 공격을 버틴다.");
		case EPlayerJobType::Archer:
			return NSLOCTEXT("FrontendGameMode", "ArcherDesc", "그림자에서 기습하는 민첩한 암살자.\n빠른 연속 공격으로 적을 무너뜨린다.");
		case EPlayerJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageDesc", "주사위 마법으로 광역 피해를 주는 원거리 술사.\n강력하지만 체력이 낮다.");
		default:
			return FText::GetEmpty();
		}
	}

	/** @brief 실제 DataAsset 후보에 특정 직업이 이미 있는지 확인해 잠금 placeholder 중복 생성을 막는다. */
	bool HasCharacterOptionForJob(const TArray<FFrontendCharacterOption>& Options, EPlayerJobType JobType)
	{
		/*
		 * 실제 PlayerUnit DataAsset에서 특정 직업 카드가 내려왔는지 확인한다.
		 * 현재 Archer/Mage는 아직 실제 데이터가 준비되지 않은 경우가 있어, 없는 직업만 잠김 카드로 보강한다.
		 * 이렇게 해야 "데이터가 없어서 빈 칸"과 "잠긴 캐릭터로 보여주려는 의도"가 UI에서 구분된다.
		 */
		return Options.ContainsByPredicate([JobType](const FFrontendCharacterOption& Option)
		{
			return Option.mJobType == JobType;
		});
	}

	/** @brief 아직 DataAsset이 없는 직업을 선택 불가 View 데이터로 추가해 레이아웃 슬롯을 유지한다. */
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
		NewOption.mRoleText = GetPlayerJobRole(JobType);
		// 잠긴 카드도 직업별 일러스트는 보여줄 수 있게 화면 아트 키를 채운다.
		NewOption.mJobType = JobType;
		// "데이터 없음" 문구 대신 직업 설명을 표시(기획: not ready 삭제). 잠금 사유는 내부적으로만 유지.
		NewOption.mDescription = GetPlayerJobDescription(JobType);
		NewOption.mDisabledReason = NSLOCTEXT("FrontendGameMode", "LockedCharacterReason", "준비 중인 캐릭터입니다");
		NewOption.mSelectable = false;
		Options.Add(MoveTemp(NewOption));
	}

	int32 GetClassSelectSortOrder(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return 0;
		case EPlayerJobType::Mage:
			return 1;
		case EPlayerJobType::Archer:
			return 2;
		default:
			return 99;
		}
	}

	void SortCharacterOptionsForClassSelect(TArray<FFrontendCharacterOption>& Options)
	{
		Options.StableSort([](const FFrontendCharacterOption& Left, const FFrontendCharacterOption& Right)
		{
			const int32 LeftOrder = GetClassSelectSortOrder(Left.mJobType);
			const int32 RightOrder = GetClassSelectSortOrder(Right.mJobType);
			if (LeftOrder != RightOrder)
			{
				return LeftOrder < RightOrder;
			}

			return Left.mIndex < Right.mIndex;
		});

		for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
		{
			Options[OptionIndex].mIndex = OptionIndex;
		}
	}

}

/** @brief 프론트엔드 방에서 생성해둘 공용 월드 위젯 타입을 등록한다. */
// 여기서 등록한다는 말은 화면에 바로 띄운다는 뜻이 아니라,
// BeginPlay()에서 WorldWidgetSubsystem::InitWorldWidget()이 해당 타입의 위젯 인스턴스를 미리 만들어 보관하게 한다는 뜻이다.
// 실제로 화면에 보일 때는 HUD든 WorldWidget이든 모두 URDUserWidget::OpenUI()를 통한다.
// 타이틀 HUD는 프론트엔드 방의 메인 화면이라 mHUDClass와 InitHUD() 경로로 한 개만 생성하고,
// BeginRoom()에서 OpenUI()로 표시한다. 반면 MsgNotify, FadeInOut, LoadingNotify, InGameSettings는
// 여러 GameMode가 공유하거나 HUD 밖에서 뜨는 보조 UI라 mWorldWidgets에 넣어 WorldWidgetSubsystem이 생성/보관하게 한다.
// InGameSettings를 프론트엔드에도 준비하는 이유:
// 타이틀 설정과 인게임 설정은 같은 WBP_SettingsPanel을 사용해야 하고, 차이는 PanelMode로 저장 후 종료/포기하기 영역만 숨기는 것이다.
// 타이틀 WBP 안에 자체 설정 영역이 있더라도 실제 설정 화면은 이 공용 월드 위젯을 OpenUI()로 연다.
// 왜 생성 경로를 나누는가:
// "누가 만들고 보관할지"와 "어떻게 화면에 열지"는 다른 문제다. 생성 책임은 HUD/WorldWidget으로 나누되,
// 표시 책임은 OpenUI() 하나로 맞춰야 AddToViewport, Visibility, 애니메이션 완료 콜백 규칙이 통일된다.
AFrontendGameMode::AFrontendGameMode()
{
	mWorldWidgets = {
		EWorldWidgetType::MsgNotify,
		EWorldWidgetType::FadeInOut,
		EWorldWidgetType::LoadingNotify,
		EWorldWidgetType::InGameSettings,
		EWorldWidgetType::CharacterSelect,
	};

	mShowFadeInUIOnTransition = true;
	mShowFadeOutUIOnTransition = true;
	mShowLoadingNotifyUIOnTransition = true;
	mWaitExternalWorkOnTransition = false;
}

void AFrontendGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	checkf(GamePlaySettings != nullptr, TEXT("게임 플레이 세팅 CDO nullptr"));

	TObjectPtr<UStaticFrontendRoomSpawnData> FrontendRoomSpawnData = AssetManager->GetPrimaryAssetObject<UStaticFrontendRoomSpawnData>(GamePlaySettings->mFrontendRoomId);
	TSoftObjectPtr<USoundBase> MainBGMSoftPtr = FrontendRoomSpawnData->mOverrideBGM;
	SetMainBGM(MainBGMSoftPtr.LoadSynchronous());
}

/** @brief 프론트엔드 방 진입 후 타이틀 HUD를 공통 UI 생명주기로 연다. */
// RDUserWidget 기반 HUD는 InitHUD()에서 생성되더라도 자동으로 화면에 표시되지 않는다.
// 실제 표시 시점은 GameMode가 방 진입 준비를 끝낸 뒤 OpenUI()로 명시한다.
// 여기서 UTitleMenuWidget으로 구체 타입을 확인하는 이유는 프론트엔드 시작 화면이 단순한 HUD 베이스가 아니라
// 캐릭터 선택, 설정, 이어하기/새 런 시작 흐름을 가진 타이틀 메뉴여야 하기 때문이다.
// 공통 베이스 포인터만 받아 열면 잘못된 HUD 클래스가 들어와도 늦게 발견되므로, 방 시작 시점에 바로 검증한다.
// 왜 InitHUD()에서 바로 AddToViewport 하지 않는가:
// HUD 생성은 "준비"이고 OpenUI()는 "보여주기"다. 두 단계를 분리해야 타이틀 HUD도 다른 위젯처럼
// 열기 애니메이션과 완료 콜백 규칙을 공유하고, 잘못된 HUD 클래스도 OpenUI 호출 전에 확인할 수 있다.
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

#if PLATFORM_DESKTOP
	PlayerController->SetShowMouseCursor(true);
#endif
#if PLATFORM_ANDROID
	PlayerController->ActivateTouchInterface(nullptr);
#endif

	FInputModeGameAndUI InputMode;
	PlayerController->SetInputMode(InputMode);
}

/** @brief 타이틀 START 입력을 캐릭터 선택 화면 진입 요청으로 처리한다. */
// 프론트엔드의 START는 아직 새 Run 생성 단계가 아니다.
// 캐릭터와 난이도가 확정되기 전이므로, 여기서는 TitleMenuWidget의 캐릭터 선택 화면만 열고
// 실제 RunPersistData 생성은 CharacterSelectWidget의 Confirm 이후 StartNewRun()에서 처리한다.
bool AFrontendGameMode::RequestCharacterSelectFromTitle()
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

/** @brief 캐릭터 선택 Confirm 이후 새 Run을 만들고 Stage1 첫 방 입장을 시작한다. */
// UI는 선택된 PlayerUnitId와 Difficulty만 전달하고,
// GameMode가 "선택값 검증 -> RunPersistData 생성 -> Stage1 첫 방 프리로드/전환" 순서를 고정한다.
// 이렇게 해야 캐릭터 선택 WBP가 저장 데이터 생성이나 방 전환 시스템을 직접 호출하지 않는다.
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

/** @brief 타이틀 CONTINUE 입력으로 저장된 활성 Run의 현재 방 입장을 시작한다. */
// Continue는 세이브 파일을 여기서 새로 불러오는 기능이 아니라,
// 이미 활성화되어 있는 RunPersistData의 현재 row/column으로 방 전환을 요청하는 기능이다.
// 타이틀에서는 월드맵이나 다음 방 선택을 열지 않고, 방 입장 이후의 UI/선택/저장 흐름은 RoomGameModeBase가 맡는다.
bool AFrontendGameMode::ContinueRunFromTitle()
{
	/*
	 * Continue는 "세이브 파일을 불러온다"가 아니라 "이미 활성화된 Run의 현재 방으로 돌아간다"는 의미다.
	 * 세이브 슬롯 선택/로드는 프론트엔드에 진입하기 전 또는 별도 UI 흐름에서 끝나 있어야 하고,
	 * 이 함수는 RunPersistData가 이미 준비되어 있다는 전제에서 방 전환만 시작한다.
	 *
	 * 타이틀에서는 더 이상 지도 미리보기나 다음 방 선택을 열지 않는다.
	 * 타이틀은 시작/이어하기 같은 프론트엔드 선택만 담당하고,
	 * 현재 방 UI, 월드맵 표시, 다음 방 선택, 저장 후 전환은 실제 방에 들어간 뒤 RoomGameMode가 맡는다.
	 */
	if (mWasNextRoomPreloadRequested == true)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("방 전환 시 추가 로직 요청 불가"));
		return false;
	}

	/*
	 * Continue 버튼은 TitleMenuWidget에서 활성 Run이 있을 때만 보이지만,
	 * UI 상태가 갱신되기 전 클릭되거나 외부 흐름에서 직접 호출될 수 있으므로 GameMode에서도 다시 확인한다.
	 */
	const URunPersistData* RunPersistData = GetRunPersistData();
	if (RunPersistData == nullptr || RunPersistData->IsActive() == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("이어갈 런 데이터가 없음"));
		return false;
	}

	int32 CurrentRowIndex = INDEX_NONE;
	int32 CurrentColumnIndex = INDEX_NONE;
	RunPersistData->GetCurrentRoomIndex(OUT CurrentRowIndex, OUT CurrentColumnIndex);

	/*
	 * RunPersistData에는 "현재 방 좌표"만 저장되어 있으므로,
	 * 전환 요청 전에 해당 좌표가 현재 Stage에 실제로 존재하는 방인지 확인한다.
	 * 좌표가 깨진 상태로 PreloadAndTransitionRoomAsync()에 넘기면 잘못된 방 전환 요청이 되기 때문이다.
	 */
	const FStage& Stage = RunPersistData->GetStage();
	if (Stage.HasRoom(CurrentRowIndex, CurrentColumnIndex) == false)
	{
		UE_LOG(LogFrontendGameMode, Log, TEXT("현재 런의 방 위치가 Stage에 없음"));
		return false;
	}

	/*
	 * 여기서부터는 기존 방 전환 공통 흐름을 사용한다.
	 * 플레이어/스테이지/방 에셋 프리로드, 페이드/로딩 UI, 실제 레벨 전환 순서는 RDGameModeBase와 RoomTransitionSubsystem이 처리한다.
	 */
	checkf(PreloadAndTransitionRoomAsync(CurrentRowIndex, CurrentColumnIndex) == true, TEXT("이어하기 방 전환 실패"));
	return true;
}

/** @brief 타이틀/설정 화면에서 기존 활성 Run 포기를 처리한다. */
// 프론트엔드는 아직 실제 전투 방 안이 아니므로 전투 정리나 방 퇴장 처리를 하지 않는다.
// RunPersistData만 비우고, 이후 TitleMenuWidget이 메뉴 상태를 다시 읽어 Continue 버튼 표시 여부를 갱신한다.
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
		UE_LOG(LogFrontendGameMode, Log, TEXT("포기할 활성 런 데이터가 없음"));
		return false;
	}

	ClearRunPersistData();
	return true;
}

/** @brief 캐릭터 선택 카드에 표시할 FFrontendCharacterOption 목록을 만든다. */
// DA_TestFrontend의 mPlayableUnits에서 로드된 PlayerUnit DataAsset을 읽어
// 이름, 직업, 스탯, 초상화, 선택 가능 여부만 담은 UI 전용 DTO로 변환한다.
// CharacterSelectWidget은 이 View 데이터만 보고 카드를 그리며, PlayerUnit DataAsset 내부 구조를 직접 해석하지 않는다.
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
		// mIndex는 클릭 이벤트의 안정 키다. WBP 카드 배열 위치와 같게 시작하지만 UI는 이 값을 기준으로 다시 찾는다.
		NewOption.mIndex = OutOptions.Num();
		// 표시 이름은 한글 직업명으로 통일(기획: 한글화). 역할은 이름과 구분되는 한 줄 문구.
		NewOption.mDisplayName = GetPlayerJobName(LoadedPlayerUnitData->mJobType);
		NewOption.mRoleText = GetPlayerJobRole(LoadedPlayerUnitData->mJobType);
		// 설명: DataAsset 설명이 있으면 그것, 없으면 직업별 폴백(설명 스크림에 표시).
		const FText JobDesc = GetPlayerJobDescription(LoadedPlayerUnitData->mJobType);
		// 화면 아트(직업별 일러스트) 선택용 키. 이 값이 빠지면 UI가 직업을 None으로 보고 일러스트를 못 고른다.
		NewOption.mJobType = LoadedPlayerUnitData->mJobType;
		NewOption.mDescription = LoadedPlayerUnitData->mDescription.IsEmpty()
			? JobDesc : LoadedPlayerUnitData->mDescription;
		NewOption.mMaxHP = FMath::RoundToInt(LoadedPlayerUnitData->GetDefaultAttributeValue(GetWorld(), UPlayerUnitAttributeSet::StaticClass(), UPlayerUnitAttributeSet::GetMaxHPAttribute(), DefaultDifficulty));
		NewOption.mDice = LoadedPlayerUnitData->mDiceDatas.Num();
		NewOption.mGold = FMath::RoundToInt(LoadedPlayerUnitData->GetDefaultAttributeValue(GetWorld(), UPlayerUnitAttributeSet::StaticClass(), UPlayerUnitAttributeSet::GetMoneyAttribute(), DefaultDifficulty));
		NewOption.mStatSummary = FText::Format(
			NSLOCTEXT("FrontendGameMode", "CharacterStatSummary", "HP {0} / Dice {1} / Gold {2}"),
			FText::AsNumber(NewOption.mMaxHP),
			FText::AsNumber(NewOption.mDice),
			FText::AsNumber(NewOption.mGold));
		NewOption.mPortrait = LoadedPlayerUnitData->mPortrait;
		NewOption.mIcon = LoadedPlayerUnitData->mIcon;
		NewOption.mPlayerUnitId = LoadedPlayerUnitData->GetPrimaryAssetId();
		// 선택 가능 여부는 로드 상태와 실제 Pawn Class 설정을 동시에 본다. UI는 이 bool을 재해석하지 않는다.
		NewOption.mSelectable = PlayerUnitData.IsValid() && !LoadedPlayerUnitData->mViewClass.IsNull();
		OutOptions.Add(MoveTemp(NewOption));
	}

	// [합의필요] Archer/Mage DataAsset이 준비되면 placeholder 보강을 제거하고 DA_TestFrontend.mPlayableUnits만 단일 출처로 둔다.

	if (!HasCharacterOptionForJob(OutOptions, EPlayerJobType::Archer))
	{
		AppendLockedCharacterOption(OutOptions, EPlayerJobType::Archer);
	}
	if (!HasCharacterOptionForJob(OutOptions, EPlayerJobType::Mage))
	{
		AppendLockedCharacterOption(OutOptions, EPlayerJobType::Mage);
	}

	// 위 placeholder는 선택 불가 View 데이터라 런 생성으로 이어지지 않는다.
	SortCharacterOptionsForClassSelect(OutOptions);

	return OutOptions.IsEmpty() == false;
}

/** @brief 프론트엔드 방 DataAsset에 등록된 선택 가능 캐릭터 목록을 가져온다. */
// 선택 가능한 캐릭터의 기준은 DA_TestFrontend.mPlayableUnits다.
// 이 함수는 별도 임시 목록이나 CSV를 만들지 않고, 프론트엔드 방 정보에 저장된 PlayerUnit SoftObjectPtr 목록만 캐시한다.
// 실제 캐릭터 카드 View 변환은 GetCharacterOptions()에서 수행한다.
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
				// FrontendRoom은 전환 전에 프리로드되어 있어야 한다. nullptr이면 설정/번들 계약이 깨진 것이다.
				TObjectPtr<UStaticFrontendRoomSpawnData> FrontendRoomSpawnData = AssetManager->GetPrimaryAssetObject<UStaticFrontendRoomSpawnData>(GamePlaySettings->mFrontendRoomId);
				mPlayerUnitDataCache = FrontendRoomSpawnData->mPlayableUnits;
			}
		}
	}
	return mPlayerUnitDataCache;
}

/** @brief 선택된 캐릭터 ID가 실제 선택 후보에 포함되어 있는지 검증한다. */
// UI에서 비활성 카드 선택을 막더라도, 런 생성 직전 GameMode가 다시 검증한다.
// 잘못된 PlayerUnitId가 StartRun()으로 들어가면 잘못된 RunPersistData가 만들어질 수 있으므로
// DA_TestFrontend.mPlayableUnits 기준으로 한 번 더 확인한다.
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

/** @brief 선택된 난이도가 새 Run 생성에 사용할 수 있는 값인지 검증한다. */
// 현재는 난이도 선택 기능이 아직 없어서 모든 값을 통과시킨다.
// 추후 난이도 테이블이나 해금 조건이 들어오면 StartNewRun() 직전에 이 함수에서 검증한다.
bool AFrontendGameMode::IsDifficultyValid(int32 Difficulty) const
{
	// [합의필요] 난이도 테이블/해금 규칙이 생기면 StartNewRun() 직전 이 함수에서 검증한다.

	return true;
}

/** @brief 독립 캐릭터 선택 월드 위젯을 열고 타이틀 HUD를 잠시 닫는다. */
// 캐릭터 선택은 타이틀 HUD 내부 ScreenSwitcher 슬롯이 아니라 WorldWidgetSubsystem이 관리하는 별도 위젯이다.
// 그래서 START 입력 시 타이틀 HUD를 닫고 CharacterSelect 월드 위젯을 OpenUI()로 열며,
// Back 요청을 받으면 다시 타이틀 HUD를 OpenUI()로 복귀시킨다.
bool AFrontendGameMode::OpenTitleCharacterSelect()
{
	/*
	 * 캐릭터 선택 화면은 타이틀 HUD와 분리된 독립 월드 위젯이다.
	 * 타이틀 HUD 안에 끼워 넣지 않아야 새 WBP_CharacterSelect_New의 전체 화면 레이아웃과 버튼 배선이 그대로 동작한다.
	 */
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UCharacterSelectWidget* CharacterSelectWidget = WorldWidgetSubsystem->GetWorldWidget<UCharacterSelectWidget>(EWorldWidgetType::CharacterSelect);
	checkf(CharacterSelectWidget != nullptr, TEXT("캐릭터 선택 위젯이 준비가 안됨"));

	CharacterSelectWidget->OnBackToMainRequested.AddUniqueDynamic(this, &AFrontendGameMode::HandleCharacterSelectBackRequested);

	if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
	{
		TitleMenuWidget->CloseUI();
	}

	CharacterSelectWidget->OpenCharacterSelect();
	CharacterSelectWidget->OpenUI();
	return true;
}

/** @brief 독립 캐릭터 선택 위젯의 Back 요청을 타이틀 HUD 복귀로 처리한다. */
void AFrontendGameMode::HandleCharacterSelectBackRequested()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>()
		: nullptr;
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	if (UCharacterSelectWidget* CharacterSelectWidget = WorldWidgetSubsystem->GetWorldWidget<UCharacterSelectWidget>(EWorldWidgetType::CharacterSelect))
	{
		CharacterSelectWidget->CloseUI();
	}

	if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
	{
		TitleMenuWidget->OpenUI();
	}
}

/** @brief 선택된 캐릭터/난이도로 RunPersistData 생성을 GameProfileSubsystem에 위임한다. */
// CreateRunData()는 WBP 상태를 읽거나 화면을 전환하지 않고,
// 캐릭터/난이도 검증 후 GameProfileSubsystem->StartRun()을 호출하는 데 집중한다.
// 현재는 슬롯/닉네임 UI가 아직 없어서 유저 데이터가 없으면 기본 유저를 임시 생성한 뒤 Run을 만든다.
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

