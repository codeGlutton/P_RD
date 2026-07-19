#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"   // 골드 카운트업 코인 사운드 재생
#include "TimerManager.h"             // 골드 카운트업 반복 타이머
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Combat/CombatUIModel.h"
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

	// 턴 변화 연출은 턴/유닛 도메인이 갱신될 때 다시 평가한다.
	if (Domain == ECombatUIDomain::Turn
		|| Domain == ECombatUIDomain::Unit
		|| Domain == ECombatUIDomain::All)
	{
		// 라운드가 실제로 올랐을 때만 "N번째 턴" 배너를 띄운다(내부에서 중복 방지).
		RefreshTurnRoundBanner();
	}

	// 유닛 수가 바뀌면 머리 위 HP바도 다시 만든다.
	if (Domain == ECombatUIDomain::Unit || Domain == ECombatUIDomain::All)
	{
		RebuildUnitHpBars();
		RefreshContextActions();
	}

	// 주사위(굴림/사용) 갱신 시 보유 주사위 표시를 다시 읽어 그린다(쓴 주사위 비활성 반영).
	if (Domain == ECombatUIDomain::Dice || Domain == ECombatUIDomain::All)
	{
		RefreshDiceViewsFromRunData();
		RefreshOwnedDiceCards();
		RefreshContextActions();
		TrySubmitContextTargetWhenReady();
	}

	// 스킬: 보유 스킬 스냅샷이 바뀌면 레일 슬롯 상태(아이콘/라벨/빈칸 커버)를 다시 그린다.
	if (Domain == ECombatUIDomain::Skill || Domain == ECombatUIDomain::All)
	{
		RefreshSkillRailWidgets();
		RefreshDiceAssignmentText();
		RefreshOwnedDiceCards();
		TrySubmitContextTargetWhenReady();
	}

	if (Domain == ECombatUIDomain::Turn || Domain == ECombatUIDomain::All)
	{
		RefreshContextActions();
	}

	// 공개 계획과 실행 결과는 별도 Intent 도메인으로 부분 갱신한다.
	if (Domain == ECombatUIDomain::Intent || Domain == ECombatUIDomain::All)
	{
		RefreshEnemyIntentPanel();
		// 밀기 보고가 들어온 프레임에 머리 위 계획 배지도 새 위치/상태로 즉시 갱신한다.
		UpdateUnitHpBars();
	}

	if (Domain == ECombatUIDomain::DisplacementPreview
		|| Domain == ECombatUIDomain::Turn
		|| Domain == ECombatUIDomain::All)
	{
		RefreshDisplacementPreview();
	}

	// 튜토리얼은 예고뿐 아니라 실제 스킬/주사위 선택과 턴 변화를 관찰해 자동으로 다음 단계로 간다.
	if (Domain == ECombatUIDomain::Intent
		|| Domain == ECombatUIDomain::Skill
		|| Domain == ECombatUIDomain::Dice
		|| Domain == ECombatUIDomain::Turn
		|| Domain == ECombatUIDomain::DisplacementPreview
		|| Domain == ECombatUIDomain::All)
	{
		UpdateEnemyIntentTutorial();
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

	// 로컬 상태줄(스킨 모드에선 접힘)에도 값은 채워둔다 — 실제 표시는 concept 값 라벨(RefreshSkinValueLabels).
	mCombatStatusBarText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "CombatStatusBarFormat", "Lv {0}    HP {1}/{2}    GOLD {3}"),
		FText::AsNumber(Level), FText::AsNumber(HP), FText::AsNumber(MaxHP), FText::AsNumber(Gold)));

	// 스킨 모드(concept_02): 합쳐진 단일 상태줄은 숨긴다. Lv/HP/Gold는 concept 값 칸(HUD_M_*)에
	// 개별 텍스트로 그린다(RefreshSkinValueLabels).
	if (IsDesignerSkinActive() && mCombatStatusBarText != nullptr)
	{
		mCombatStatusBarText->SetVisibility(ESlateVisibility::Collapsed);
	}

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
		{ TEXT("HUD_M_btn_end_turn_label"), NSLOCTEXT("CombatTileMapHUDWidget", "EndTurnLabel", "턴\n종료") },
	};

	for (const FSkinValueBinding& Binding : Bindings)
	{
		if (UTextBlock* ValueText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(Binding.WidgetName))))
		{
			ValueText->SetText(Binding.Text);
			ValueText->SetJustification(ETextJustify::Center);
		}
	}

	// 골드 칸은 즉시 그리지 않고 카운트업 경로로 — 증가 시 코인 사운드와 함께 차르륵 올라간다.
	UpdateGoldValueLabel(Meta.mGold);
}

