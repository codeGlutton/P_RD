#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "UI/Combat/CombatViewModel.h"

UCombatTileMapHUDWidget::UCombatTileMapHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mIntroDiceRollDuration = 1.10f;
	mIntroDiceHoldDuration = 0.65f;
}

int32 UCombatTileMapHUDWidget::GetCombatDiceViewCount() const
{
	return mDiceViews.Num();
}

bool UCombatTileMapHUDWidget::GetCombatDiceView(int32 DiceIndex, FPrimaryAssetId& OutDiceId, ERarityType& OutRarityType, int32& OutResultValue, bool& OutIsRolled) const
{
	if (mDiceViews.IsValidIndex(DiceIndex) == false)
	{
		return false;
	}

	const FDiceViewData& DiceView = mDiceViews[DiceIndex];
	OutDiceId = DiceView.mDiceId;
	OutRarityType = DiceView.mRarityType;
	OutResultValue = DiceView.mResultValue;
	OutIsRolled = DiceView.mIsRolled;
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

	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
	}

	DestroyDiceCaptureActors(mDicePreviewActors);
	DestroyDiceCaptureActors(mOwnedDicePreviewActors);

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

	UpdateSkillPress(InDeltaTime);
}

void UCombatTileMapHUDWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();

	EnsureRuntimeWidgets();
	RefreshDiceViewsFromRunData();
	RebuildOwnedDiceCards();
	RefreshCombatStatusBar();   // 위젯 생성 이후에 뷰모델 값(HP/Gold/Lv/Round)을 상단 상태바에 채운다.
	RebuildUnitHpBars();        // 유닛 수에 맞춰 머리 위 HP바를 만든다.
	mSelectedDiceIndex = INDEX_NONE;
	mSelectedSkillIndex = INDEX_NONE;
	HideSkillDetail();
	RefreshSkillRailWidgets();
	RefreshDiceAssignmentText();

	/*
	 * HUD 루트는 입력을 통과시키고, 버튼만 입력을 받게 둔다.
	 * 타일맵 조작 API가 붙은 뒤에도 장식/연출 영역이 맵 터치를 막지 않게 하기 위한 기본 상태다.
	 */
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PrepareIntroDiceRoll();
}

int32 UCombatTileMapHUDWidget::GetViewportZOrder() const
{
	return -10;
}
