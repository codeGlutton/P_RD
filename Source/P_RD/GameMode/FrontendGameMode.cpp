#include "GameMode/FrontendGameMode.h"

#include "GameFramework/PlayerController.h"

#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "DataAsset/PrimaryAssetType.h"

#include "DataAsset/StageSpawnData/StageLevelType.h"

#include "PCGStage/Stage.h"

#include "Singleton/InstanceSubsystem/SaveGameSubsystem.h"
#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/TitleMenuWidget.h"
#include "UI/Hire/MercenaryHireWidget.h"

#include "Setting/GamePlaySettings.h"
#include "HAL/IConsoleManager.h"
#include "DataAsset/RoomSpawnData/StaticFrontendRoomSpawnData.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Attack.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_GetActionPoint.h"
#include "DataAsset/UnitSpawnData/StaticPlayerUnitSpawnData.h"

#include "UI/RDUserWidget.h"

#include "AttributeSet/UnitAttributeSet.h"

DEFINE_LOG_CATEGORY(LogFrontendGameMode);

namespace
{
	// [합의필요] 난이도 선택 UI가 들어오기 전까지 새 Run 생성은 프론트엔드 기본 난이도 1로 고정한다.
	constexpr int32 DefaultDifficulty = 1;

	/** @brief 데리고 갈 인원. 여섯 중 셋으로 시작한다. */
	constexpr int32 DefaultPartySize = 3;