bool UCombatTileMapHUDWidget::IsScreenPositionOverLevelValue(const FVector2D& ScreenPosition) const
{
	if (WidgetTree == nullptr || IsDesignerSkinActive() == false)
	{
		return false;
	}

	const UWidget* LevelText = WidgetTree->FindWidget(TEXT("HUD_M_lv_value"));
	if (LevelText == nullptr || LevelText->IsVisible() == false)
	{
		return false;
	}

	const FGeometry& LevelGeometry = LevelText->GetCachedGeometry();
	if (LevelGeometry.GetLocalSize().X <= 0.0f || LevelGeometry.GetLocalSize().Y <= 0.0f)
	{
		return false;
	}

	constexpr float TouchPadding = 24.0f;
	const FVector2D TopLeft = LevelGeometry.GetAbsolutePosition();
	const FVector2D BottomRight = TopLeft + LevelGeometry.GetAbsoluteSize();
	return ScreenPosition.X >= TopLeft.X - TouchPadding && ScreenPosition.X <= BottomRight.X + TouchPadding
		&& ScreenPosition.Y >= TopLeft.Y - TouchPadding && ScreenPosition.Y <= BottomRight.Y + TouchPadding;
}

void UCombatTileMapHUDWidget::EnsureExpHoldPanel()
{
	if (mExpHoldPanel != nullptr || WidgetTree == nullptr)
	{
		return;
	}

	UTextBlock* LevelText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HUD_M_lv_value")));
	UCanvasPanelSlot* LevelSlot = LevelText != nullptr ? Cast<UCanvasPanelSlot>(LevelText->Slot) : nullptr;
	UCanvasPanel* ParentCanvas = LevelText != nullptr ? Cast<UCanvasPanel>(LevelText->GetParent()) : nullptr;
	if (LevelSlot == nullptr || ParentCanvas == nullptr)
	{
		return;
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HUD_ExpHoldPanel"));
	UTextBlock* PanelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HUD_ExpHoldText"));
	if (Panel == nullptr || PanelText == nullptr)
	{
		return;
	}

	Panel->SetBrushColor(FLinearColor(0.10f, 0.10f, 0.12f, 0.88f));
	Panel->SetPadding(FMargin(12.0f, 6.0f));
	Panel->SetVisibility(ESlateVisibility::Collapsed);

	FSlateFontInfo PanelFont = PanelText->GetFont();
	PanelFont.Size = 20;
	PanelFont.OutlineSettings.OutlineSize = 2;
	PanelFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
	PanelText->SetFont(PanelFont);
	PanelText->SetJustification(ETextJustify::Center);
	PanelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Panel->SetContent(PanelText);

	if (UCanvasPanelSlot* PanelSlot = ParentCanvas->AddChildToCanvas(Panel))
	{
		const FMargin LevelOffsets = LevelSlot->GetOffsets();
		PanelSlot->SetAnchors(LevelSlot->GetAnchors());
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetPosition(FVector2D(
			LevelOffsets.Left + LevelOffsets.Right * 0.5f,
			LevelOffsets.Top + LevelOffsets.Bottom + 10.0f));
		PanelSlot->SetZOrder(60);
	}

	mExpHoldPanel = Panel;
	mExpHoldText = PanelText;
}

void UCombatTileMapHUDWidget::SetExpHoldPanelVisible(bool bVisible)
{
	if (bVisible)
	{
		EnsureExpHoldPanel();
	}
	if (mExpHoldPanel == nullptr)
	{
		return;
	}

	if (bVisible && mExpHoldText != nullptr && mCombatUIModel != nullptr)
	{
		const FPlayerMetaUI& Meta = mCombatUIModel->GetPlayerMeta();
		mExpHoldText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "ExpHoldPanel", "EXP {0}/{1}"),
			FText::AsNumber(FMath::RoundToInt(Meta.mExp)), FText::AsNumber(FMath::RoundToInt(Meta.mMaxExp))));
	}

	mExpHoldPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UCombatTileMapHUDWidget::UpdateGoldValueLabel(int32 NewGold) const
{
	// 최초 표시(전투 진입)나 감소는 연출 없이 즉시 반영한다 — 차르륵은 '획득' 연출.
	if (mDisplayedGold == INDEX_NONE || NewGold < mDisplayedGold)
	{
		if (GetWorld() != nullptr)
		{
			GetWorld()->GetTimerManager().ClearTimer(mGoldCountUpTimerHandle);
		}
		mDisplayedGold = NewGold;
		SetGoldValueLabelText(NewGold);
		return;
	}

	if (NewGold == mDisplayedGold || GetWorld() == nullptr)
	{
		return;
	}

	// 증가: 목표만 갱신하고 30Hz 반복 타이머로 수렴시킨다. 이미 도는 중이면 목표만 늘어난다.
	mGoldCountUpTarget = NewGold;
	if (GetWorld()->GetTimerManager().IsTimerActive(mGoldCountUpTimerHandle) == false)
	{
		if (mCoinGainSound != nullptr)
		{
			UGameplayStatics::PlaySound2D(this, mCoinGainSound);
		}
		GetWorld()->GetTimerManager().SetTimer(
			mGoldCountUpTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCombatTileMapHUDWidget::TickGoldCountUp),
			0.033f, true);
	}
}

