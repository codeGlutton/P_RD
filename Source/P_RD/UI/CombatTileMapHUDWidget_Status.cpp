#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameMode/CombatGameMode.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/UIRuntimeLayout.h"

void UCombatTileMapHUDWidget::BindCombatGameModeDelegates()
{
	ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr;
	if (CombatGameMode == nullptr)
	{
		return;
	}

	UnbindCombatGameModeDelegates();

	/*
	 * Combat HUD 구독 범위.
	 *
	 * HUD는 전투 조작/전투 보드 표시만 구독한다.
	 * 장비 슬롯은 TopMenuBar 소유라서 OnRefreshEquipmentUI / OnShowEquipmentDetailPanelUI를 여기서 구독하지 않는다.
	 * 신호를 받으면 payload를 신뢰하는 방식이 아니라 UCombatUIModel을 다시 읽어 필요한 영역만 redraw한다.
	 */
	CombatGameMode->OnRefreshAllUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshAllUI);
	CombatGameMode->OnRefreshUnitUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshUnitUI);
	CombatGameMode->OnRefreshDiceUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshDiceUI);
	CombatGameMode->OnRefreshSelectedDiceUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshSelectedDiceUI);
	CombatGameMode->OnRefreshSkillUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshSkillUI);
	CombatGameMode->OnRefreshTurnUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshTurnUI);
	CombatGameMode->OnRefreshPlayerMetaUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshPlayerMetaUI);
	CombatGameMode->OnRefreshSkillBuildPhase.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshSkillBuildPhase);
	CombatGameMode->OnRefreshMoveBuildPhase.AddUObject(this, &UCombatTileMapHUDWidget::HandleRefreshMoveBuildPhase);
	CombatGameMode->OnCombatActionResolvedUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleCombatActionResolvedUI);
	CombatGameMode->OnShowDicePanelAnyTurnUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleShowDicePanelAnyTurn);
	CombatGameMode->OnShowTargetDetailPanelUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleShowTargetDetailPanel);
	CombatGameMode->OnShowSkillDetailPanelUI.AddUObject(this, &UCombatTileMapHUDWidget::HandleShowSkillDetailPanel);
}

void UCombatTileMapHUDWidget::UnbindCombatGameModeDelegates()
{
	ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr;
	if (CombatGameMode == nullptr)
	{
		return;
	}

	CombatGameMode->OnRefreshAllUI.RemoveAll(this);
	CombatGameMode->OnRefreshUnitUI.RemoveAll(this);
	CombatGameMode->OnRefreshDiceUI.RemoveAll(this);
	CombatGameMode->OnRefreshSelectedDiceUI.RemoveAll(this);
	CombatGameMode->OnRefreshSkillUI.RemoveAll(this);
	CombatGameMode->OnRefreshTurnUI.RemoveAll(this);
	CombatGameMode->OnRefreshPlayerMetaUI.RemoveAll(this);
	CombatGameMode->OnRefreshSkillBuildPhase.RemoveAll(this);
	CombatGameMode->OnRefreshMoveBuildPhase.RemoveAll(this);
	CombatGameMode->OnCombatActionResolvedUI.RemoveAll(this);
	CombatGameMode->OnShowDicePanelAnyTurnUI.RemoveAll(this);
	CombatGameMode->OnShowTargetDetailPanelUI.RemoveAll(this);
	CombatGameMode->OnShowSkillDetailPanelUI.RemoveAll(this);
}

