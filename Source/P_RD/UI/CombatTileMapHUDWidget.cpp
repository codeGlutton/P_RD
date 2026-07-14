#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/ScaleBox.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Reward/RewardUIWidgetBase.h"
#include "UObject/ConstructorHelpers.h"

#include "GameMode/CombatGameMode.h"

UCombatTileMapHUDWidget::UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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
	if (mCancelActionButton != nullptr)
	{
		mCancelActionButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCancelActionButtonClicked);
	}
	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
	}
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked);
	}
}

void UCombatTileMapHUDWidget::NativeDestruct()
{
	if (EndTurnButton != nullptr)
	{
		EndTurnButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleEndTurnButtonClicked);
	}
	if (mCancelActionButton != nullptr)
	{
		mCancelActionButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCancelActionButtonClicked);
	}
	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
	}
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked);
	}
	if (mSkillDrawerHandleButton != nullptr)
	{
		mSkillDrawerHandleButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDrawerToggleClicked);
	}
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		mCombatUIModel->OnUnitDetailReady.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatUnitDetailReady);
	}

	DestroyDiceCaptureActors(mOwnedDicePreviewActors);

	Super::NativeDestruct();
}

void UCombatTileMapHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ScaleBox가 받은 Safe Area의 실제 가용 크기로 프로필을 고른다. DesignCanvas는 이 화면비에
	// 맞춰 논리 높이가 달라지므로 화면비 판정에 다시 사용하면 안 된다.
	FVector2D ViewportSize = MyGeometry.GetLocalSize();
	if (mCombatDesignScaleBox != nullptr)
	{
		const FVector2D SafeAreaSize = mCombatDesignScaleBox->GetCachedGeometry().GetLocalSize();
		if (SafeAreaSize.X > 1.0f && SafeAreaSize.Y > 1.0f)
		{
			ViewportSize = SafeAreaSize;
		}
	}
	if (ViewportSize.Equals(mLastLoggedLayoutViewportSize, 0.5f) == false)
	{
		mLastLoggedLayoutViewportSize = ViewportSize;
		LogCombatLayoutMetrics(ViewportSize);
		ApplyAspectVariantSlots(ViewportSize);
	}

	UpdateUnitHpBars();   // 유닛 머리 위 HP바를 월드→스크린 투영으로 매 프레임 따라가게 한다.
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
	UpdateSkillDrawerAnimation(InDeltaTime);
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
	if (IsDesignerSkinActive() == false)
	{
		RefreshRoomNameLabel();
		RebuildTurnOrderBar();
	}
	RebuildEquipmentBar();      // 탑바 좌측 하단 장비 칩.
	RebuildUnitHpBars();        // 유닛 수에 맞춰 머리 위 HP바를 만든다.
	ClearOwnedDiceSelectionHighlight();
	mSelectedSkillIndex = INDEX_NONE;
	HideSkillDetail();
	RefreshSkillRailWidgets();
	RefreshDiceAssignmentText();
	RefreshActionControls();

	/*
	 * LV 홀드와 스킬 레일 폴백 입력을 위해 루트는 Visible을 유지한다.
	 * 빈 월드 영역은 NativeOnMouse/TouchStarted가 Unhandled로 반환해 CombatCameraPawn으로 내려보낸다.
	 */
	SetVisibility(ESlateVisibility::Visible);
}

int32 UCombatTileMapHUDWidget::GetViewportZOrder() const
{
	return -10;
}
