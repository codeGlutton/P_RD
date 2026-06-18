// @file TopMenuBarWidget_Debug.cpp
// @brief [디버그] 전투 진행 로직이 없어도 승/패 UI 흐름을 테스트하기 위한 탑바 디버그 버튼

#include "UI/TopMenuBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

namespace
{
	/** @brief 디버그 버튼 하나를 만들어 우상단 캔버스에 라벨과 함께 배치한다. */
	UButton* MakeDebugResultButton(UWidgetTree* Tree, UCanvasPanel* Canvas, FName Name, const FText& Label, float OffsetY, const FLinearColor& Color)
	{
		if (Tree == nullptr || Canvas == nullptr)
		{
			return nullptr;
		}

		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		if (Button == nullptr)
		{
			return nullptr;
		}
		Button->SetBackgroundColor(Color);

		if (UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Text->SetText(Label);
			Text->SetJustification(ETextJustify::Center);
			Button->SetContent(Text);
		}

		Canvas->AddChildToCanvas(Button);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Button->Slot))
		{
			// 우상단 기준으로 세로로 쌓는다(탑바 본체와 겹치지 않게 아래로 내림).
			CanvasSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
			CanvasSlot->SetAlignment(FVector2D(1.f, 0.f));
			CanvasSlot->SetSize(FVector2D(150.f, 48.f));
			CanvasSlot->SetPosition(FVector2D(-16.f, OffsetY));
		}
		return Button;
	}
}

void UTopMenuBarWidget::SetupDebugResultButtons()
{
	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree != nullptr ? WidgetTree->RootWidget : nullptr);
	if (RootCanvas == nullptr)
	{
		return;
	}

	if (mDebugVictoryButton == nullptr)
	{
		mDebugVictoryButton = MakeDebugResultButton(WidgetTree, RootCanvas, TEXT("DebugVictoryButton"),
			NSLOCTEXT("TopMenuBarDebug", "Victory", "승리(DEBUG)"), 96.f, FLinearColor(0.10f, 0.45f, 0.12f, 0.85f));
		if (mDebugVictoryButton != nullptr)
		{
			mDebugVictoryButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleDebugVictoryClicked);
		}
	}

	if (mDebugDefeatButton == nullptr)
	{
		mDebugDefeatButton = MakeDebugResultButton(WidgetTree, RootCanvas, TEXT("DebugDefeatButton"),
			NSLOCTEXT("TopMenuBarDebug", "Defeat", "패배(DEBUG)"), 152.f, FLinearColor(0.50f, 0.12f, 0.12f, 0.85f));
		if (mDebugDefeatButton != nullptr)
		{
			mDebugDefeatButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleDebugDefeatClicked);
		}
	}
}

void UTopMenuBarWidget::HandleDebugVictoryClicked()
{
	if (USRPGCombatSubsystem* CombatSubsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<USRPGCombatSubsystem>() : nullptr)
	{
		CombatSubsystem->DebugForceEndCombat(ESRPGCombatResult::PlayerWin);
	}
}

void UTopMenuBarWidget::HandleDebugDefeatClicked()
{
	if (USRPGCombatSubsystem* CombatSubsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<USRPGCombatSubsystem>() : nullptr)
	{
		CombatSubsystem->DebugForceEndCombat(ESRPGCombatResult::PlayerLose);
	}
}