void UCombatTileMapHUDWidget::HandleRefreshAllUI()
{
	// 전체 스냅샷을 다시 읽는 broad refresh. 초기 HUD open 직후와 큰 상태 변경에서 사용한다.
	RefreshCombatStatusBar();
	RebuildTurnOrderBar();
	RebuildUnitHpBars();
	RefreshDiceViewsFromUIModel();
	RebuildOwnedDiceCards();
	RebuildSkillRailWidgets();
	RefreshSkillRailWidgets();
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::HandleRefreshUnitUI()
{
	RefreshCombatStatusBar();
	RebuildTurnOrderBar();
	RebuildUnitHpBars();
	RefreshMoveButton();
}

void UCombatTileMapHUDWidget::HandleRefreshDiceUI()
{
	RefreshDiceViewsFromUIModel();
	RebuildOwnedDiceCards();
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::HandleRefreshSelectedDiceUI()
{
	RefreshDiceViewsFromUIModel();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::HandleRefreshSkillUI()
{
	RebuildSkillRailWidgets();
	RefreshSkillRailWidgets();
	RefreshCombatStatusBar();
}

void UCombatTileMapHUDWidget::HandleRefreshTurnUI()
{
	RefreshCombatStatusBar();
	RebuildTurnOrderBar();
	RefreshMoveButton();
}

void UCombatTileMapHUDWidget::HandleRefreshPlayerMetaUI()
{
	RefreshCombatStatusBar();
}

void UCombatTileMapHUDWidget::HandleRefreshSkillBuildPhase(ECombatBuildPhaseUI Phase)
{
	// 스킬 phase는 UI enum만 받는다. HUD는 SRPGSkillBuildPhase/커맨드 객체를 몰라도 된다.
	if (Phase == ECombatBuildPhaseUI::None)
	{
		HandleCombatActionResolved();
		return;
	}

	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

void UCombatTileMapHUDWidget::HandleRefreshMoveBuildPhase(ECombatBuildPhaseUI Phase)
{
	// MOVE는 스킬과 독립된 행동이다. phase에 따라 MOVE/CANCEL 표시만 바꾼다.
	mMoveBuildPhase = Phase;
	RefreshMoveButton();

	if (Phase == ECombatBuildPhaseUI::None)
	{
		HandleCombatActionResolved();
	}
}

void UCombatTileMapHUDWidget::HandleCombatActionResolvedUI()
{
	HandleCombatActionResolved();
}

void UCombatTileMapHUDWidget::HandleShowTargetDetailPanel()
{
	UE_LOG(LogRD, Log, TEXT("Combat HUD target detail requested."));
}

void UCombatTileMapHUDWidget::HandleShowSkillDetailPanel(int32 SkillIndex)
{
	ShowSkillDetailPanel(SkillIndex);
}

void UCombatTileMapHUDWidget::RefreshCombatStatusBar() const
{
	if (mCombatStatusBarText == nullptr)
	{
		return;
	}

	if (mCombatUIModel == nullptr)
	{
		// 뷰모델 미연결(시안 단독)에서는 상태바를 비워 둔다.
		mCombatStatusBarText->SetText(FText::GetEmpty());
		return;
	}

	// 플레이어 유닛 HP는 유닛 뷰에서, 골드/레벨은 메타에서 읽는다(전부 뷰모델 경유).
	float PlayerHP = 0.f;
	float PlayerMaxHP = 0.f;
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer)
		{
			PlayerHP = Unit.mHP;
			PlayerMaxHP = Unit.mMaxHP;
			break;
		}
	}

	const FPlayerMetaUI& Meta = mCombatUIModel->GetPlayerMeta();
	const int32 Level = Meta.mLevel;
	const int32 HP = FMath::RoundToInt(PlayerHP);
	const int32 MaxHP = FMath::RoundToInt(PlayerMaxHP);
	const int32 Gold = Meta.mGold;

	// 로컬 상태줄에도 값은 채워두고, 디자이너 스킨 값 텍스트는 RefreshSkinValueLabels에서 갱신한다.
	mCombatStatusBarText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "CombatStatusBarFormat", "Lv {0}    HP {1}/{2}    GOLD {3}"),
		FText::AsNumber(Level), FText::AsNumber(HP), FText::AsNumber(MaxHP), FText::AsNumber(Gold)));

	RefreshMoveButton();
	RefreshSkinValueLabels();
}

