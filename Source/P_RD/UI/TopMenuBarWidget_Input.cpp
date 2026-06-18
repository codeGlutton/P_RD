#include "UI/TopMenuBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "UI/TopMenuBarLayoutPolicy.h"

namespace
{
	/** @brief 현재 표시 중인 위젯인지 확인한다. */
	// 탑바는 루트와 일반 장식 위젯을 SelfHitTestInvisible로 바꾸되, 이미 숨겨진 위젯은 숨김 상태를 유지해야 한다.
	bool ShouldKeepWidgetVisibility(ESlateVisibility Visibility)
	{
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}
}

/** @brief 탑바의 장식 영역은 입력을 통과시키고 버튼만 입력을 받게 한다. */
// 인게임 탑바가 전체 화면 위젯으로 올라와도 맵 클릭, 전투 입력, 하위 팝업 입력을 막지 않게 하기 위한 설정이다.
// 버튼은 Visible로 유지해 클릭 가능하게 두고, 나머지 표시 위젯은 SelfHitTestInvisible로 바꾼다.
// 왜 입력을 통과시키는가:
// 탑바 WBP가 화면 전체 크기로 배치되면 보이지 않는 배경이 아래 UI의 터치를 먹을 수 있다.
// 장식 영역은 통과시키고 버튼만 입력을 받게 해야 게임 조작과 팝업 조작이 막히지 않는다.
void UTopMenuBarWidget::ApplyInputPassThrough()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (WidgetTree != nullptr)
	{
		UWidget* RootWidget = WidgetTree->RootWidget;
		WidgetTree->ForEachWidget([RootWidget](UWidget* Widget)
		{
			if (Widget == nullptr || !ShouldKeepWidgetVisibility(Widget->GetVisibility()))
			{
				return;
			}

			if (Widget == RootWidget)
			{
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				return;
			}

			if (Cast<UButton>(Widget) != nullptr)
			{
				Widget->SetVisibility(ESlateVisibility::Visible);
				return;
			}

			Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		});
	}

	ConfigureDesignerButton(MapButton);
	ConfigureDesignerButton(SettingsButton);
	ConfigureDesignerButton(DiceButton);
	ConfigureDesignerButton(SkillButton);
	ConfigureDesignerButton(mRuntimeDiceHitButton);
	ConfigureDesignerButton(mRuntimeSkillHitButton);
}

/** @brief WBP 버튼 입력을 탑바의 토글 핸들러에 연결한다. */
void UTopMenuBarWidget::BindButtonEvents()
{
	if (MapButton != nullptr)
	{
		MapButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleMapButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSettingsButtonClicked);
	}

	if (DiceButton != nullptr)
	{
		DiceButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleDiceButtonClicked);
	}

	if (SkillButton != nullptr)
	{
		SkillButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSkillButtonClicked);
	}
}

/** @brief NativeConstruct()에서 연결한 버튼 입력을 해제한다. */
void UTopMenuBarWidget::UnbindButtonEvents()
{
	if (MapButton != nullptr)
	{
		MapButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleMapButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleSettingsButtonClicked);
	}

	if (DiceButton != nullptr)
	{
		DiceButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleDiceButtonClicked);
	}

	if (SkillButton != nullptr)
	{
		SkillButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleSkillButtonClicked);
	}
}

/** @brief 모바일 입력에서 버튼이 눌림 즉시 반응하도록 버튼 입력 방식을 보정한다. */
void UTopMenuBarWidget::ConfigureDesignerButton(UButton* Button) const
{
	if (Button == nullptr)
	{
		return;
	}

	Button->SetVisibility(ESlateVisibility::Visible);
	Button->SetIsEnabled(true);
	Button->SetClickMethod(EButtonClickMethod::MouseDown);
	Button->SetTouchMethod(EButtonTouchMethod::Down);
	Button->SetPressMethod(EButtonPressMethod::ButtonPress);
}

void UTopMenuBarWidget::EnsureRuntimeTopBarHitAreas()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (RootCanvas == nullptr)
	{
		return;
	}

	if (mRuntimeDiceHitButton == nullptr)
	{
		mRuntimeDiceHitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RuntimeDiceHitButton"));
		if (mRuntimeDiceHitButton != nullptr)
		{
			mRuntimeDiceHitButton->SetBackgroundColor(RDTopMenuBarLayout::GetRuntimeHitAreaColor());
			mRuntimeDiceHitButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleDiceButtonClicked);
			RootCanvas->AddChildToCanvas(mRuntimeDiceHitButton);
		}
	}

	if (mRuntimeSkillHitButton == nullptr)
	{
		mRuntimeSkillHitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RuntimeSkillHitButton"));
		if (mRuntimeSkillHitButton != nullptr)
		{
			mRuntimeSkillHitButton->SetBackgroundColor(RDTopMenuBarLayout::GetRuntimeHitAreaColor());
			mRuntimeSkillHitButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSkillButtonClicked);
			RootCanvas->AddChildToCanvas(mRuntimeSkillHitButton);
		}
	}

	/*
	 * 시각 버튼은 최상단에 그대로 두고, 실제 입력은 아래쪽까지 받는다.
	 * 이렇게 해야 상태바/상단 제스처 영역을 피해서 눌러도 같은 DICE/SKILL 패널을 열 수 있다.
	 */
	ConfigureRuntimeHitArea(mRuntimeDiceHitButton, RDTopMenuBarLayout::GetDiceHitAreaAnchors());
	ConfigureRuntimeHitArea(mRuntimeSkillHitButton, RDTopMenuBarLayout::GetSkillHitAreaAnchors());
}

void UTopMenuBarWidget::ConfigureRuntimeHitArea(UButton* Button, const FAnchors& Anchors) const
{
	if (Button == nullptr)
	{
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Button->Slot);
	if (CanvasSlot == nullptr)
	{
		return;
	}

	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetOffsets(FMargin(0.0f));
	CanvasSlot->SetAlignment(FVector2D::ZeroVector);
	CanvasSlot->SetZOrder(RDTopMenuBarLayout::GetRuntimeHitAreaZOrder());
	ConfigureDesignerButton(Button);
}
