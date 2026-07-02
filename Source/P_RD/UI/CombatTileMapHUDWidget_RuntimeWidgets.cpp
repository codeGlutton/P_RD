#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	// 주사위 판(물리 굴림) 보드/그림자 텍스처. 20260626 디스크에 이미 존재.
	const TCHAR* const DiceRollBoardTexturePath = TEXT("/Game/SVN/OutSideAsset/AICreation/DiceRoll/UI_DiceRoll_Board_StyleMatch.UI_DiceRoll_Board_StyleMatch");

	template <typename T>
	T* FindNamedWidget(const UWidgetTree* InWidgetTree, const TCHAR* WidgetName)
	{
		return InWidgetTree != nullptr ? Cast<T>(InWidgetTree->FindWidget(FName(WidgetName))) : nullptr;
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
	}

	// (값 텍스트 LV/HP/Gold는 더 이상 C++가 생성/정렬하지 않는다 — 빌드가 WBP의 HUD_M_lv/hp/gold_value를
	//  실제 TextBlock으로 심고 위치/폰트/정렬을 소유한다. C++는 RefreshSkinValueLabels에서 내용만 채운다.)


	// --- 주사위 판(물리 굴림) 보드/배경/캡처 위젯 (20260622 이식). 위치는 ApplyRuntimeWidgetLayout에서 잡는다. ---
	if (mDiceRollBoardTexture == nullptr)
	{
		mDiceRollBoardTexture = LoadObject<UTexture2D>(nullptr, DiceRollBoardTexturePath);
	}
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
			DiceRollStatusText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "IntroDiceRolling", "ROLLING DICE"));
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
			// Lv/HP/Gold는 concept 값 라벨(HUD_M_*)이 보여주므로 합쳐진 단일 상태줄은 숨긴다.
			mCombatStatusBarText->SetVisibility(ESlateVisibility::Collapsed);
			TargetRootCanvas->AddChildToCanvas(mCombatStatusBarText);
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
	RebuildEquipmentBar();    // 탑바 좌측 하단 장비 칩(뷰모델 미연결이면 비워 둠)
	RebuildTurnOrderBar();    // 탑바 가운데 하단 턴 순서 칩
	ApplyRuntimeWidgetLayout();
}