void UCombatTileMapHUDWidget::RefreshSkinValueLabels() const
{
	// 값 텍스트(LV/HP/Gold)의 위치·폰트·정렬은 WBP가 소유한다(빌드가 HUD_M_lv/hp/gold_value를 실제
	// TextBlock으로 심음). C++는 동적으로 변하는 '내용'만 채운다 — 런타임 좌표 계산/재부모화 핵 없음.
	if (IsDesignerSkinActive() == false || mCombatUIModel == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	float PlayerHP = 0.f;
	float PlayerMaxHP = 0.f;
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer)
		{
			PlayerHP = Unit.mHP;
			PlayerMaxHP = Unit.mMaxHP;
			break;
		}
	}
	const FPlayerMetaUI& Meta = mCombatUIModel->GetPlayerMeta();

	struct FSkinValueBinding
	{
		const TCHAR* WidgetName;
		FText Text;
	};
	// 빌드는 빈 TextBlock(color/font/slot만)으로 마커를 심는다(Android 직렬화 안전). 내용/정렬은 여기서 채운다.
	// END TURN 라벨은 정적이지만 동일하게 C++가 설정(CDO에 FText를 넣지 않기 위함).
	const FSkinValueBinding Bindings[] = {
		{ TEXT("HUD_M_lv_value"), FText::AsNumber(Meta.mLevel) },
		{ TEXT("HUD_M_hp_value"), FText::Format(NSLOCTEXT("CombatTileMapHUDWidget", "SkinHP", "{0}/{1}"),
			FText::AsNumber(FMath::RoundToInt(PlayerHP)), FText::AsNumber(FMath::RoundToInt(PlayerMaxHP))) },
		{ TEXT("HUD_M_gold_value"), FText::AsNumber(Meta.mGold) },
		{ TEXT("HUD_M_btn_end_turn_label"), NSLOCTEXT("CombatTileMapHUDWidget", "EndTurnLabel", "END\nTURN") },
	};

	for (const FSkinValueBinding& Binding : Bindings)
	{
		if (UTextBlock* ValueText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(Binding.WidgetName))))
		{
			ValueText->SetText(Binding.Text);
			ValueText->SetJustification(ETextJustify::Center);
		}
	}
}

void UCombatTileMapHUDWidget::RefreshMoveButton() const
{
	// MOVE 라벨은 WBP TextBlock(HUD_M_btn_move_label)이 소유한다. C++는 이동력 수치만 채운다.
	UTextBlock* MoveLabel = WidgetTree != nullptr ? Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HUD_M_btn_move_label"))) : nullptr;
	if (MoveLabel == nullptr || mCombatUIModel == nullptr)
	{
		return;
	}

	int32 Move = 0;
	int32 MaxMove = 0;
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer)
		{
			Move = FMath::RoundToInt(Unit.mMovementPoint);
			MaxMove = FMath::RoundToInt(Unit.mMaxMovementPoint);
			break;
		}
	}

	const bool moveBuildActive = mMoveBuildPhase == ECombatBuildPhaseUI::AimSelection
		|| mMoveBuildPhase == ECombatBuildPhaseUI::Preview;
	const FText MoveModeText = moveBuildActive
		? NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandCancel", "CANCEL")
		: NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandMove", "MOVE");

	MoveLabel->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandCount", "{0}\n{1}/{2}"),
		MoveModeText,
		FText::AsNumber(Move),
		FText::AsNumber(MaxMove)));
	MoveLabel->SetJustification(ETextJustify::Center);
}

void UCombatTileMapHUDWidget::HandleMoveButtonClicked()
{
	// 이동 모드 진입 의도. 게임플레이가 이동 가능 타일을 표시하고, 타일 탭으로 이동시킨다.
	if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
	{
		CombatGameMode->SelectMove();
	}
}

void UCombatTileMapHUDWidget::HandleCombatActionResolved()
{
	// 스킬/주사위 선택 강조를 푼다(액션 확정·취소 후).
	mSelectedSkillIndex = INDEX_NONE;
	mSelectedDiceIndex = INDEX_NONE;
	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}

namespace
{
	// 작은 칩 텍스트의 폰트 크기를 줄여 좁은 칩 안에 들어가게 한다.
	void SetChipFontSize(UTextBlock* Text, int32 Size)
	{
		if (Text == nullptr)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
	}
}