void UCombatTileMapHUDWidget::TickGoldCountUp() const
{
	// 남은 차이의 ~18%씩(최소 1) 올려 자연스럽게 감속하며 0.5~0.8초 안에 목표에 붙는다.
	const int32 Remaining = mGoldCountUpTarget - mDisplayedGold;
	const int32 Step = FMath::Max(1, FMath::RoundToInt(StaticCast<float>(Remaining) * 0.18f));
	mDisplayedGold = FMath::Min(mDisplayedGold + Step, mGoldCountUpTarget);
	SetGoldValueLabelText(mDisplayedGold);

	if (mDisplayedGold >= mGoldCountUpTarget && GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(mGoldCountUpTimerHandle);
	}
}

void UCombatTileMapHUDWidget::SetGoldValueLabelText(int32 Gold) const
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	if (UTextBlock* GoldText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HUD_M_gold_value"))))
	{
		GoldText->SetText(FText::AsNumber(Gold));
		GoldText->SetJustification(ETextJustify::Center);
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
	for (const FUnitUI& Unit : mCombatUIModel->GetUnitUIs())
	{
		if (Unit.mIsPlayer)
		{
			Move = FMath::RoundToInt(Unit.mMovementPoint);
			break;
		}
	}

	// 남은 이동력만 표시한다 - 최대치는 플레이어 판단에 불필요해 노출하지 않는다.
	MoveLabel->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandCount", "이동\n{0}"),
		FText::AsNumber(Move)));
	MoveLabel->SetJustification(ETextJustify::Center);
}

void UCombatTileMapHUDWidget::HandleMoveButtonClicked()
{
	// 이동 모드 진입 의도. 게임플레이가 이동 가능 타일을 표시하고, 타일 탭으로 이동시킨다.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestMove();
	}
}

