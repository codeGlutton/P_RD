#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "GameMode/CombatGameMode.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UI/Combat/CombatUIModel.h"

UCombatTileMapHUDWidget::UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mIntroDiceRollDuration = 1.10f;
	mIntroDiceHoldDuration = 0.65f;
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
	BindCombatGameModeDelegates();
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
	if (mMapNavButton != nullptr)
	{
		mMapNavButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleMapNavButtonClicked);
	}
	if (mDiceNavButton != nullptr)
	{
		mDiceNavButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleDiceNavButtonClicked);
	}
	if (mSkillNavButton != nullptr)
	{
		mSkillNavButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSkillNavButtonClicked);
	}
	if (mSettingsNavButton != nullptr)
	{
		mSettingsNavButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSettingsNavButtonClicked);
	}
	if (UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetCombatFloatingPanel(EWorldWidgetType::WorldMap)))
	{
		WorldMapWidget->OnCloseRequested.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleWorldMapCloseRequested);
	}
	if (USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetCombatFloatingPanel(EWorldWidgetType::InGameSettings)))
	{
		SettingsPanelWidget->OnBackRequested.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSettingsPanelBackRequested);
	}
	if (mSkillDetailCloseButton != nullptr)
	{
		mSkillDetailCloseButton->OnClicked.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailCloseButtonClicked);
	}
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
	}
	UnbindCombatGameModeDelegates();

	DestroyDiceCaptureActors(mDicePreviewActors);
	DestroyDiceCaptureActors(mOwnedDicePreviewActors);
	DestroyDiceRollPhysicsActor();

	Super::NativeDestruct();
}

void UCombatTileMapHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshDicePreviewActors();
	RefreshOwnedDiceCards();
	UpdateUnitHpBars();   // 유닛 머리 위 HP바를 월드→스크린 투영으로 매 프레임 따라가게 한다.

	if (mIntroDiceRollActive)
	{
		UpdateIntroDiceRoll(InDeltaTime);
	}

	UpdateShakeToRoll(InDeltaTime);
	UpdateSkillPress(InDeltaTime);
	UpdateWorldPress(InDeltaTime);
}

void UCombatTileMapHUDWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();

	EnsureRuntimeWidgets();
	RefreshDiceViewsFromUIModel();
	RebuildOwnedDiceCards();
	RefreshCombatStatusBar();   // 위젯 생성 이후에 뷰모델 값(Lv/HP/Gold)을 상단 상태바에 채운다.
	RebuildTurnOrderBar();      // 전투 HUD 가운데 하단 턴 순서 칩.
	RebuildUnitHpBars();        // 유닛 수에 맞춰 머리 위 HP바를 만든다.
	mSelectedDiceIndex = INDEX_NONE;
	mSelectedSkillIndex = INDEX_NONE;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressing = false;
	mSkillLongPressConsumed = false;
	mSkillPressElapsed = 0.0f;
	HideSkillDetailPanel();
	RefreshSkillRailWidgets();
	RefreshDiceAssignmentText();

	/*
	 * HUD 루트가 빈 영역(타일맵) 탭도 받아야 NativeOnTouchStarted에서 CombatGameMode의
	 * 월드 터치 명령을 호출할 수 있다. 스킬레일/주사위 같은 자식 버튼은 여전히 우선 처리되고,
	 * 버튼 밖(월드) 탭만 루트가 받는다. (SelfHitTestInvisible이면 월드 탭이 통과돼버림)
	 */
	SetVisibility(ESlateVisibility::Visible);
	PrepareIntroDiceRoll();
}

int32 UCombatTileMapHUDWidget::GetViewportZOrder() const
{
	return -10;
}
