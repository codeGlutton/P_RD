#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Viewport.h"

void UCombatTileMapHUDWidget::EnsureRuntimeWidgets()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	UCanvasPanel* TargetRootCanvas = RootCanvas.Get();
	if (TargetRootCanvas == nullptr)
	{
		TargetRootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		RootCanvas = TargetRootCanvas;
	}

	if (TargetRootCanvas == nullptr)
	{
		return;
	}

	HideLegacyDiceSlots();
	HideLegacySkillDetailCard();
	HideLegacySkillRail();

	if (DiceRollViewport != nullptr)
	{
		DiceRollViewport->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (DiceRollStatusText == nullptr)
	{
		DiceRollStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceRollStatusText"));
		if (DiceRollStatusText != nullptr)
		{
			DiceRollStatusText->SetJustification(ETextJustify::Center);
			DiceRollStatusText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceRolling", "ROLLING DICE"));
			DiceRollStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 1.0f, 0.96f, 1.0f)));
			TargetRootCanvas->AddChildToCanvas(DiceRollStatusText);
		}
	}

	if (mDiceRollInputButton == nullptr)
	{
		mDiceRollInputButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DiceRollInputButton"));
		if (mDiceRollInputButton != nullptr)
		{
			mDiceRollInputButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.01f));
			mDiceRollInputButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
			TargetRootCanvas->AddChildToCanvas(mDiceRollInputButton);
		}
	}

	if (mDiceAssignmentText == nullptr)
	{
		mDiceAssignmentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceAssignmentText"));
		if (mDiceAssignmentText != nullptr)
		{
			mDiceAssignmentText->SetJustification(ETextJustify::Left);
			mDiceAssignmentText->SetText(FText::GetEmpty());   // 유휴 안내문구 제거
			mDiceAssignmentText->SetVisibility(ESlateVisibility::Collapsed);
			mDiceAssignmentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 1.0f, 0.96f, 0.96f)));
			TargetRootCanvas->AddChildToCanvas(mDiceAssignmentText);
		}
	}

	if (mSkillDetailDismissButton == nullptr)
	{
		mSkillDetailDismissButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillDetailDismissButton"));
		if (mSkillDetailDismissButton != nullptr)
		{
			mSkillDetailDismissButton->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));
			mSkillDetailDismissButton->SetVisibility(ESlateVisibility::Collapsed);
			mSkillDetailDismissButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked);
			TargetRootCanvas->AddChildToCanvas(mSkillDetailDismissButton);
		}
	}

	if (mSkillDetailPanel == nullptr)
	{
		mSkillDetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SkillDetailPanel"));
		mSkillDetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailText"));
		if (mSkillDetailPanel != nullptr && mSkillDetailText != nullptr)
		{
			mSkillDetailPanel->SetBrushColor(FLinearColor(0.025f, 0.042f, 0.048f, 0.96f));
			mSkillDetailPanel->SetPadding(FMargin(32.0f, 28.0f));
			mSkillDetailPanel->AddChild(mSkillDetailText);
			mSkillDetailText->SetJustification(ETextJustify::Left);
			mSkillDetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 1.0f, 0.96f, 1.0f)));
			mSkillDetailText->SetAutoWrapText(true);
			mSkillDetailText->SetLineHeightPercentage(1.12f);
			mSkillDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mSkillDetailPanel);
		}
	}

	if (mSkillDetailBackdropPanels.Num() == 0)
	{
		for (int32 PanelIndex = 0; PanelIndex < 1; ++PanelIndex)
		{
			UBorder* BackdropPanel = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("SkillDetailBackdropPanel_%d"), PanelIndex))
			);
			if (BackdropPanel == nullptr)
			{
				continue;
			}

			BackdropPanel->SetBrushColor(FLinearColor(0.015f, 0.020f, 0.025f, 0.78f));
			BackdropPanel->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(BackdropPanel);
			mSkillDetailBackdropPanels.Add(BackdropPanel);
		}
	}

	if (mCombatFeedText == nullptr)
	{
		mCombatFeedText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CombatFeedText"));
		if (mCombatFeedText != nullptr)
		{
			mCombatFeedText->SetJustification(ETextJustify::Center);
			mCombatFeedText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.42f, 1.0f)));
			mCombatFeedText->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mCombatFeedText);
		}
	}

	if (mCombatStatusBarText == nullptr)
	{
		mCombatStatusBarText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CombatStatusBarText"));
		if (mCombatStatusBarText != nullptr)
		{
			mCombatStatusBarText->SetJustification(ETextJustify::Left);
			mCombatStatusBarText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 1.0f, 0.92f, 1.0f)));
			// Lv/HP/Gold는 이제 탑바(TopMenuBar)에서 보여주므로 전투 HUD의 중복 상태줄은 숨긴다.
			mCombatStatusBarText->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mCombatStatusBarText);
		}
	}

	if (mMoveButton == nullptr)
	{
		mMoveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MoveCommandButton"));
		if (mMoveButton != nullptr)
		{
			mMoveButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MoveCommandButtonText"));
			if (mMoveButtonText != nullptr)
			{
				mMoveButtonText->SetJustification(ETextJustify::Center);
				mMoveButtonText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandIdle", "MOVE\n0/0"));
				mMoveButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 1.0f, 0.92f, 1.0f)));
				mMoveButton->AddChild(mMoveButtonText);
			}
			mMoveButton->SetBackgroundColor(FLinearColor(0.10f, 0.30f, 0.32f, 0.95f));
			mMoveButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleMoveButtonClicked);
			TargetRootCanvas->AddChildToCanvas(mMoveButton);
		}
	}

	if (EndTurnButton == nullptr)
	{
		EndTurnButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EndTurnButton"));
		if (EndTurnButton != nullptr)
		{
			UTextBlock* EndTurnButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndTurnButtonText"));
			if (EndTurnButtonText != nullptr)
			{
				EndTurnButtonText->SetJustification(ETextJustify::Center);
				EndTurnButtonText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "EndTurnButtonText", "END\nTURN"));
				EndTurnButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.96f, 0.86f, 1.0f)));
				EndTurnButton->AddChild(EndTurnButtonText);
			}

			EndTurnButton->SetBackgroundColor(FLinearColor(0.32f, 0.08f, 0.07f, 0.95f));
			TargetRootCanvas->AddChildToCanvas(EndTurnButton);
		}
	}

	RebuildSkillRailWidgets();
	EnsureSkillInputButtons();
	RebuildEquipmentBar();    // 탑바 좌측 하단 장비 칩(뷰모델 미연결이면 비워 둠)
	RebuildTurnOrderBar();    // 탑바 가운데 하단 턴 순서 칩
	ApplyRuntimeWidgetLayout();
}