// [아키텍처 의도] 콘셉트 HUD의 상단 내비 버튼(MAP/DICE/SKILL/SET)은 HUD가 소유한 패널 토글
// (CombatTileMapHUDWidget_Nav.cpp — 레거시 탑바에서 이관)을 직접 호출한다.
// 패널 생명주기/상호배타/승리 후 월드맵 흐름의 단일 책임자가 HUD가 되고, 탑바 경유 위임은 제거됐다.
void UCombatTileMapHUDWidget::HandleNavMapButtonClicked()
{
	ToggleWorldMap();
}

void UCombatTileMapHUDWidget::HandleNavDiceButtonClicked()
{
	ToggleFloatingPanel(EWorldWidgetType::DicePanel, TEXT("DicePanel"));
}

void UCombatTileMapHUDWidget::HandleNavSkillButtonClicked()
{
	ToggleFloatingPanel(EWorldWidgetType::SkillPanel, TEXT("SkillPanel"));
}

void UCombatTileMapHUDWidget::HandleNavSettingsButtonClicked()
{
	ToggleSettingsPanel();
}

void UCombatTileMapHUDWidget::HandleCombatActionResolved()
{
	// 스킬/주사위 선택 강조를 푼다(액션 확정·취소 후).
	mDirectUnitGestureActive = false;
	SetDirectUnitGestureVisual(false);
	mSelectedSkillIndex = INDEX_NONE;
	mDirectArmedSkillIndex = INDEX_NONE;
	mDirectArmedTargetUnitId = INDEX_NONE;
	mDirectGripGesture = false;
	mDirectGripCanSwap = false;
	mDirectGripSwapPreview = false;
	ClearOwnedDiceSelectionHighlight();
	CloseContextActions();
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

	// 디자이너 스킨(concept_02): 레거시 칩(Border+Text) 대신 탑바 레벨 아래에 장비 아이콘을 그린다.
	if (IsDesignerSkinActive())
	{
		RebuildEquipmentIcons();
		return;
	}

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

void UCombatTileMapHUDWidget::RebuildEquipmentIcons()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	// 장비 슬롯은 WBP가 소유한 마커(HUD_M_equip_0..2)다. C++는 브러시/표시여부만 채운다.
	// 위치·크기는 WBP가 소유 — 런타임 좌표 계산 없음(HUD_M_lv/hp/gold_value 값 마커와 동일 규칙).
	static const TCHAR* const SlotMarkerNames[] = { TEXT("HUD_M_equip_0"), TEXT("HUD_M_equip_1"), TEXT("HUD_M_equip_2") };
	const int32 MarkerCount = UE_ARRAY_COUNT(SlotMarkerNames);

	// 마커 i ↔ 장비 슬롯 i(무기/장갑/신발) 직접 매핑. 장착된 슬롯만 그 아이콘을 띄우고, 빈 슬롯은 감춘다.
	// 모델이 아직 없을 때도 마커를 전부 감춰서 WBP에 박힌 기본 브러시(임시 하트)가 새어 나오지 않게 한다.
	for (int32 MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		UWidget* RawMarker = WidgetTree->FindWidget(SlotMarkerNames[MarkerIndex]);
		UImage* SlotImage = Cast<UImage>(RawMarker);

		if (SlotImage == nullptr)
		{
			continue;   // 마커가 없는 스킨/구버전이면 조용히 건너뛴다.
		}

		UTexture2D* IconTex = nullptr;
		if (mCombatUIModel != nullptr)
		{
			const TArray<FEquipmentUI>& Equips = mCombatUIModel->GetEquipmentUIs();
			if (Equips.IsValidIndex(MarkerIndex) == true
				&& Equips[MarkerIndex].mIsEquipped == true
				&& Equips[MarkerIndex].mIcon != nullptr)
			{
				IconTex = Equips[MarkerIndex].mIcon.Get();
			}
		}

		if (IconTex != nullptr)
		{
			SlotImage->SetBrushFromTexture(IconTex, false);
			SlotImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			SlotImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