	/** @brief UI 표시용 직업명(한글)을 GameMode에서 확정해 WBP가 enum 문자열을 직접 해석하지 않게 한다. */
	FText GetPlayerJobName(EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightNameText", "기사");
		// Archer -> Ranger 개명(#487) 반영. 옛 "도적" 표기는 진짜 도적(Rogue)과
		// 부딪혀서 잠금 카드가 거짓말을 했다(0807 감사).
		case EUnitJobType::Ranger:
			return NSLOCTEXT("FrontendGameMode", "RangerNameText", "레인저");
		case EUnitJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageNameText", "마법사");
		case EUnitJobType::Barbarian:
			return NSLOCTEXT("FrontendGameMode", "BarbarianNameText", "야만전사");
		case EUnitJobType::Rogue:
			return NSLOCTEXT("FrontendGameMode", "RogueNameText", "도적");
		case EUnitJobType::Druid:
			return NSLOCTEXT("FrontendGameMode", "DruidNameText", "드루이드");
		default:
			return NSLOCTEXT("FrontendGameMode", "UnknownNameText", "알 수 없음");
		}
	}

	/** @brief 이름과 구분되는 역할 한 줄 문구. 카드 상세에서 이름 아래 부제로 쓴다. */
	FText GetPlayerJobRole(EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightRoleText", "방패 탱커 · 근접");
		case EUnitJobType::Ranger:
			return NSLOCTEXT("FrontendGameMode", "RangerRoleText", "명사수 · 원거리");
		case EUnitJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageRoleText", "주문 술사 · 원거리");
		case EUnitJobType::Barbarian:
			return NSLOCTEXT("FrontendGameMode", "BarbarianRoleText", "광전사 · 근접");
		case EUnitJobType::Rogue:
			return NSLOCTEXT("FrontendGameMode", "RogueRoleText", "기습 암살자 · 근접");
		case EUnitJobType::Druid:
			return NSLOCTEXT("FrontendGameMode", "DruidRoleText", "자연 술사 · 지원");
		default:
			return FText::GetEmpty();
		}
	}

	/** @brief 직업별 설명 문구(DataAsset 설명이 비어 있을 때 폴백). 설명 스크림에 표시된다. */
	FText GetPlayerJobDescription(EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight:
			return NSLOCTEXT("FrontendGameMode", "KnightDesc", "두꺼운 갑옷과 방패로 전열을 지키는 근접 수호자.\n높은 체력으로 적의 공격을 버틴다.");
		case EUnitJobType::Ranger:
			return NSLOCTEXT("FrontendGameMode", "RangerDesc", "먼 거리에서 활로 적을 솎아내는 명사수.\n가까이 붙기 전에 승부를 낸다.");
		case EUnitJobType::Mage:
			return NSLOCTEXT("FrontendGameMode", "MageDesc", "마법으로 광역 피해를 주는 원거리 술사.\n강력하지만 체력이 낮다.");
		case EUnitJobType::Rogue:
			return NSLOCTEXT("FrontendGameMode", "RogueDesc", "그림자에서 기습하는 민첩한 암살자.\n빠른 연속 공격으로 적을 무너뜨린다.");
		default:
			return FText::GetEmpty();
		}
	}

	/** @brief 실제 DataAsset 후보에 특정 직업이 이미 있는지 확인해 잠금 placeholder 중복 생성을 막는다. */
	bool HasCharacterOptionForJob(const TArray<FFrontendCharacterOption>& Options, EUnitJobType JobType)
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
	void AppendLockedCharacterOption(TArray<FFrontendCharacterOption>& Options, EUnitJobType JobType)
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

	int32 GetClassSelectSortOrder(EUnitJobType JobType)
	{
		switch (JobType)
		{
		case EUnitJobType::Knight:
			return 0;
		case EUnitJobType::Mage:
			return 1;
		case EUnitJobType::Ranger:
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

#if !UE_BUILD_SHIPPING
	// @brief 디버깅용: 새 Run의 시작 스테이지를 대체할 스테이지 레벨 이름 (None 또는 빈 값이면 미사용)
	FString GFixedFirstStageLevelNameForDebugging;

	FAutoConsoleVariableRef CFixedFirstStageLevelNameForDebugging(
		TEXT("Stage.FixedFirstStageLevel"),
		GFixedFirstStageLevelNameForDebugging,
		TEXT("디버깅을 위해서 새 Run의 시작 스테이지를 지정 레벨로 고정 (예: Stage2). None 또는 빈 값이면 미사용"),
		ECVF_Default
	);

	/**
	 * @brief 디버깅용 시작 스테이지 고정 콘솔 변수를 스테이지 레벨로 해석
	 * @param StageLevel [out] 해석된 스테이지 레벨
	 * @return 고정이 켜져 있고 유효한 레벨 이름이면 true
	 */
	bool TryGetFixedFirstStageLevelForDebugging(OUT EStageLevelType& StageLevel)
	{
		if (GFixedFirstStageLevelNameForDebugging.IsEmpty() == true)
		{
			return false;
		}

		// 콘솔 변수 문자열을 스테이지 레벨로 해석 (예: "Stage2"), 잘못된 이름이나 None이면 미사용 처리
		const UEnum* StageLevelEnum = StaticEnum<EStageLevelType>();
		const int64 StageLevelValue = StageLevelEnum != nullptr ? StageLevelEnum->GetValueByNameString(GFixedFirstStageLevelNameForDebugging) : INDEX_NONE;
		if (StageLevelValue == INDEX_NONE ||
			StaticCast<EStageLevelType>(StageLevelValue) == EStageLevelType::None)
		{
			UE_LOG(LogFrontendGameMode, Warning, TEXT("시작 스테이지 고정 미적용. %s은(는) 유효한 스테이지 레벨 아님"), *GFixedFirstStageLevelNameForDebugging);
			return false;
		}

		StageLevel = StaticCast<EStageLevelType>(StageLevelValue);
		return true;
	}
#endif

}

AFrontendGameMode::AFrontendGameMode()
{
	mWorldWidgets = {
		EWorldWidgetType::FadeInOut,
		EWorldWidgetType::LoadingNotify,
		EWorldWidgetType::InGameSettings,
		// 여기에 넣어야 서브시스템이 실제로 만든다. Config 의 클래스 매핑은
		// "무엇을 만들지"만 정하고, 이 목록이 "만들지 말지"를 정한다 -- 매핑만
		// 걸고 여기를 빠뜨려서 START 를 누르는 순간 위젯이 없다고 멈췄다.
		EWorldWidgetType::MercenaryHire,
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

void AFrontendGameMode::InitializeCommonRoom()
{
	Super::InitializeCommonRoom();

	/* 저장 */

	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();

	SaveGameSubsystem->SaveRunAsync(FAsyncSaveGameToSlotDelegate());
	SaveGameSubsystem->SaveUserAsync(FAsyncSaveGameToSlotDelegate());
	SaveGameSubsystem->SaveOptionAsync(FAsyncSaveGameToSlotDelegate());
}

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

	const bool IsCharacterSelectOpened = OpenTitleCharacterSelect();
	checkf(IsCharacterSelectOpened == true, TEXT("캐릭터 선택창 열기 오류"));

	return IsCharacterSelectOpened;
}

bool AFrontendGameMode::StartNewRun(const TArray<FPrimaryAssetId>& PlayerUnitIds, int32 Difficulty)
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

	const bool IsRunDataCreated = CreateRunData(PlayerUnitIds, Difficulty);
	checkf(IsRunDataCreated == true, TEXT("런 데이터 생성 오류"));
	if (IsRunDataCreated == false)
	{
		return false;
	}

	// 새 Run의 시작 스테이지는 Stage1 고정
	EStageLevelType FirstStageLevel = EStageLevelType::Stage1;

#if !UE_BUILD_SHIPPING
	// 디버깅용 시작 스테이지 고정이 켜져 있으면 지정 레벨로 대체 (꺼져 있으면 기존 흐름 그대로)
	if (TryGetFixedFirstStageLevelForDebugging(OUT FirstStageLevel) == true)
	{
		UE_LOG(LogFrontendGameMode, Warning, TEXT("시작 스테이지를 %s로 고정 (Stage.FixedFirstStageLevel)"), *GFixedFirstStageLevelNameForDebugging);
	}
#endif

	const bool IsTransitionStarted = PreloadAndTransitionRoomAsync(FirstStageLevel);
	checkf(IsTransitionStarted == true, TEXT("시작 스테이지 처음 방으로 전환 실패"));
	return IsTransitionStarted;
}

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
	const bool IsTransitionStarted = PreloadAndTransitionRoomAsync(CurrentRowIndex, CurrentColumnIndex);
	checkf(IsTransitionStarted == true, TEXT("이어하기 방 전환 실패"));
	return IsTransitionStarted;
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
		UE_LOG(LogFrontendGameMode, Log, TEXT("포기할 활성 런 데이터가 없음"));
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
		NewOption.mMaxAP = FMath::RoundToInt(LoadedPlayerUnitData->GetDefaultAttributeValue(
			GetWorld(), UPlayerUnitAttributeSet::StaticClass(),
			UUnitAttributeSet::GetRechargeActionPointAttribute(), DefaultDifficulty));
		// 선택 화면의 속도는 라운드마다 충전되는 고유 속도(RechargeSpeedPoint)다.
		// SpeedPoint는 전투 중 누적 자원이라 기본값이 0으로 나올 수 있다.
		NewOption.mSpeed = FMath::RoundToInt(LoadedPlayerUnitData->GetDefaultAttributeValue(
			GetWorld(), UPlayerUnitAttributeSet::StaticClass(),
			UUnitAttributeSet::GetRechargeSpeedPointAttribute(), DefaultDifficulty));
		NewOption.mStatSummary = FText::Format(
			NSLOCTEXT("FrontendGameMode", "CharacterStatSummary", "HP {0} / Gold {1}"),
			FText::AsNumber(NewOption.mMaxHP),
			FText::AsNumber(NewOption.mGold));
		NewOption.mPortrait = LoadedPlayerUnitData->mPortrait;
		NewOption.mIcon = LoadedPlayerUnitData->mIcon;
		// 용병 선택과 전투 카드가 같은 스킬 DA 이름·아이콘을 보여 주도록
		// 게임모드가 실제 후보 데이터에 함께 실어 보낸다.
		for (const TSoftObjectPtr<UStaticSkillData>& SkillPtr : LoadedPlayerUnitData->mSkillDatas)
		{
			const UStaticSkillData* Skill = SkillPtr.LoadSynchronous();
			if (Skill == nullptr)
			{
				continue;
			}
			NewOption.mSkillNames.Add(Skill->mName);
			NewOption.mSkillIcons.Add(Skill->mIcon);

			// 용병 선택에서도 전투 HUD와 같은 롱프레스 상세를 열 수 있게, 이미
			// 로드한 DA의 정적 스펙을 UI 전용 값으로 함께 내려준다. 전투 중에만
			// 생기는 남은 쿨타임/사용 가능 여부는 여기서 만들지 않는다.
			FFrontendSkillOption& SkillOption = NewOption.mSkillDetails.AddDefaulted_GetRef();
			SkillOption.mName = Skill->mName;
			SkillOption.mDescription = Skill->mDescription;
			SkillOption.mIcon = Skill->mIcon;
			SkillOption.mCooldownTurns = FMath::Max(Skill->mCooldownDuration, 0);
			SkillOption.mAimPattern = Skill->mAimPattern;
			SkillOption.mAimRange = Skill->mAimRange;
			SkillOption.mEffectPattern = Skill->mEffectPattern;
			SkillOption.mEffectArea = Skill->mEffectArea;
			SkillOption.mAimBlockerMask = Skill->mAimBlockerMask;
			SkillOption.mEffectBlockerMask = Skill->mEffectBlockerMask;
			SkillOption.mTargetPattern = Skill->mTargetPattern;
			if (const UStaticUnitSkillData* UnitSkill = Cast<UStaticUnitSkillData>(Skill))
			{
				SkillOption.mActionPointCost = FMath::Max(UnitSkill->mRequiredActionPoint, 0);
			}
			for (const FSkillPhaseLayer& MotionLayer : Skill->mSkillPhaseLayers)
			{
				for (const TInstancedStruct<FSkillEffectLayer>& EffectLayer : MotionLayer.mSkillEffectLayers)
				{
					if (const FSkillEffectLayer_Attack* Attack =
						EffectLayer.GetPtr<FSkillEffectLayer_Attack>())
					{
						SkillOption.mDamageMin += Attack->mMinDamage;
						SkillOption.mDamageMax += Attack->mMaxDamage;
					}
					if (const FSkillEffectLayer_GetActionPoint* Gain =
						EffectLayer.GetPtr<FSkillEffectLayer_GetActionPoint>())
					{
						SkillOption.mActionPointGain += Gain->mActionPointGain;
					}
				}
			}
			SkillOption.mCriticalDamage = FMath::RoundToInt(
				SkillOption.mDamageMax * 1.5f);
		}
		NewOption.mPlayerUnitId = LoadedPlayerUnitData->GetPrimaryAssetId();
		// 선택 가능 여부는 로드 상태와 실제 Pawn Class 설정을 동시에 본다. UI는 이 bool을 재해석하지 않는다.
		NewOption.mSelectable = PlayerUnitData.IsValid() && !LoadedPlayerUnitData->mViewClass.IsNull();
		OutOptions.Add(MoveTemp(NewOption));
	}

	// [합의필요] Archer/Mage DataAsset이 준비되면 placeholder 보강을 제거하고 DA_TestFrontend.mPlayableUnits만 단일 출처로 둔다.

	if (!HasCharacterOptionForJob(OutOptions, EUnitJobType::Ranger))
	{
		AppendLockedCharacterOption(OutOptions, EUnitJobType::Ranger);
	}
	if (!HasCharacterOptionForJob(OutOptions, EUnitJobType::Mage))
	{
		AppendLockedCharacterOption(OutOptions, EUnitJobType::Mage);
	}

	// 위 placeholder는 선택 불가 View 데이터라 런 생성으로 이어지지 않는다.
	SortCharacterOptionsForClassSelect(OutOptions);

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
				// FrontendRoom은 전환 전에 프리로드되어 있어야 한다. nullptr이면 설정/번들 계약이 깨진 것이다.
				TObjectPtr<UStaticFrontendRoomSpawnData> FrontendRoomSpawnData = AssetManager->GetPrimaryAssetObject<UStaticFrontendRoomSpawnData>(GamePlaySettings->mFrontendRoomId);
				mPlayerUnitDataCache = FrontendRoomSpawnData->mPlayableUnits;
			}
		}
	}
	return mPlayerUnitDataCache;
}

