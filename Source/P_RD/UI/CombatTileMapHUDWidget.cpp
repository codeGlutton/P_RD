#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "UI/Combat/CombatUIModel.h"
#include "UObject/ConstructorHelpers.h"

#include "GameMode/CombatGameMode.h"

namespace
{
	constexpr float InitialTurnChangeIntroDelaySeconds = 0.35f;
}

UCombatTileMapHUDWidget::UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> DiceRollBoardFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/DiceRoll/UI_DiceRoll_Board_StyleMatch.UI_DiceRoll_Board_StyleMatch"));
	if (DiceRollBoardFinder.Succeeded())
	{
		mDiceRollBoardTexture = DiceRollBoardFinder.Object;
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
	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
	}
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked);
	}
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
	}

	DestroyDiceCaptureActors(mOwnedDicePreviewActors);

	Super::NativeDestruct();
}

void UCombatTileMapHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 뷰포트 크기가 바뀔 때만 레이아웃 진단 로그 + 화면비 변형(폴드 마커 기반, 마커 없으면 no-op)을 갱신한다.
	const FVector2D ViewportSize = MyGeometry.GetLocalSize();
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
	if (mPendingInitialTurnChangeIntro == true)
	{
		mPendingInitialTurnChangeIntroDelay = FMath::Max(0.0f, mPendingInitialTurnChangeIntroDelay - InDeltaTime);
		if (mPendingInitialTurnChangeIntroDelay <= 0.0f
			&& mTurnChangeIntroPlaying == false
			&& mIntroDiceRollActive == false
			&& mIntroDiceRollReady == false
			&& mIntroDiceResultWaitingForDismiss == false)
		{
			mPendingInitialTurnChangeIntro = false;
			PlayTurnChangeIntro(true);
		}
	}

	UpdateTopBarBackdrop();
	UpdateSkillPress(InDeltaTime);
	UpdateEquipPress(InDeltaTime);
}

void UCombatTileMapHUDWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();

	EnsureRuntimeWidgets();
	RefreshDiceViewsFromRunData();
	RebuildOwnedDiceCards();
	RefreshCombatStatusBar();   // 위젯 생성 이후에 뷰모델 값(Lv/HP/Gold)을 상단 상태바에 채운다.
	RebuildEquipmentBar();      // 탑바 좌측 하단 장비 칩.
	RebuildTurnOrderBar();      // 탑바 가운데 하단 턴 순서 칩.
	RebuildUnitHpBars();        // 유닛 수에 맞춰 머리 위 HP바를 만든다.
	ClearOwnedDiceSelectionHighlight();
	mSelectedSkillIndex = INDEX_NONE;
	HideSkillDetail();
	RefreshSkillRailWidgets();
	RefreshDiceAssignmentText();

	/*
	 * HUD 루트가 빈 영역(타일맵) 탭도 받아야 NativeOnTouchStarted에서 RequestWorldTouch로
	 * 좌표를 게임플레이에 넘길 수 있다. 스킬레일/주사위 같은 자식 버튼은 여전히 우선 처리되고,
	 * 버튼 밖(월드) 탭만 루트가 받아 좌표를 전달한다. (SelfHitTestInvisible이면 월드 탭이 통과돼버림)
	 */
	SetVisibility(ESlateVisibility::Visible);
	mPendingInitialTurnChangeIntro = true;
	mPendingInitialTurnChangeIntroDelay = InitialTurnChangeIntroDelaySeconds;
}

int32 UCombatTileMapHUDWidget::GetViewportZOrder() const
{
	return -10;
}
