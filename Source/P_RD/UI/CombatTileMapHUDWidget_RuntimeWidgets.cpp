#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "UI/IndexedButtonWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"

namespace
{
	template <typename T>
	T* FindNamedWidget(const UWidgetTree* InWidgetTree, const TCHAR* WidgetName)
	{
		return InWidgetTree != nullptr ? Cast<T>(InWidgetTree->FindWidget(FName(WidgetName))) : nullptr;
	}

	void ConfigureInvisibleButton(UButton* Button)
	{
		if (Button == nullptr)
		{
			return;
		}

		FSlateBrush EmptyBrush;
		EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;

		FButtonStyle InvisibleStyle;
		InvisibleStyle.SetNormal(EmptyBrush);
		InvisibleStyle.SetHovered(EmptyBrush);
		InvisibleStyle.SetPressed(EmptyBrush);
		InvisibleStyle.SetDisabled(EmptyBrush);

		Button->SetStyle(InvisibleStyle);
		Button->SetBackgroundColor(FLinearColor::Transparent);
	}
}

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

	// WBP에 디자이너 스킨(concept 아트 + HUD_* 앵커)이 있는지 먼저 판별한다.
	// 활성이면 아래 플레이스홀더 배경을 투명화해 WBP 아트가 보이게 하고, 좌표는 WBP 앵커에서 읽는다.
	ResolveDesignerSkin();
	if (IsDesignerSkinActive())
	{
		UCanvasPanel* DesignerCanvasPanel = DesignCanvas.Get();
		if (DesignerCanvasPanel == nullptr)
		{
			DesignerCanvasPanel = FindNamedWidget<UCanvasPanel>(WidgetTree, TEXT("DesignCanvas"));
			DesignCanvas = DesignerCanvasPanel;
		}
		if (DesignerCanvasPanel != nullptr)
		{
			// 스킨 런타임 위젯(주사위/스킬/MOVE/값텍스트 등)은 DesignCanvas(1920x1080 디자인 좌표계)에 붙인다.
			// 단 RootCanvas 멤버는 '풀뷰포트 루트'로 유지한다 — 월드투영 HP바(UnitBars.cpp)가 RootCanvas에
			// ProjectWorldLocationToWidgetPosition의 뷰포트 픽셀을 그대로 넣기 때문. RootCanvas를 DesignCanvas로
			// 덮으면 뷰포트 픽셀이 디자인 픽셀로 오해석돼 HP바가 유닛 머리에서 떨어진다.
			TargetRootCanvas = DesignerCanvasPanel;
		}

		// 기존 콘셉트 레일은 아이콘만 담는 좁은 프레임이라 카드형 이름/역할 UI와 겹친다.
		// 슬롯 위치는 더 이상 상속하지 않고 새 런타임 도크가 렌더/입력을 함께 소유한다.
		WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			if (Widget != nullptr && Widget->GetName().StartsWith(TEXT("R_skill_rail")))
			{
				Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
		});
	}

	// (값 텍스트 LV/HP/Gold는 더 이상 C++가 생성/정렬하지 않는다 — 빌드가 WBP의 HUD_M_lv/hp/gold_value를
	//  실제 TextBlock으로 심고 위치/폰트/정렬을 소유한다. C++는 RefreshSkinValueLabels에서 내용만 채운다.)


	// --- 주사위 판(물리 굴림) 보드/배경/캡처 위젯 (20260622 이식). 위치는 ApplyRuntimeWidgetLayout에서 잡는다. ---
	if (mDiceRollBackdropPanel == nullptr)
	{
		mDiceRollBackdropPanel = FindNamedWidget<UBorder>(WidgetTree, TEXT("DiceRollBackdropPanel"));
	}
	if (mDiceRollBackdropPanel == nullptr)
	{
		mDiceRollBackdropPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DiceRollBackdropPanel"));
		if (mDiceRollBackdropPanel != nullptr)
		{
			RootCanvas->AddChildToCanvas(mDiceRollBackdropPanel);   // 풀뷰포트 회색 배경(전체 덮기)
		}
	}
	if (mDiceRollBackdropPanel != nullptr)
	{
		mDiceRollBackdropPanel->SetBrushColor(FLinearColor(0.010f, 0.014f, 0.020f, 0.80f));
		mDiceRollBackdropPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mDiceRollBoardImage == nullptr)
	{
		mDiceRollBoardImage = FindNamedWidget<UImage>(WidgetTree, TEXT("DiceRollBoardImage"));
	}
	if (mDiceRollBoardImage == nullptr)
	{
		mDiceRollBoardImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DiceRollBoardImage"));
		if (mDiceRollBoardImage != nullptr)
		{
			RootCanvas->AddChildToCanvas(mDiceRollBoardImage);   // 오버레이는 풀뷰포트 RootCanvas 기준
		}
	}
	if (mDiceRollBoardImage != nullptr)
	{
		if (mDiceRollBoardTexture != nullptr)
		{
			mDiceRollBoardImage->SetBrushFromTexture(mDiceRollBoardTexture, true);
		}
		mDiceRollBoardImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.98f));
		mDiceRollBoardImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mDiceRollPhysicsImage == nullptr)
	{
		mDiceRollPhysicsImage = FindNamedWidget<UImage>(WidgetTree, TEXT("DiceRollPhysicsImage"));
	}
	if (mDiceRollPhysicsImage == nullptr)
	{
		mDiceRollPhysicsImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DiceRollPhysicsImage"));
		if (mDiceRollPhysicsImage != nullptr)
		{
			RootCanvas->AddChildToCanvas(mDiceRollPhysicsImage);   // 오버레이는 풀뷰포트 RootCanvas 기준
		}
	}
	if (mDiceRollPhysicsImage != nullptr)
	{
		mDiceRollPhysicsImage->SetColorAndOpacity(FLinearColor::White);
		mDiceRollPhysicsImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (DiceRollStatusText == nullptr)
	{
		DiceRollStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceRollStatusText"));
		if (DiceRollStatusText != nullptr)
		{
			DiceRollStatusText->SetJustification(ETextJustify::Center);
			DiceRollStatusText->SetText(FText::GetEmpty());
			DiceRollStatusText->SetVisibility(ESlateVisibility::Collapsed);
			DiceRollStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 1.0f, 0.96f, 1.0f)));
			RootCanvas->AddChildToCanvas(DiceRollStatusText);   // 오버레이는 풀뷰포트 RootCanvas 기준
		}
	}

	if (mDiceRollInputButton == nullptr)
	{
		mDiceRollInputButton = FindNamedWidget<UButton>(WidgetTree, TEXT("DiceRollInputButton"));   // WBP-네이티브 버튼 우선 바인딩
	}
	if (mDiceRollInputButton == nullptr)
	{
		mDiceRollInputButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DiceRollInputButton"));
		if (mDiceRollInputButton != nullptr)
		{
			RootCanvas->AddChildToCanvas(mDiceRollInputButton);   // 풀뷰포트 입력 차단막(WBP에 없을 때만)
		}
	}
	if (mDiceRollInputButton != nullptr)
	{
		mDiceRollInputButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.01f));
		mDiceRollInputButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleDiceRollInputButtonClicked);
	}

	if (mTurnChangeBackdropPanel == nullptr)
	{
		mTurnChangeBackdropPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TurnChangeBackdropPanel"));
		if (mTurnChangeBackdropPanel != nullptr)
		{
			RootCanvas->AddChildToCanvas(mTurnChangeBackdropPanel);
		}
	}
	if (mTurnChangeBackdropPanel != nullptr)
	{
		mTurnChangeBackdropPanel->SetBrushColor(FLinearColor(0.004f, 0.006f, 0.010f, 0.86f));
		mTurnChangeBackdropPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mTurnChangeInputBlocker == nullptr)
	{
		mTurnChangeInputBlocker = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TurnChangeInputBlocker"));
		if (mTurnChangeInputBlocker != nullptr)
		{
			RootCanvas->AddChildToCanvas(mTurnChangeInputBlocker);
		}
	}
	if (mTurnChangeInputBlocker != nullptr)
	{
		ConfigureInvisibleButton(mTurnChangeInputBlocker);
		mTurnChangeInputBlocker->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mTurnChangeVideoImage == nullptr)
	{
		mTurnChangeVideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TurnChangeVideoImage"));
		if (mTurnChangeVideoImage != nullptr)
		{
			RootCanvas->AddChildToCanvas(mTurnChangeVideoImage);
		}
	}
	if (mTurnChangeVideoImage != nullptr)
	{
		mTurnChangeVideoImage->SetColorAndOpacity(FLinearColor::White);
		mTurnChangeVideoImage->SetRenderOpacity(1.0f);
		mTurnChangeVideoImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mTurnChangeTurnTextPanel == nullptr)
	{
		mTurnChangeTurnTextPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TurnChangeTurnTextPanel"));
		mTurnChangeTurnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnChangeTurnText"));
		if (mTurnChangeTurnTextPanel != nullptr && mTurnChangeTurnText != nullptr)
		{
			mTurnChangeTurnTextPanel->SetBrushColor(FLinearColor::Transparent);
			mTurnChangeTurnTextPanel->SetHorizontalAlignment(HAlign_Center);
			mTurnChangeTurnTextPanel->SetVerticalAlignment(VAlign_Center);
			mTurnChangeTurnTextPanel->SetVisibility(ESlateVisibility::Collapsed);
			mTurnChangeTurnTextPanel->AddChild(mTurnChangeTurnText);

			mTurnChangeTurnText->SetJustification(ETextJustify::Center);
			mTurnChangeTurnText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.90f, 0.48f, 1.0f)));
			mTurnChangeTurnText->SetShadowOffset(FVector2D(3.0f, 4.0f));
			mTurnChangeTurnText->SetShadowColorAndOpacity(FLinearColor(0.02f, 0.01f, 0.0f, 0.85f));
			FSlateFontInfo TurnFont = mTurnChangeTurnText->GetFont();
			TurnFont.Size = 104;
			mTurnChangeTurnText->SetFont(TurnFont);
			mTurnChangeTurnText->SetText(FText::GetEmpty());

			RootCanvas->AddChildToCanvas(mTurnChangeTurnTextPanel);
		}
	}

	if (mSkillDockPanel == nullptr)
	{
		mSkillDockPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SkillDockPanel"));
		mSkillDockTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDockTitleText"));
		if (mSkillDockPanel != nullptr && mSkillDockTitleText != nullptr)
		{
			mSkillDockPanel->SetBrushColor(FLinearColor(0.018f, 0.032f, 0.045f, 0.78f));
			mSkillDockPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
			mSkillDockTitleText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "SkillDockTitle", "스킬  ·  짧게 선택 / 길게 설명"));
			mSkillDockTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.80f, 0.96f, 1.0f, 1.0f)));
			mSkillDockTitleText->SetJustification(ETextJustify::Left);
			mSkillDockTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
			FSlateFontInfo TitleFont = mSkillDockTitleText->GetFont();
			TitleFont.Size = 15;
			TitleFont.OutlineSettings.OutlineSize = 1;
			TitleFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
			mSkillDockTitleText->SetFont(TitleFont);
			TargetRootCanvas->AddChildToCanvas(mSkillDockPanel);
			TargetRootCanvas->AddChildToCanvas(mSkillDockTitleText);
		}
	}

	if (mDiceTrayPanel == nullptr)
	{
		mDiceTrayPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DiceTrayPanel"));
		mDiceTrayTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceTrayTitleText"));
		if (mDiceTrayPanel != nullptr && mDiceTrayTitleText != nullptr)
		{
			mDiceTrayPanel->SetBrushColor(FLinearColor(0.018f, 0.032f, 0.045f, 0.82f));
			mDiceTrayPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
			mDiceTrayTitleText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "DiceTrayTitle", "이번 턴 주사위"));
			mDiceTrayTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.80f, 0.96f, 1.0f, 1.0f)));
			mDiceTrayTitleText->SetJustification(ETextJustify::Left);
			mDiceTrayTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
			FSlateFontInfo TitleFont = mDiceTrayTitleText->GetFont();
			TitleFont.Size = 15;
			TitleFont.OutlineSettings.OutlineSize = 1;
			TitleFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
			mDiceTrayTitleText->SetFont(TitleFont);
			TargetRootCanvas->AddChildToCanvas(mDiceTrayPanel);
			TargetRootCanvas->AddChildToCanvas(mDiceTrayTitleText);
		}
	}

	if (mDiceAssignmentText == nullptr)
	{
		mDiceAssignmentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceAssignmentText"));
		if (mDiceAssignmentText != nullptr)
		{
			mDiceAssignmentText->SetJustification(ETextJustify::Left);
			mDiceAssignmentText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "DiceAssignmentChooseTarget", "① 전장의 적을 누르세요"));
			mDiceAssignmentText->SetVisibility(ESlateVisibility::HitTestInvisible);
			mDiceAssignmentText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 1.0f, 0.96f, 0.96f)));
			mDiceAssignmentText->SetAutoWrapText(true);
			mDiceAssignmentText->SetTextOverflowPolicy(ETextOverflowPolicy::Clip);
			FSlateFontInfo AssignmentFont = mDiceAssignmentText->GetFont();
			AssignmentFont.Size = 13;
			mDiceAssignmentText->SetFont(AssignmentFont);
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
			RootCanvas->AddChildToCanvas(mSkillDetailDismissButton);   // 풀뷰포트 RootCanvas — 상세 위 아무 데나 탭 닫기.
		}
	}

	if (mSkillDetailPanel == nullptr)
	{
		mSkillDetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SkillDetailPanel"));
		mSkillDetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailText"));
		if (mSkillDetailPanel != nullptr && mSkillDetailText != nullptr)
		{
			// concept_09: 패널 배경은 프레임 이미지가 담당 → 설명 Border는 가독성용 옅은 딤만.
			mSkillDetailPanel->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.035f, 0.32f));
			mSkillDetailPanel->SetPadding(FMargin(18.0f, 14.0f));
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

			// 상세 오버레이 뒤 풀뷰포트 딤 — 주사위 굴림 배경(mDiceRollBackdropPanel)과 "완전 동일한" 색/알파.
			// 주사위 딤과 동일하게 RootCanvas(풀뷰포트)에 붙인다. DesignCanvas(스킨)에 붙이면 RootCanvas의 HP바를 못 덮는다.
			BackdropPanel->SetBrushColor(FLinearColor(0.010f, 0.014f, 0.020f, 0.80f));
			BackdropPanel->SetVisibility(ESlateVisibility::Collapsed);
			RootCanvas->AddChildToCanvas(BackdropPanel);
			mSkillDetailBackdropPanels.Add(BackdropPanel);
		}
	}

	// 공용 상세창: 디자이너 WBP(WBP_CombatDetailOverlay)를 인스턴스화하고 named widget만 캐시한다.
	// 레이아웃/아트(스크림·프레임·배치)는 WBP가 소유, C++은 아이콘/제목/부제/본문 내용만 채운다.
	if (mDetailOverlay == nullptr && mDetailOverlayClass != nullptr)
	{
		mDetailOverlay = CreateWidget<UUserWidget>(this, mDetailOverlayClass);
		if (mDetailOverlay != nullptr)
		{
			mDetailIconImage = Cast<UImage>(mDetailOverlay->GetWidgetFromName(TEXT("DetailIconImage")));
			mDetailTitleText = Cast<UTextBlock>(mDetailOverlay->GetWidgetFromName(TEXT("DetailTitleText")));
			mDetailSubtitleText = Cast<UTextBlock>(mDetailOverlay->GetWidgetFromName(TEXT("DetailSubtitleText")));
			mDetailBodyText = Cast<UTextBlock>(mDetailOverlay->GetWidgetFromName(TEXT("DetailBodyText")));
			mDetailOverlay->SetVisibility(ESlateVisibility::Collapsed);
			RootCanvas->AddChildToCanvas(mDetailOverlay);   // 풀뷰포트 RootCanvas — 스킨/HP바 위로 뜨게.
		}
	}

	// 장비 슬롯 롱프레스 감지용 투명 버튼(마커 HUD_M_equip_0..2 위에 얹는다). 위치는 Layout에서 마커 슬롯으로 지정.
	// 장비 아이콘은 WBP 마커라 입력을 못 받으므로, 이 버튼의 press/release로 스킬과 동일하게 롱프레스를 감지한다.
	if (mEquipSlotButtons.Num() == 0)
	{
		for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
		{
			UIndexedButtonWidget* EquipButton = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
				UIndexedButtonWidget::StaticClass(),
				FName(*FString::Printf(TEXT("EquipSlotButton_%d"), SlotIndex)));
			if (EquipButton == nullptr)
			{
				continue;
			}
			EquipButton->SetButtonIndex(SlotIndex);
			EquipButton->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.01f));   // 거의 투명(입력만 받음).
			EquipButton->OnIndexedPressed.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleEquipButtonPressed);
			EquipButton->OnReleased.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleEquipButtonReleased);
			EquipButton->SetVisibility(ESlateVisibility::Visible);
			TargetRootCanvas->AddChildToCanvas(EquipButton);
			mEquipSlotButtons.Add(EquipButton);
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
			// Lv/HP/Gold는 concept 값 라벨(HUD_M_*)이 보여주므로 합쳐진 단일 상태줄은 숨긴다.
			mCombatStatusBarText->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mCombatStatusBarText);
		}
	}

	// 공개된 적 전술은 기존 HUD 위 작은 우상단 패널로만 추가한다. 전장을 덮는 새 화면이나 모달 입력막은 만들지 않는다.
	if (mEnemyIntentPanel == nullptr)
	{
		mEnemyIntentPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EnemyIntentPanel"));
		mEnemyIntentList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EnemyIntentList"));
		if (mEnemyIntentPanel != nullptr && mEnemyIntentList != nullptr)
		{
			mEnemyIntentPanel->SetBrushColor(FLinearColor(0.025f, 0.045f, 0.052f, 0.72f));
			mEnemyIntentPanel->SetPadding(FMargin(8.0f, 7.0f));
			mEnemyIntentPanel->SetHorizontalAlignment(HAlign_Fill);
			mEnemyIntentPanel->SetVerticalAlignment(VAlign_Fill);
			mEnemyIntentPanel->AddChild(mEnemyIntentList);
			mEnemyIntentPanel->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mEnemyIntentPanel);
		}
	}

	// 같은 타일 재클릭 대신 행동 이름·결과를 확인하고 명시적으로 실행/취소하는 작은 카드다.
	if (mDisplacementConfirmPanel == nullptr)
	{
		mDisplacementConfirmPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DisplacementConfirmPanel"));
		mDisplacementConfirmContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DisplacementConfirmContent"));
		mDisplacementConfirmTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplacementConfirmTitle"));
		mDisplacementConfirmSummary = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplacementConfirmSummary"));
		mDisplacementConfirmButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DisplacementConfirmButtons"));
		mDisplacementCancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DisplacementCancelButton"));
		mDisplacementCancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplacementCancelText"));
		mDisplacementExecuteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DisplacementExecuteButton"));
		mDisplacementExecuteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplacementExecuteText"));
		if (mDisplacementConfirmPanel != nullptr && mDisplacementConfirmContent != nullptr
			&& mDisplacementConfirmTitle != nullptr && mDisplacementConfirmSummary != nullptr
			&& mDisplacementConfirmButtons != nullptr && mDisplacementCancelButton != nullptr
			&& mDisplacementCancelText != nullptr && mDisplacementExecuteButton != nullptr
			&& mDisplacementExecuteText != nullptr)
		{
			mDisplacementConfirmPanel->SetBrushColor(FLinearColor(0.018f, 0.045f, 0.060f, 0.94f));
			mDisplacementConfirmPanel->SetPadding(FMargin(12.0f, 8.0f));
			mDisplacementConfirmPanel->AddChild(mDisplacementConfirmContent);
			mDisplacementConfirmTitle->SetJustification(ETextJustify::Center);
			mDisplacementConfirmTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.58f, 1.0f, 0.92f, 1.0f)));
			FSlateFontInfo ConfirmTitleFont = mDisplacementConfirmTitle->GetFont();
			ConfirmTitleFont.Size = 18;
			ConfirmTitleFont.OutlineSettings.OutlineSize = 1;
			mDisplacementConfirmTitle->SetFont(ConfirmTitleFont);
			mDisplacementConfirmContent->AddChildToVerticalBox(mDisplacementConfirmTitle);

			mDisplacementConfirmSummary->SetJustification(ETextJustify::Center);
			mDisplacementConfirmSummary->SetAutoWrapText(false);
			mDisplacementConfirmSummary->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			FSlateFontInfo ConfirmSummaryFont = mDisplacementConfirmSummary->GetFont();
			ConfirmSummaryFont.Size = 14;
			mDisplacementConfirmSummary->SetFont(ConfirmSummaryFont);
			if (UVerticalBoxSlot* SummarySlot = mDisplacementConfirmContent->AddChildToVerticalBox(mDisplacementConfirmSummary))
			{
				SummarySlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 7.0f));
			}

			mDisplacementCancelText->SetText(FText::FromString(TEXT("다시 선택")));
			mDisplacementCancelText->SetJustification(ETextJustify::Center);
			mDisplacementCancelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.94f, 0.96f, 1.0f)));
			mDisplacementCancelButton->SetBackgroundColor(FLinearColor(0.16f, 0.21f, 0.24f, 1.0f));
			mDisplacementCancelButton->AddChild(mDisplacementCancelText);
			mDisplacementCancelButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleDisplacementCancelClicked);
			if (UHorizontalBoxSlot* CancelSlot = mDisplacementConfirmButtons->AddChildToHorizontalBox(mDisplacementCancelButton))
			{
				CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				CancelSlot->SetPadding(FMargin(0.0f, 0.0f, 5.0f, 0.0f));
			}

			mDisplacementExecuteText->SetText(FText::FromString(TEXT("실행")));
			mDisplacementExecuteText->SetJustification(ETextJustify::Center);
			mDisplacementExecuteText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			mDisplacementExecuteButton->SetBackgroundColor(FLinearColor(0.88f, 0.48f, 0.05f, 1.0f));
			mDisplacementExecuteButton->AddChild(mDisplacementExecuteText);
			mDisplacementExecuteButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleDisplacementConfirmClicked);
			if (UHorizontalBoxSlot* ExecuteSlot = mDisplacementConfirmButtons->AddChildToHorizontalBox(mDisplacementExecuteButton))
			{
				ExecuteSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				ExecuteSlot->SetPadding(FMargin(5.0f, 0.0f, 0.0f, 0.0f));
			}
			mDisplacementConfirmContent->AddChildToVerticalBox(mDisplacementConfirmButtons);
			mDisplacementConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mDisplacementConfirmPanel);
		}
	}

	// 월드 표식은 화면을 가리는 패널이 아니라 투영된 선/아이콘만 RootCanvas 위에 놓는다.
	if (mDisplacementPathSegments.IsEmpty())
	{
		for (int32 Index = 0; Index < 12; ++Index)
		{
			UBorder* Segment = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("DisplacementPathSegment_%d"), Index)));
			if (Segment == nullptr) { continue; }
			Segment->SetVisibility(ESlateVisibility::Collapsed);
			Segment->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Segment))
			{
				CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				CanvasSlot->SetAutoSize(false);
				CanvasSlot->SetZOrder(900);
			}
			mDisplacementPathSegments.Add(Segment);
		}
	}
	if (mDisplacementDirectionLabels.IsEmpty())
	{
		for (int32 Index = 0; Index < 8; ++Index)
		{
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
				FName(*FString::Printf(TEXT("DisplacementDirection_%d"), Index)));
			if (Label == nullptr) { continue; }
			Label->SetJustification(ETextJustify::Center);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.58f, 0.08f, 1.0f)));
			FSlateFontInfo ArrowFont = Label->GetFont();
			ArrowFont.Size = 32;
			ArrowFont.OutlineSettings.OutlineSize = 2;
			ArrowFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			Label->SetFont(ArrowFont);
			Label->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Label))
			{
				CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetAutoSize(true);
				CanvasSlot->SetZOrder(905);
			}
			mDisplacementDirectionLabels.Add(Label);
		}
	}
	auto MakeWorldLabel = [this](const TCHAR* Name, const FLinearColor& Color, int32 FontSize) -> UTextBlock*
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
		if (Label == nullptr) { return nullptr; }
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FontSize;
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Label->SetFont(Font);
		Label->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Label))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetZOrder(908);
		}
		return Label;
	};
	if (mDisplacementTargetLabel == nullptr)
	{
		mDisplacementTargetLabel = MakeWorldLabel(TEXT("DisplacementTargetLabel"), FLinearColor(0.45f, 1.0f, 0.92f, 1.0f), 18);
	}
	if (mDisplacementLandingLabel == nullptr)
	{
		mDisplacementLandingLabel = MakeWorldLabel(TEXT("DisplacementLandingLabel"), FLinearColor(0.55f, 1.0f, 0.80f, 1.0f), 17);
	}
	if (mDisplacementCollisionLabel == nullptr)
	{
		mDisplacementCollisionLabel = MakeWorldLabel(TEXT("DisplacementCollisionLabel"), FLinearColor(1.0f, 0.25f, 0.08f, 1.0f), 22);
	}
	if (mDisplacementLandingGhost == nullptr)
	{
		mDisplacementLandingGhost = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DisplacementLandingGhost"));
		if (mDisplacementLandingGhost != nullptr)
		{
			mDisplacementLandingGhost->SetColorAndOpacity(FLinearColor(0.40f, 1.0f, 0.86f, 0.42f));
			mDisplacementLandingGhost->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(mDisplacementLandingGhost))
			{
				CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetSize(FVector2D(58.0f, 58.0f));
				CanvasSlot->SetZOrder(904);
			}
		}
	}

	// 첫 전투 튜토리얼은 설명 페이지가 아니라 현재 누를 곳 하나만 가리키는 코치마크다.
	if (mEnemyIntentTutorialPanel == nullptr)
	{
		mEnemyIntentTutorialPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EnemyIntentTutorialPanel"));
		mEnemyIntentTutorialContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EnemyIntentTutorialContent"));
		mEnemyIntentTutorialTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnemyIntentTutorialTitle"));
		mEnemyIntentTutorialText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnemyIntentTutorialText"));
		mEnemyIntentTutorialFlow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EnemyIntentTutorialFlow"));
		mEnemyIntentTutorialContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EnemyIntentTutorialContinueButton"));
		mEnemyIntentTutorialContinueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnemyIntentTutorialContinueText"));
		mEnemyIntentTutorialProgress = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EnemyIntentTutorialProgress"));

		if (mEnemyIntentTutorialPanel != nullptr
			&& mEnemyIntentTutorialContent != nullptr
			&& mEnemyIntentTutorialTitle != nullptr
			&& mEnemyIntentTutorialText != nullptr
			&& mEnemyIntentTutorialFlow != nullptr
			&& mEnemyIntentTutorialContinueButton != nullptr
			&& mEnemyIntentTutorialContinueText != nullptr
			&& mEnemyIntentTutorialProgress != nullptr)
		{
			mEnemyIntentTutorialPanel->SetBrushColor(FLinearColor(0.018f, 0.035f, 0.045f, 0.90f));
			mEnemyIntentTutorialPanel->SetPadding(FMargin(12.0f, 6.0f));
			mEnemyIntentTutorialPanel->SetHorizontalAlignment(HAlign_Fill);
			mEnemyIntentTutorialPanel->SetVerticalAlignment(VAlign_Center);
			mEnemyIntentTutorialPanel->AddChild(mEnemyIntentTutorialContent);

			mEnemyIntentTutorialTitle->SetText(FText::FromString(TEXT("1 / 5   왼쪽 스킬 클릭")));
			mEnemyIntentTutorialTitle->SetJustification(ETextJustify::Center);
			mEnemyIntentTutorialTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.28f, 1.0f)));
			FSlateFontInfo TitleFont = mEnemyIntentTutorialTitle->GetFont();
			TitleFont.Size = 19;
			TitleFont.OutlineSettings.OutlineSize = 1;
			TitleFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			mEnemyIntentTutorialTitle->SetFont(TitleFont);
			if (UVerticalBoxSlot* TitleSlot = mEnemyIntentTutorialContent->AddChildToVerticalBox(mEnemyIntentTutorialTitle))
			{
				TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
			}

			mEnemyIntentTutorialFlowCards.Reset();
			mEnemyIntentTutorialFlowTexts.Reset();
			const TCHAR* FlowLabels[] = { TEXT("① 적 계획"), TEXT("② 방향 선택"), TEXT("③ 새 계획 · 이동력↓") };
			for (int32 FlowIndex = 0; FlowIndex < UE_ARRAY_COUNT(FlowLabels); ++FlowIndex)
			{
				UBorder* FlowCard = WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(),
					FName(*FString::Printf(TEXT("EnemyIntentTutorialFlowCard_%d"), FlowIndex)));
				UTextBlock* FlowText = WidgetTree->ConstructWidget<UTextBlock>(
					UTextBlock::StaticClass(),
					FName(*FString::Printf(TEXT("EnemyIntentTutorialFlowText_%d"), FlowIndex)));
				if (FlowCard == nullptr || FlowText == nullptr) { continue; }
				FlowCard->SetPadding(FMargin(9.0f, 4.0f));
				FlowCard->SetBrushColor(FLinearColor(0.07f, 0.11f, 0.12f, 0.96f));
				FlowText->SetText(FText::FromString(FlowLabels[FlowIndex]));
				FlowText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.70f, 0.70f, 1.0f)));
				FSlateFontInfo FlowFont = FlowText->GetFont();
				FlowFont.Size = 13;
				FlowText->SetFont(FlowFont);
				FlowCard->AddChild(FlowText);
				if (UHorizontalBoxSlot* FlowSlot = mEnemyIntentTutorialFlow->AddChildToHorizontalBox(FlowCard))
				{
					FlowSlot->SetPadding(FMargin(0.0f, 0.0f, FlowIndex + 1 < UE_ARRAY_COUNT(FlowLabels) ? 6.0f : 0.0f, 0.0f));
					FlowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				}
				mEnemyIntentTutorialFlowCards.Add(FlowCard);
				mEnemyIntentTutorialFlowTexts.Add(FlowText);
			}
			if (UVerticalBoxSlot* FlowSlot = mEnemyIntentTutorialContent->AddChildToVerticalBox(mEnemyIntentTutorialFlow))
			{
				FlowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
			}
			mEnemyIntentTutorialFlow->SetVisibility(ESlateVisibility::Collapsed);

			mEnemyIntentTutorialText->SetAutoWrapText(true);
			mEnemyIntentTutorialText->SetLineHeightPercentage(0.94f);
			mEnemyIntentTutorialText->SetJustification(ETextJustify::Center);
			mEnemyIntentTutorialText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 1.0f, 0.98f, 1.0f)));
			mEnemyIntentTutorialText->SetVisibility(ESlateVisibility::HitTestInvisible);
			FSlateFontInfo TutorialFont = mEnemyIntentTutorialText->GetFont();
			TutorialFont.Size = 15;
			TutorialFont.OutlineSettings.OutlineSize = 1;
			TutorialFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.90f);
			mEnemyIntentTutorialText->SetFont(TutorialFont);
			if (UVerticalBoxSlot* TextSlot = mEnemyIntentTutorialContent->AddChildToVerticalBox(mEnemyIntentTutorialText))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Fill);
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}

			mEnemyIntentTutorialContinueButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleEnemyIntentTutorialContinue);
			mEnemyIntentTutorialContinueButton->AddChild(mEnemyIntentTutorialContinueText);
			mEnemyIntentTutorialContinueButton->SetBackgroundColor(FLinearColor(0.82f, 0.48f, 0.06f, 1.0f));
			mEnemyIntentTutorialContinueText->SetText(FText::FromString(TEXT("경로를 확인했어요  →  직접 해보기")));
			mEnemyIntentTutorialContinueText->SetJustification(ETextJustify::Center);
			mEnemyIntentTutorialContinueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			FSlateFontInfo ContinueFont = mEnemyIntentTutorialContinueText->GetFont();
			ContinueFont.Size = 14;
			mEnemyIntentTutorialContinueText->SetFont(ContinueFont);
			mEnemyIntentTutorialContinueButton->SetVisibility(ESlateVisibility::Collapsed);
			if (UVerticalBoxSlot* ContinueSlot = mEnemyIntentTutorialContent->AddChildToVerticalBox(mEnemyIntentTutorialContinueButton))
			{
				ContinueSlot->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));
				ContinueSlot->SetHorizontalAlignment(HAlign_Fill);
			}

			mEnemyIntentTutorialProgressDots.Reset();
			for (int32 StepIndex = 0; StepIndex < 5; ++StepIndex)
			{
				UBorder* Dot = WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(),
					FName(*FString::Printf(TEXT("EnemyIntentTutorialStep_%d"), StepIndex)));
				if (Dot == nullptr)
				{
					continue;
				}
				Dot->SetPadding(FMargin(5.0f));
				Dot->SetBrushColor(FLinearColor(0.22f, 0.28f, 0.29f, 0.95f));
				Dot->SetVisibility(ESlateVisibility::HitTestInvisible);
				if (UHorizontalBoxSlot* DotSlot = mEnemyIntentTutorialProgress->AddChildToHorizontalBox(Dot))
				{
					DotSlot->SetPadding(FMargin(4.0f, 0.0f));
					DotSlot->SetHorizontalAlignment(HAlign_Center);
					DotSlot->SetVerticalAlignment(VAlign_Center);
				}
				mEnemyIntentTutorialProgressDots.Add(Dot);
			}
			if (UVerticalBoxSlot* ProgressSlot = mEnemyIntentTutorialContent->AddChildToVerticalBox(mEnemyIntentTutorialProgress))
			{
				ProgressSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
				ProgressSlot->SetHorizontalAlignment(HAlign_Center);
				ProgressSlot->SetVerticalAlignment(VAlign_Center);
			}
			mEnemyIntentTutorialProgress->SetVisibility(ESlateVisibility::Collapsed);
			mEnemyIntentTutorialPanel->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mEnemyIntentTutorialPanel);
		}
	}

	// 포커스는 DesignCanvas와 월드 HP바 어느 쪽도 정확히 감쌀 수 있게 풀뷰포트 RootCanvas에 둔다.
	// 선분은 모두 HitTestInvisible이라 강조 대상의 실제 버튼/월드 입력을 가로채지 않는다.
	if (mEnemyIntentTutorialFocusEdges.Num() != 4)
	{
		for (UBorder* Edge : mEnemyIntentTutorialFocusEdges)
		{
			if (Edge != nullptr) { Edge->RemoveFromParent(); }
		}
		mEnemyIntentTutorialFocusEdges.Reset();
		for (int32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
		{
			UBorder* Edge = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("EnemyIntentTutorialFocusEdge_%d"), EdgeIndex)));
			if (Edge == nullptr) { continue; }
			Edge->SetBrushColor(FLinearColor(1.0f, 0.66f, 0.06f, 1.0f));
			Edge->SetVisibility(ESlateVisibility::Collapsed);
			Edge->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			if (UCanvasPanelSlot* EdgeSlot = RootCanvas->AddChildToCanvas(Edge))
			{
				EdgeSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				EdgeSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				EdgeSlot->SetAutoSize(false);
				EdgeSlot->SetZOrder(1050);
			}
			mEnemyIntentTutorialFocusEdges.Add(Edge);
		}
	}

	if (mEnemyIntentTutorialArrowParts.Num() != 3)
	{
		for (UBorder* Part : mEnemyIntentTutorialArrowParts)
		{
			if (Part != nullptr) { Part->RemoveFromParent(); }
		}
		mEnemyIntentTutorialArrowParts.Reset();
		for (int32 PartIndex = 0; PartIndex < 3; ++PartIndex)
		{
			UBorder* Part = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("EnemyIntentTutorialArrowPart_%d"), PartIndex)));
			if (Part == nullptr) { continue; }
			Part->SetBrushColor(FLinearColor(1.0f, 0.66f, 0.06f, 1.0f));
			Part->SetVisibility(ESlateVisibility::Collapsed);
			Part->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			if (UCanvasPanelSlot* PartSlot = RootCanvas->AddChildToCanvas(Part))
			{
				PartSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				PartSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				PartSlot->SetAutoSize(false);
				PartSlot->SetZOrder(1051);
			}
			mEnemyIntentTutorialArrowParts.Add(Part);
		}
	}

	if (mEnemyIntentTutorialDimPanels.Num() != 4)
	{
		for (UBorder* DimPanel : mEnemyIntentTutorialDimPanels)
		{
			if (DimPanel != nullptr) { DimPanel->RemoveFromParent(); }
		}
		mEnemyIntentTutorialDimPanels.Reset();
		for (int32 DimIndex = 0; DimIndex < 4; ++DimIndex)
		{
			UBorder* DimPanel = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("EnemyIntentTutorialDim_%d"), DimIndex)));
			if (DimPanel == nullptr) { continue; }
			DimPanel->SetBrushColor(FLinearColor(0.0f, 0.01f, 0.015f, 0.24f));
			DimPanel->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* DimSlot = RootCanvas->AddChildToCanvas(DimPanel))
			{
				DimSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				DimSlot->SetAutoSize(false);
				DimSlot->SetZOrder(210);
			}
			mEnemyIntentTutorialDimPanels.Add(DimPanel);
		}
	}

	if (mEnemyIntentTutorialPointerLabel == nullptr)
	{
		mEnemyIntentTutorialPointerLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EnemyIntentTutorialPointerLabel"));
		if (mEnemyIntentTutorialPointerLabel != nullptr)
		{
			mEnemyIntentTutorialPointerLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.86f, 0.26f, 1.0f)));
			FSlateFontInfo PointerFont = mEnemyIntentTutorialPointerLabel->GetFont();
			PointerFont.Size = 18;
			PointerFont.OutlineSettings.OutlineSize = 2;
			PointerFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			mEnemyIntentTutorialPointerLabel->SetFont(PointerFont);
			mEnemyIntentTutorialPointerLabel->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* PointerSlot = RootCanvas->AddChildToCanvas(mEnemyIntentTutorialPointerLabel))
			{
				PointerSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				PointerSlot->SetAutoSize(true);
				PointerSlot->SetZOrder(1052);
			}
		}
	}

	if (mMoveButton == nullptr)
	{
		mMoveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MoveCommandButton"));
		if (mMoveButton != nullptr)
		{
			// 라벨은 WBP TextBlock(HUD_M_btn_move_label)이 소유한다(RefreshMoveButton이 SetText). 버튼은 투명 클릭영역.
			// 디자이너 스킨이면 버튼 배경을 투명화해 WBP의 concept MOVE 프레임이 보이게 한다.
			mMoveButton->SetBackgroundColor(IsDesignerSkinActive()
				? FLinearColor(1.0f, 1.0f, 1.0f, 0.01f)
				: FLinearColor(0.10f, 0.30f, 0.32f, 0.95f));
			mMoveButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleMoveButtonClicked);
			TargetRootCanvas->AddChildToCanvas(mMoveButton);
		}
	}

	if (EndTurnButton == nullptr)
	{
		EndTurnButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EndTurnButton"));
		if (EndTurnButton != nullptr)
		{
			// 라벨은 WBP TextBlock(HUD_M_btn_end_turn_label, 정적 "END\nTURN")이 소유. 버튼은 투명 클릭영역.
			// 디자이너 스킨이면 배경 투명화해 WBP의 concept END TURN 프레임이 보이게 한다.
			EndTurnButton->SetBackgroundColor(IsDesignerSkinActive()
				? FLinearColor(1.0f, 1.0f, 1.0f, 0.01f)
				: FLinearColor(0.32f, 0.08f, 0.07f, 0.95f));
			TargetRootCanvas->AddChildToCanvas(EndTurnButton);
		}
	}

	// 내비 투명 버튼(MAP/DICE/SKILL/SET): concept 내비 아트 위에 얹어 클릭만 받고 HUD 소유 패널 토글을 호출한다.
	if (IsDesignerSkinActive())
	{
		auto MakeNavButton = [&](TObjectPtr<UButton>& OutButton, const TCHAR* Name)
		{
			if (OutButton == nullptr)
			{
				OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
				if (OutButton != nullptr)
				{
					OutButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.01f));   // 항상 투명: concept 내비 아트가 보이게.
					TargetRootCanvas->AddChildToCanvas(OutButton);
				}
			}
		};
		MakeNavButton(mNavMapButton, TEXT("NavMapButton"));
		MakeNavButton(mNavDiceButton, TEXT("NavDiceButton"));
		MakeNavButton(mNavSkillButton, TEXT("NavSkillButton"));
		MakeNavButton(mNavSettingsButton, TEXT("NavSettingsButton"));
		if (mNavMapButton != nullptr)
		{
			mNavMapButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleNavMapButtonClicked);
		}
		if (mNavDiceButton != nullptr)
		{
			mNavDiceButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleNavDiceButtonClicked);
		}
		if (mNavSkillButton != nullptr)
		{
			mNavSkillButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleNavSkillButtonClicked);
		}
		if (mNavSettingsButton != nullptr)
		{
			mNavSettingsButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleNavSettingsButtonClicked);
		}
	}

	RebuildSkillRailWidgets();
	EnsureSkillInputButtons();
	EnsureContextActionWidgets();
	RebuildEquipmentBar();    // 탑바 좌측 하단 장비 칩(뷰모델 미연결이면 비워 둠)
	RefreshEnemyIntentPanel();
	UpdateEnemyIntentTutorial();
	ApplyRuntimeWidgetLayout();
}