void UCombatTileMapHUDWidget::RebuildTurnOrderBar()
{
	for (UBorder* Chip : mTurnOrderChips)
	{
		if (Chip != nullptr) { Chip->RemoveFromParent(); }
	}
	for (UTextBlock* Text : mTurnOrderChipTexts)
	{
		if (Text != nullptr) { Text->RemoveFromParent(); }
	}
	mTurnOrderChips.Reset();
	mTurnOrderChipTexts.Reset();

	UCanvasPanel* Canvas = RootCanvas.Get();
	if (Canvas == nullptr || WidgetTree == nullptr || mCombatUIModel == nullptr)
	{
		return;
	}

	const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
	const FTurnUI& Turn = mCombatUIModel->GetTurnUI();

	int32 PlayerId = INDEX_NONE;
	for (const FUnitUI& Unit : Units)
	{
		if (Unit.mIsPlayer) { PlayerId = Unit.mUnitId; break; }
	}

	// 순서는 무조건 플레이어(나)부터, 그 뒤 턴순서(없으면 유닛순서)에서 플레이어 제외.
	TArray<int32> Order;
	if (PlayerId != INDEX_NONE)
	{
		Order.Add(PlayerId);
	}
	if (Turn.mTurnOrderUnitIds.Num() > 0)
	{
		for (int32 Id : Turn.mTurnOrderUnitIds)
		{
			if (Id != PlayerId && !Order.Contains(Id)) { Order.Add(Id); }
		}
	}
	else
	{
		for (const FUnitUI& Unit : Units)
		{
			if (Unit.mUnitId != PlayerId && !Order.Contains(Unit.mUnitId)) { Order.Add(Unit.mUnitId); }
		}
	}
	if (Order.Num() == 0)
	{
		return;
	}

	// 전투 HUD 가운데 하단, 칩 줄을 가로 중앙 정렬.
	const float ChipWidth = 0.048f;
	const float ChipHeight = 0.040f;
	const float Gap = 0.007f;
	const float Top = 0.108f;
	const float Total = StaticCast<float>(Order.Num()) * ChipWidth + StaticCast<float>(Order.Num() - 1) * Gap;
	const float Start = 0.5f - Total * 0.5f;

	int32 EnemyOrdinal = 0;
	for (int32 SlotIndex = 0; SlotIndex < Order.Num(); ++SlotIndex)
	{
		const int32 Id = Order[SlotIndex];
		const bool bPlayer = (Id == PlayerId);
		const bool bCurrent = (Id == Turn.mCurrentUnitId);

		UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (Chip == nullptr || Text == nullptr)
		{
			continue;
		}

		FLinearColor BrushColor = bPlayer
			? FLinearColor(0.10f, 0.34f, 0.22f, 0.92f)
			: FLinearColor(0.34f, 0.12f, 0.10f, 0.90f);
		if (bCurrent)
		{
			// 현재 턴 강조(밝게).
			BrushColor = FMath::Lerp(BrushColor, FLinearColor(1.0f, 0.92f, 0.50f, 1.0f), 0.45f);
		}
		Chip->SetBrushColor(BrushColor);
		Chip->SetPadding(FMargin(2.0f, 1.0f));

		// "나" / "적N" — 소스 인코딩에 흔들리지 않게 UTF-8 바이트를 명시 변환.
		const FText Label = bPlayer
			? FText::FromString(UTF8_TO_TCHAR("\xEB\x82\x98"))
			: FText::Format(FText::FromString(UTF8_TO_TCHAR("\xEC\xA0\x81{0}")), FText::AsNumber(++EnemyOrdinal));
		Text->SetJustification(ETextJustify::Center);
		Text->SetText(Label);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 1.0f, 0.94f, 1.0f)));
		SetChipFontSize(Text, 13);

		Chip->AddChild(Text);
		Canvas->AddChildToCanvas(Chip);

		const float Left = Start + StaticCast<float>(SlotIndex) * (ChipWidth + Gap);
		RDUILayout::ApplyAnchoredSlot(Chip, FAnchors(Left, Top, Left + ChipWidth, Top + ChipHeight), 32);

		mTurnOrderChips.Add(Chip);
		mTurnOrderChipTexts.Add(Text);
	}
}