bool AFrontendGameMode::IsAnyPlayerUnitIdValid(const TArray<FPrimaryAssetId>& PlayerUnitIds) const
{
	/*
	 * Confirm 버튼이 보낸 PlayerUnitId가 실제 프론트엔드 방의 선택 후보 안에 있는지 마지막으로 확인한다.
	 * UI에서 비활성 카드를 막아도, 런 생성 직전 GameMode가 다시 검증해야 잘못된 ID로 StartRun()이 호출되지 않는다.
	 */

	bool IsValid = false;
	for (const FPrimaryAssetId& PlayerUnitId : PlayerUnitIds)
	{
		if (PlayerUnitId.IsValid() == false)
		{
			continue;
		}

		const TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>>& PlayerUnitDatas = GetPlayerUnitDatas();
		IsValid = PlayerUnitDatas.ContainsByPredicate([&PlayerUnitId](const TSoftObjectPtr<UStaticPlayerUnitSpawnData>& PlayerUnitData) {
			const UStaticPlayerUnitSpawnData* LoadedPlayerUnitData = PlayerUnitData.Get();
			if (LoadedPlayerUnitData == nullptr)
			{
				return false;
			}
			return LoadedPlayerUnitData->GetPrimaryAssetId() == PlayerUnitId;
			});

		if (IsValid == true)
		{
			break;
		}
	}

	return IsValid;
}

