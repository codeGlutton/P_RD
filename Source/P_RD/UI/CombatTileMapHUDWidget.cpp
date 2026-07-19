#include "UI/CombatTileMapHUDWidget.h"

#include "Actor/Dice/CombatDiceCaptureActor.h"   // NativeDestruct에서 공유 캡처 액터 파괴
#include "Components/Button.h"
#include "TimerManager.h"   // NativeDestruct에서 골드 카운트업/결과 딜레이 타이머 명시 정리
#include "UI/Combat/CombatUIModel.h"
#include "UI/Reward/RewardUIWidgetBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"

#include "GameMode/CombatGameMode.h"

UCombatTileMapHUDWidget::UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 드래그 중 원본 유닛의 메시/포즈를 복제해 보여주는 시각 전용 반투명 재질.
	// 엔진 콘텐츠 하드레퍼런스라 별도 프로젝트 플러그인이나 신규 에셋이 필요하지 않다.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DragGhostMaterialFinder(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	if (DragGhostMaterialFinder.Succeeded())
	{
		mDirectUnitGestureGhostMaterial = DragGhostMaterialFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> DiceRollBoardFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/DiceRoll/UI_DiceRoll_Board_StyleMatch.UI_DiceRoll_Board_StyleMatch"));
	if (DiceRollBoardFinder.Succeeded())
	{
		mDiceRollBoardTexture = DiceRollBoardFinder.Object;
	}

	// 유닛 HP바 왼쪽 방어도 아이콘(SVN uasset) 하드레퍼런스 프리로드 → 쿡 보장(#300 컨벤션).
	static ConstructorHelpers::FObjectFinder<UTexture2D> UnitDefenseIconFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_UnitHpBar_Defense_Icon.T_UnitHpBar_Defense_Icon"));
	if (UnitDefenseIconFinder.Succeeded())
	{
		mUnitDefenseIconTexture = UnitDefenseIconFinder.Object;
	}

	// 골드 카운트업 코인 사운드 + 승리/패배 결과 징글(SVN uasset)을 하드레퍼런스로 프리로드 → 쿡 보장(#300 컨벤션).
	static ConstructorHelpers::FObjectFinder<USoundBase> CoinGainSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/Coin/SFX_CoinGain_OGA_CC0_11.SFX_CoinGain_OGA_CC0_11"));
	if (CoinGainSoundFinder.Succeeded())
	{
		mCoinGainSound = CoinGainSoundFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> VictoryJingleFinder(TEXT("/Game/SVN/OutSideAsset/Music/OpenGameArt/Jingle/BGM_Jingle_Victory_Fupi_CC0.BGM_Jingle_Victory_Fupi_CC0"));
	if (VictoryJingleFinder.Succeeded())
	{
		mVictoryJingleSound = VictoryJingleFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> DefeatJingleFinder(TEXT("/Game/SVN/OutSideAsset/Music/OpenGameArt/Jingle/BGM_Jingle_Defeat_Spuispuin_CCBY4.BGM_Jingle_Defeat_Spuispuin_CCBY4"));
	if (DefeatJingleFinder.Succeeded())
	{
		mDefeatJingleSound = DefeatJingleFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> MapOpenSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/UI/SFX_MapOpen_OGA_CC0_BookFlip3.SFX_MapOpen_OGA_CC0_BookFlip3"));
	if (MapOpenSoundFinder.Succeeded())
	{
		mMapOpenSound = MapOpenSoundFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> DiceChooseSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/UI/SFX_DiceChoose_OGA_CC0_WoodDice2.SFX_DiceChoose_OGA_CC0_WoodDice2"));
	if (DiceChooseSoundFinder.Succeeded())
	{
		mDiceChooseSound = DiceChooseSoundFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> SkillSelectSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/UI/SFX_SkillSelect_OGA_CC0_QuickslotClear.SFX_SkillSelect_OGA_CC0_QuickslotClear"));
	if (SkillSelectSoundFinder.Succeeded())
	{
		mSkillSelectSound = SkillSelectSoundFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> ExpGainSoundFinder(TEXT("/Game/SVN/OutSideAsset/SFX/OpenGameArt_CC0/UI/SFX_XPGain_OGA_CC0_Rise03.SFX_XPGain_OGA_CC0_Rise03"));
	if (ExpGainSoundFinder.Succeeded())
	{
		mExpGainSound = ExpGainSoundFinder.Object;
	}
	// 플로팅 로그 종류별 아이콘 (SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons). HP는 피해/회복으로 갈린다.
#define RD_LOAD_LOGICON(Member, Path) \
	{ static ConstructorHelpers::FObjectFinder<UTexture2D> Finder(TEXT(Path)); if (Finder.Succeeded()) { Member = Finder.Object; } }
	RD_LOAD_LOGICON(mLogIconHpDamage,      "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_HP_Damage.T_Status_HP_Damage");
	RD_LOAD_LOGICON(mLogIconHpRecovery,    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_HP_Recovery.T_Status_HP_Recovery");
	RD_LOAD_LOGICON(mLogIconGetMove,       "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetMove.T_Status_GetMove");
	RD_LOAD_LOGICON(mLogIconGetDefense,    "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_GetDefense.T_Status_GetDefense");
	RD_LOAD_LOGICON(mLogIconAgility,       "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Agility.T_Status_Agility");
	RD_LOAD_LOGICON(mLogIconFortification, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Fortification.T_Status_Fortification");
	RD_LOAD_LOGICON(mLogIconVulnerability, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Vulnerability.T_Status_Vulnerability");
	RD_LOAD_LOGICON(mLogIconWeakness,      "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/StatusIcons/T_Status_Weakness.T_Status_Weakness");
#undef RD_LOAD_LOGICON

	static ConstructorHelpers::FClassFinder<UUserWidget> DetailOverlayClassFinder(TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay"));
	if (DetailOverlayClassFinder.Succeeded())
	{
		mDetailOverlayClass = DetailOverlayClassFinder.Class;
	}

	// 유닛 머리 위 HP바 WBP 및 채움 텍스처(빨강=적, 초록=아군)를 하드 레퍼런스로 프리로드한다(#300 컨벤션 — ini 강제쿡 대신).
	static ConstructorHelpers::FClassFinder<UUserWidget> UnitHpBarClassFinder(TEXT("/Game/UI/CombatHUD/UnitHpBar/WBP_CombatUnitHpBar"));
	if (UnitHpBarClassFinder.Succeeded())
	{
		mUnitHpBarWidgetClass = UnitHpBarClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<URewardUIWidgetBase> RewardWidgetClassFinder(TEXT("/Game/BP/UI/WBP_Reward"));
	if (RewardWidgetClassFinder.Succeeded())
	{
		mRewardWidgetClass = RewardWidgetClassFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> UnitHpFillRedFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_CombatHUD_UnitHpBar_Fill_Red.T_CombatHUD_UnitHpBar_Fill_Red"));
	if (UnitHpFillRedFinder.Succeeded())
	{
		mUnitHpFillRedTexture = UnitHpFillRedFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> UnitHpFillGreenFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/UnitHpBar/T_CombatHUD_UnitHpBar_Fill_Green.T_CombatHUD_UnitHpBar_Fill_Green"));
	if (UnitHpFillGreenFinder.Succeeded())
	{
		mUnitHpFillGreenTexture = UnitHpFillGreenFinder.Object;
	}
}

void UCombatTileMapHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 첫 라운드 배너에서 프레임 텍스처를 동기 로드하지 않도록 전투 진입 시점에 미리 올린다.
	PreloadTurnChangeFrameTextures();

	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		ACombatGameMode* GameMode = World->GetAuthGameMode<ACombatGameMode>();
		if (GameMode != nullptr)
		{
			BindCombatUIModel(GameMode->GetCombatUIModel());
		}
	}
}

void UCombatTileMapHUDWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	Super::OpenUI(Callback);

	// 이미 어댑터가 push해 둔 현재 값을 즉시 반영(구독 전 발생분 보강).
	RefreshCombatStatusBar();
}

int32 UCombatTileMapHUDWidget::GetCombatDiceViewCount() const
{
	return mDiceUIs.Num();
}

bool UCombatTileMapHUDWidget::GetCombatDiceView(int32 DiceIndex, FDiceViewData& OutDiceView) const
{
	if (mDiceUIs.IsValidIndex(DiceIndex) == false)
	{
		return false;
	}

	OutDiceView = mDiceUIs[DiceIndex];
	return true;
}

void UCombatTileMapHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureRuntimeWidgets();

	if (EndTurnButton != nullptr)
	{
		EndTurnButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleEndTurnButtonClicked);
	}
	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
	}
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked);
	}
	if (mEnemyIntentTutorialContinueButton != nullptr)
	{
		mEnemyIntentTutorialContinueButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleEnemyIntentTutorialContinue);
	}
}

void UCombatTileMapHUDWidget::NativeDestruct()
{
	DestroyDirectUnitGestureGhost();

	if (EndTurnButton != nullptr)
	{
		EndTurnButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleEndTurnButtonClicked);
	}
	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
	}
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked);
	}
	if (mEnemyIntentTutorialContinueButton != nullptr)
	{
		mEnemyIntentTutorialContinueButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleEnemyIntentTutorialContinue);
	}
	if (mCombatUIModel != nullptr)
	{
		// BindCombatUIModel이 구독하는 7종 전부를 대칭 해제한다(다이나믹 델리게이트의 약참조
		// 시맨틱 덕에 해제 없이도 사고는 안 나지만, 안전성을 시맨틱에 기대지 않게 명시 정리).
		mCombatUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		mCombatUIModel->OnUIChanged.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatUIChanged);
		mCombatUIModel->OnActionResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatActionResolved);
		mCombatUIModel->OnCombatFloatingLog.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLog);
		mCombatUIModel->OnCombatFloatingLogMotionFinished.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLogMotionFinished);
		mCombatUIModel->OnCombatFloatingLogsCleared.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLogsCleared);
		mCombatUIModel->OnDiceRollRequested.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatDiceRollRequested);
	}

	// UObject 바인딩 타이머라 파괴 후 발화하진 않지만, 핸들을 명시적으로 거둬 수명을 코드로 드러낸다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mGoldCountUpTimerHandle);
		World->GetTimerManager().ClearTimer(mCombatResultStartDelayTimerHandle);
	}

	if (IsValid(mOwnedDiceSharedCaptureActor))
	{
		mOwnedDiceSharedCaptureActor->Destroy();
	}
	mOwnedDiceSharedCaptureActor = nullptr;

	// 보유 주사위 캡처 액터와 달리 물리 굴림 캡처 액터는 정리가 빠져 있었다 — 위젯 재생성 시
	// 은닉 위치(Y≈30000)에 SceneCapture+RenderTarget 액터가 누적되는 누수의 원인.
	DestroyDiceRollPhysicsActor();

	Super::NativeDestruct();
}

void UCombatTileMapHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 뷰포트 크기가 바뀔 때만 레이아웃 진단 로그를 갱신한다.
	const FVector2D ViewportSize = MyGeometry.GetLocalSize();
	if (ViewportSize.Equals(mLastLoggedLayoutViewportSize, 0.5f) == false)
	{
		mLastLoggedLayoutViewportSize = ViewportSize;
		LogCombatLayoutMetrics(ViewportSize);
	}

	UpdateUnitHpBars();   // 유닛 머리 위 HP바를 월드→스크린 투영으로 매 프레임 따라가게 한다.
	UpdateContextActionLayout(); // 선택한 유닛 옆 행동 팔레트도 모델/카메라 이동을 따라간다.
	UpdateDisplacementPreviewVisuals(InDeltaTime); // 대상·방향·착지·충돌을 바닥 셀 대신 월드 위에 직접 표시한다.
	UpdateEnemyIntentTutorialVisuals(InDeltaTime); // HP바 위치가 정해진 뒤 실제 스킬/주사위/적/턴 종료에 포커스를 붙인다.
	UpdateFloatingCombatLogQueue(InDeltaTime); // 대기 중인 전투 로그를 순서대로 하나씩 스폰한다.
	UpdateFloatingCombatLogs(InDeltaTime); // 머리 위 전투 로그(HP 증감 텍스트) 상승+페이드.
	UpdateTurnRoundBanner(InDeltaTime);    // "N번째 턴" 배너 페이드.

	if (mIntroDiceRollActive)
	{
		UpdateIntroDiceRoll(InDeltaTime);
	}
	if (mTurnChangeIntroPlaying)
	{
		UpdateTurnChangeIntro(InDeltaTime);
	}

	UpdateTopBarBackdrop();
	UpdateSkillPress(InDeltaTime);
	UpdateEquipPress(InDeltaTime);
}

void UCombatTileMapHUDWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();

	EnsureRuntimeWidgets();
	mLevelValueTouched = false;
	SetExpHoldPanelVisible(false);
	RefreshDiceViewsFromRunData();
	RebuildOwnedDiceCards();
	RefreshCombatStatusBar();   // 위젯 생성 이후에 뷰모델 값(Lv/HP/Gold)을 상단 상태바에 채운다.
	RebuildEquipmentBar();      // 탑바 좌측 하단 장비 칩.
	RebuildUnitHpBars();        // 유닛 수에 맞춰 머리 위 HP바를 만든다.
	ClearOwnedDiceSelectionHighlight();
	mSelectedSkillIndex = INDEX_NONE;
	CloseContextActions();
	HideSkillDetail();
	RefreshSkillRailWidgets();
	RefreshDiceAssignmentText();
	RefreshEnemyIntentPanel();   // 구독 전에 push된 라운드 계획도 HUD 재오픈 시 즉시 복원한다.
	RefreshDisplacementPreview();
	UpdateEnemyIntentTutorial();

	/*
	 * HUD 루트가 빈 영역(타일맵) 탭도 받아야 NativeOnTouchStarted에서 RequestWorldTouch로
	 * 좌표를 게임플레이에 넘길 수 있다. 스킬레일/주사위 같은 자식 버튼은 여전히 우선 처리되고,
	 * 버튼 밖(월드) 탭만 루트가 받아 좌표를 전달한다. (SelfHitTestInvisible이면 월드 탭이 통과돼버림)
	 */
	SetVisibility(ESlateVisibility::Visible);
}

int32 UCombatTileMapHUDWidget::GetViewportZOrder() const
{
	return -10;
}
