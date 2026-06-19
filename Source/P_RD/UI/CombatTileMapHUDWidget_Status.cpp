#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/TopMenuBarWidget.h"
#include "UI/UIRuntimeLayout.h"

void UCombatTileMapHUDWidget::HandleCombatUIChanged(ECombatUIDomain Domain)
{
	// 상단 상태바에 영향을 주는 도메인(메타/유닛/턴/전체)만 다시 그린다.
	if (Domain == ECombatUIDomain::Meta
		|| Domain == ECombatUIDomain::Unit
		|| Domain == ECombatUIDomain::Turn
		|| Domain == ECombatUIDomain::All)
	{
		RefreshCombatStatusBar();
	}

	// 장비 칩(탑바 좌측 하단): 장비 도메인.
	if (Domain == ECombatUIDomain::Equipment || Domain == ECombatUIDomain::All)
	{
		RebuildEquipmentBar();
	}

	// 턴 순서 칩(탑바 가운데 하단): 턴/유닛이 바뀌면 순서·현재턴 강조가 달라진다.
	if (Domain == ECombatUIDomain::Turn
		|| Domain == ECombatUIDomain::Unit
		|| Domain == ECombatUIDomain::All)
	{
		RebuildTurnOrderBar();
	}

	// 유닛 수가 바뀌면 머리 위 HP바도 다시 만든다.
	if (Domain == ECombatUIDomain::Unit || Domain == ECombatUIDomain::All)
	{
		RebuildUnitHpBars();
	}

	// 주사위(굴림/사용) 갱신 시 보유 주사위 표시를 다시 읽어 그린다(쓴 주사위 비활성 반영).
	if (Domain == ECombatUIDomain::Dice || Domain == ECombatUIDomain::All)
	{
		RefreshDiceViewsFromRunData();
		RefreshOwnedDiceCards();
	}
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

	// 탑바 DICE/SKILL 라벨에 푸시할 보유 주사위/스킬 수(전부 뷰모델 경유).
	const int32 DiceCount = mCombatUIModel->GetDiceUIs().Num();
	const int32 SkillCount = mCombatUIModel->GetSkillUIs().Num();

	// 로컬 상태줄(접힘 상태)에도 값은 채워두지만, 실제 표시는 탑바로 푸시한다.
	mCombatStatusBarText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "CombatStatusBarFormat", "Lv {0}    HP {1}/{2}    GOLD {3}"),
		FText::AsNumber(Level), FText::AsNumber(HP), FText::AsNumber(MaxHP), FText::AsNumber(Gold)));

	// 게임플레이 ASC 속성 동기화가 깨져 RunPersistData를 못 믿으므로, 전투 중에는 뷰모델 값을
	// 탑바 요약에 직접 푸시한다(탑바=Lv/HP/Gold 정상 표시). 속성 초기화가 고쳐지면 이 경로는 정리 가능.
	if (UWorld* World = GetWorld())
	{
		if (UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>())
		{
			if (UTopMenuBarWidget* TopBar = WorldWidgetSubsystem->GetWorldWidget<UTopMenuBarWidget>(EWorldWidgetType::TopMenuBar))
			{
				TopBar->SetCombatPlayerSummary(Level, HP, MaxHP, Gold);
				TopBar->SetCombatDiceSkillCount(DiceCount, SkillCount);
			}
		}
	}

	RefreshMoveButton();
}

void UCombatTileMapHUDWidget::RefreshMoveButton() const
{
	if (mMoveButtonText == nullptr || mCombatUIModel == nullptr)
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

	mMoveButtonText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandCount", "MOVE\n{0}/{1}"),
		FText::AsNumber(Move),
		FText::AsNumber(MaxMove)));
}

void UCombatTileMapHUDWidget::HandleMoveButtonClicked()
{
	// 이동 모드 진입 의도. 게임플레이가 이동 가능 타일을 표시하고, 타일 탭으로 이동시킨다.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestMove();
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

void UCombatTileMapHUDWidget::RebuildEquipmentBar()
{
	// 기존 칩 제거(개수가 바뀔 수 있어 매번 다시 만든다).
	for (UBorder* Chip : mEquipmentChips)
	{
		if (Chip != nullptr) { Chip->RemoveFromParent(); }
	}
	for (UTextBlock* Text : mEquipmentChipTexts)
	{
		if (Text != nullptr) { Text->RemoveFromParent(); }
	}
	mEquipmentChips.Reset();
	mEquipmentChipTexts.Reset();

	UCanvasPanel* Canvas = RootCanvas.Get();
	if (Canvas == nullptr || WidgetTree == nullptr || mCombatUIModel == nullptr)
	{
		return;
	}

	const TArray<FEquipmentUI>& Equips = mCombatUIModel->GetEquipmentUIs();

	// 탑바(~0.11) 바로 아래, 스킬레일(0.144) 위의 얇은 좌측 줄.
	const float ChipWidth = 0.050f;
	const float ChipHeight = 0.030f;
	const float Gap = 0.006f;
	const float Left0 = 0.022f;
	const float Top = 0.112f;
	for (int32 Index = 0; Index < Equips.Num(); ++Index)
	{
		UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (Chip == nullptr || Text == nullptr)
		{
			continue;
		}

		Chip->SetBrushColor(Equips[Index].mIsEquipped
			? FLinearColor(0.16f, 0.40f, 0.40f, 0.92f)
			: FLinearColor(0.12f, 0.17f, 0.19f, 0.80f));
		Chip->SetPadding(FMargin(2.0f, 1.0f));

		Text->SetJustification(ETextJustify::Center);
		Text->SetText(Equips[Index].mName.IsEmpty()
			? FText::Format(NSLOCTEXT("CombatTileMapHUDWidget", "EquipSlotFallback", "EQ{0}"), FText::AsNumber(Index + 1))
			: Equips[Index].mName);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 1.0f, 0.96f, 1.0f)));
		SetChipFontSize(Text, 11);

		Chip->AddChild(Text);
		Canvas->AddChildToCanvas(Chip);

		const float Left = Left0 + StaticCast<float>(Index) * (ChipWidth + Gap);
		RDUILayout::ApplyAnchoredSlot(Chip, FAnchors(Left, Top, Left + ChipWidth, Top + ChipHeight), 31);

		mEquipmentChips.Add(Chip);
		mEquipmentChipTexts.Add(Text);
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

	// 탑바 가운데 하단, 칩 줄을 가로 중앙 정렬.
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