bool AFrontendGameMode::IsDifficultyValid(int32 Difficulty) const
{
	// [합의필요] 난이도 테이블/해금 규칙이 생기면 StartNewRun() 직전 이 함수에서 검증한다.

	return true;
}

/**
 * @brief 용병 선택 게시판을 열고 타이틀 HUD를 잠시 닫는다.
 *
 * @details
 * 예전에는 한 명만 고르는 캐릭터 선택 화면을 열었다. 런을 여섯 중 셋으로
 * 시작하도록 기획이 바뀌면서 이 자리가 게시판으로 바뀌었다. 캐릭터 선택
 * 위젯은 지우지 않고 두었다 -- 상점이나 도중 합류처럼 한 명만 고르는 자리가
 * 다시 생길 수 있다.
 *
 * 게시판은 값을 모른다. 여기서 후보를 만들어 넘기고, 출발을 누르면 식별자만
 * 돌려받아 런을 만든다.
 */
bool AFrontendGameMode::OpenTitleCharacterSelect()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>();
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	UMercenaryHireWidget* HireWidget = WorldWidgetSubsystem->GetWorldWidget<UMercenaryHireWidget>(EWorldWidgetType::MercenaryHire);
	checkf(HireWidget != nullptr, TEXT("용병 선택 위젯이 준비가 안됨"));

	TArray<FFrontendCharacterOption> Options;
	if (GetCharacterOptions(OUT Options) == false)
	{
		UE_LOG(LogFrontendGameMode, Warning, TEXT("고를 후보가 없음"));
		return false;
	}
	HireWidget->SetCharacterOptions(Options, GetPartySize());

	// 매번 새로 걸지 않는다. 이 화면은 서브시스템이 하나만 만들어 두고 계속
	// 쓰므로, 열 때마다 걸면 출발 한 번에 런이 여러 번 만들어진다.
	if (mWasHireDelegateBound == false)
	{
		HireWidget->mOnPartyConfirmed.AddUObject(this, &AFrontendGameMode::HandlePartyConfirmed);
		HireWidget->mOnBackRequested.AddUObject(this, &AFrontendGameMode::HandleMercenaryHireBackRequested);
		mWasHireDelegateBound = true;
	}

	if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
	{
		TitleMenuWidget->CloseUI();
	}

	HireWidget->OpenUI();
	return true;
}

