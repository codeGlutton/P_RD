#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "UI/Combat/CombatUIModel.h"

UCombatTileMapHUDWidget::UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
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

	RefreshOwnedDiceCards();
	UpdateUnitHpBars();   // 유닛 머리 위 HP바를 월드→스크린 투영으로 매 프레임 따라가게 한다.

	if (mIntroDiceRollActive)
	{
		UpdateIntroDiceRoll(InDeltaTime);
	}

	UpdateShakeToRoll(InDeltaTime);
	UpdateSkillPress(InDeltaTime);
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
	PrepareIntroDiceRoll();
}

int32 UCombatTileMapHUDWidget::GetViewportZOrder() const
{
	return -10;
}