/**
 * @brief 게시판에서 출발을 눌렀을 때 새 런을 만든다.
 * @param PartyUnitIds 고른 차례대로의 유닛 식별자
 */
void AFrontendGameMode::HandlePartyConfirmed(const TArray<FPrimaryAssetId>& PartyUnitIds)
{
	// [합의필요] 난이도 선택 UI가 들어오기 전까지 기본 난이도로 시작한다.
	if (StartNewRun(PartyUnitIds, DefaultDifficulty) == false)
	{
		UE_LOG(LogFrontendGameMode, Warning, TEXT("파티로 런을 시작하지 못함"));
	}
}

/** @brief 데리고 갈 인원. */
int32 AFrontendGameMode::GetPartySize() const
{
	// [합의필요] 인원이 바뀔 수 있으면 방 데이터나 밸런스 표로 옮긴다.
	return DefaultPartySize;
}

void AFrontendGameMode::HandleMercenaryHireBackRequested()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>()
		: nullptr;
	checkf(WorldWidgetSubsystem != nullptr, TEXT("월드 위젯 서브시스템 nullptr"));

	if (UMercenaryHireWidget* HireWidget = WorldWidgetSubsystem->GetWorldWidget<UMercenaryHireWidget>(EWorldWidgetType::MercenaryHire))
	{
		HireWidget->CloseUI();
	}

	if (UTitleMenuWidget* TitleMenuWidget = WorldWidgetSubsystem->GetHUD<UTitleMenuWidget>())
	{
		TitleMenuWidget->OpenUI();
	}
}

bool AFrontendGameMode::CreateRunData(const TArray<FPrimaryAssetId>& PlayerUnitIds, int32 Difficulty)
{
	/*
	 * RunPersistData 생성은 캐릭터/난이도 검증이 끝난 뒤 GameProfileSubsystem에 위임한다.
	 * 이 함수가 UI DTO나 WBP 상태를 읽지 않는 이유는, 런 생성 규칙을 화면 구조와 분리하기 위해서다.
	 */
	if (IsAnyPlayerUnitIdValid(PlayerUnitIds) == false)
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

	GameProfileSubsystem->StartRun(PlayerUnitIds, Difficulty);
	return true;
}

