#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

namespace
{
	constexpr int32 ActionSubmenuSlotCount = 3;

	FString GetActionFamilyName(int32 FamilyIndex)
	{
		switch (FamilyIndex)
		{
		case 0: return TEXT("공격");
		case 1: return TEXT("손아귀");
		case 2: return TEXT("제압");
		case 3: return TEXT("기동");
		default: return TEXT("행동");
		}
	}
}

void UCombatTileMapHUDWidget::RebuildSkillRailWidgets()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr || mSkillRailPanels.Num() == CombatSkillSlotCount)
	{
		return;
	}

	for (UBorder* SkillRailPanel : mSkillRailPanels)
	{
		if (SkillRailPanel != nullptr)
		{
			SkillRailPanel->RemoveFromParent();
		}
	}
	for (UImage* SkillRailIcon : mSkillRailIcons)
	{
		if (SkillRailIcon != nullptr)
		{
			SkillRailIcon->RemoveFromParent();
		}
	}
	for (UTextBlock* SkillRailText : mSkillRailTexts)
	{
		if (SkillRailText != nullptr)
		{
			SkillRailText->RemoveFromParent();
		}
	}
	mSkillRailPanels.Reset();
	mSkillRailIcons.Reset();
	mSkillRailTexts.Reset();

	// 슬롯 6칸의 뼈대(패널/아이콘/라벨)만 만든다. 어떤 슬롯에 무엇이 보이는지는
	// RefreshSkillRailWidgets가 보유 스킬 스냅샷(FSkillUI)만으로 결정한다(시안 라벨 폴백 없음).
	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UBorder* SkillRailPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailPanel_%d"), SkillIndex))
		);
		UImage* SkillRailIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailIcon_%d"), SkillIndex))
		);
		UTextBlock* SkillRailText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailText_%d"), SkillIndex))
		);
		if (SkillRailPanel == nullptr || SkillRailIcon == nullptr || SkillRailText == nullptr)
		{
			continue;
		}

		SkillRailPanel->SetPadding(GetCombatSkillRailPadding());
		SkillRailIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		SkillRailText->SetJustification(ETextJustify::Left);
		SkillRailText->SetAutoWrapText(false);
		SkillRailText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		SkillRailText->SetLineHeightPercentage(0.88f);
		FSlateFontInfo SkillFont = SkillRailText->GetFont();
		SkillFont.Size = 14;
		SkillFont.OutlineSettings.OutlineSize = 1;
		SkillFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.88f);
		SkillRailText->SetFont(SkillFont);
		SkillRailText->SetVisibility(ESlateVisibility::HitTestInvisible);

		// 스킨 활성 시 DesignCanvas: 렌더/히트테스트가 같은 레터박스 좌표계를 쓴다.
		// RootCanvas(뷰포트)에 붙이면 16:9가 아닐 때 레일만 따로 노는 정렬 버그가 생긴다.
		UCanvasPanel* SkillRailCanvas = GetSkinTargetCanvas();
		SkillRailCanvas->AddChildToCanvas(SkillRailPanel);
		SkillRailCanvas->AddChildToCanvas(SkillRailIcon);
		SkillRailCanvas->AddChildToCanvas(SkillRailText);

		mSkillRailPanels.Add(SkillRailPanel);
		mSkillRailIcons.Add(SkillRailIcon);
		mSkillRailTexts.Add(SkillRailText);
	}

	RefreshSkillRailWidgets();
}

/** @details 레일 고정 배치 - 기본 공격 / 손아귀 / 방해 / 이동의 네 행동군만 표시한다.
 * 슬롯 2상태 - [보유] WBP 빈 프레임 안에 아이콘+코스트를 채운다(사용불가면 흐림) / [미보유] 아무것도 안 그린다.
 * WBP에는 빈 프레임(크롬)만 있으므로 "가리기(검은 커버)"가 필요 없다. 데이터 소스는 FSkillUI 단일. */
void UCombatTileMapHUDWidget::RefreshSkillRailWidgets()
{
	int32 SkillIndex = mCombatUIModel != nullptr ? mCombatUIModel->GetSelectedSkillIndex() : INDEX_NONE;
	if (mSelectedSkillIndex != SkillIndex)
	{
		ClearOwnedDiceSelectionHighlight();
	}
	mSelectedSkillIndex = SkillIndex;

	const TArray<FSkillUI>* Skills = mCombatUIModel != nullptr ? &mCombatUIModel->GetSkillUIs() : nullptr;

	for (int32 RailSlotIndex = 0; RailSlotIndex < mSkillRailPanels.Num(); ++RailSlotIndex)
	{
		const int32 SkillDataIndex = GetSkillDataIndexForRailSlot(RailSlotIndex);
		const FSkillUI* Skill = (Skills != nullptr && Skills->IsValidIndex(SkillDataIndex)) ? &(*Skills)[SkillDataIndex] : nullptr;
		const bool bOwned = Skill != nullptr && Skill->mName.IsEmpty() == false;
		const bool bUsable = bOwned && Skill->mIsUsable;
		// 내부 스킬 index가 여러 행동군에서 재사용되므로 선택 강조는 실제로 연 행동군만 소유한다.
		const bool bSelected = bOwned
			&& (RailSlotIndex == mActiveActionFamily
				|| RailSlotIndex == mExpandedActionFamily);
		const bool bTutorialFocus = bOwned
			&& Skill->mIsDisplacementSkill
			&& Skill->mIsPullSkill
			&& (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::OpenGrip
				|| mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::SelectPull)
			&& mEnemyIntentTutorialDismissed == false;
		const float DimOpacity = bUsable ? 1.0f : 0.45f;

		if (UBorder* SkillRailPanel = mSkillRailPanels[RailSlotIndex])
		{
			if (bOwned == false)
			{
				// 미보유: 슬롯을 아예 비운다 - WBP의 빈 프레임이 그대로 "빈 칸"으로 보인다.
				// 브러시도 투명화해 둔다(방어): 외부에서 visibility를 강제 복원해도 UBorder 기본 흰 브러시가 새지 않게.
				SkillRailPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
				SkillRailPanel->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				SkillRailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				if (bTutorialFocus)
				{
					SkillRailPanel->SetBrushColor(FLinearColor(1.0f, 0.68f, 0.08f, 0.62f));
				}
				else if (IsDesignerSkinActive())
				{
					// 기존 WBP 프레임을 숨겼으므로 카드 배경도 런타임이 직접 그린다.
					SkillRailPanel->SetBrushColor(bSelected
						? FLinearColor(0.44f, 0.34f, 0.08f, 0.94f)
						: FLinearColor(0.025f, 0.055f, 0.075f, 0.88f));
				}
				else
				{
					SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(bSelected));
				}
				// 튜토리얼 포커스는 별도 RootCanvas 테두리가 담당한다. 실제 버튼 기하를 키우면
				// 렌더와 투명 입력영역이 어긋나므로 선택 상태의 원래 배율만 유지한다.
				SkillRailPanel->SetRenderScale(GetCombatSkillRailScale(bSelected));
			}
		}

		if (mSkillRailIcons.IsValidIndex(RailSlotIndex))
		{
			if (UImage* SkillRailIcon = mSkillRailIcons[RailSlotIndex])
			{
				if (bOwned && Skill->mIcon != nullptr)
				{
					SkillRailIcon->SetBrushFromTexture(Skill->mIcon, false);
					SkillRailIcon->SetRenderOpacity(DimOpacity);
					SkillRailIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					SkillRailIcon->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}

		if (mSkillRailTexts.IsValidIndex(RailSlotIndex))
		{
			if (UTextBlock* SkillRailText = mSkillRailTexts[RailSlotIndex])
			{
				if (bOwned == false)
				{
					SkillRailText->SetText(FText::GetEmpty());
					SkillRailText->SetVisibility(ESlateVisibility::Collapsed);
					continue;
				}

				const FString StateText = bUsable ? TEXT("") : TEXT(" · 사용 불가");
				FString FamilySummary;
				switch (RailSlotIndex)
				{
				case 0: FamilySummary = TEXT("베기 · 밀어베기 · 방패치기"); break;
				case 1: FamilySummary = TEXT("끌기 · 던지기 · 자리교환"); break;
				case 2: FamilySummary = TEXT("다리걸기 · 방패 밀치기"); break;
				case 3: FamilySummary = TEXT("전투 스텝 · 어깨 돌진 · 도약"); break;
				default: break;
				}
				const FString ActiveHint = RailSlotIndex == mExpandedActionFamily
					? TEXT("세부 행동을 고르세요")
					: (RailSlotIndex == mActiveActionFamily && mSelectedSubactionName.IsEmpty() == false
						? FString::Printf(TEXT("선택: %s"), *mSelectedSubactionName.ToString())
						: TEXT("탭해서 펼치기"));
				const FString CardText = FString::Printf(
					TEXT("%s\n%s\n%s%s"),
					*GetActionFamilyName(RailSlotIndex),
					*FamilySummary,
					*ActiveHint,
					*StateText);
				SkillRailText->SetText(FText::FromString(CardText));
				SkillRailText->SetColorAndOpacity(FSlateColor(bSelected || bTutorialFocus
					? FLinearColor(1.0f, 0.88f, 0.34f, 1.0f)
					: FLinearColor(0.86f, 0.96f, 1.0f, DimOpacity)));
				SkillRailText->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}

	// 타깃 우선 팔레트만으로는 적을 누르기 전에 스킬별 사거리를 비교할 수 없다.
	// 좌측 레일을 다시 상시 진입점으로 사용하고, 선택 즉시 게임플레이의 Aim 하이라이트를 켠다.
	const ESlateVisibility DockVisibility = mCombatControlsHidden
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	if (mSkillDockPanel != nullptr) { mSkillDockPanel->SetVisibility(DockVisibility); }
	if (mSkillDockTitleText != nullptr) { mSkillDockTitleText->SetVisibility(DockVisibility); }
	for (int32 RailSlotIndex = 0; RailSlotIndex < mSkillInputButtons.Num(); ++RailSlotIndex)
	{
		if (UIndexedButtonWidget* InputButton = mSkillInputButtons[RailSlotIndex])
		{
			const bool bOwned = GetSkillDataIndexForRailSlot(RailSlotIndex) != INDEX_NONE;
			InputButton->SetVisibility(
				mCombatControlsHidden == false && bOwned
					? ESlateVisibility::Visible
					: ESlateVisibility::Collapsed);
		}
	}
	RefreshContextActions();
	RefreshActionSubmenuWidgets();
}

void UCombatTileMapHUDWidget::EnsureActionSubmenuWidgets()
{
	if (WidgetTree == nullptr || GetSkinTargetCanvas() == nullptr
		|| mActionSubmenuButtons.Num() == ActionSubmenuSlotCount)
	{
		return;
	}

	mActionSubmenuTitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("ActionSubmenuTitleText"));
	if (mActionSubmenuTitleText != nullptr)
	{
		FSlateFontInfo Font = mActionSubmenuTitleText->GetFont();
		Font.Size = 17;
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		mActionSubmenuTitleText->SetFont(Font);
		mActionSubmenuTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.83f, 0.30f, 1.0f)));
		mActionSubmenuTitleText->SetJustification(ETextJustify::Left);
		mActionSubmenuTitleText->SetVisibility(ESlateVisibility::Collapsed);
		GetSkinTargetCanvas()->AddChildToCanvas(mActionSubmenuTitleText);
	}

	for (int32 SlotIndex = 0; SlotIndex < ActionSubmenuSlotCount; ++SlotIndex)
	{
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), FName(*FString::Printf(TEXT("ActionSubmenuPanel_%d"), SlotIndex)));
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), FName(*FString::Printf(TEXT("ActionSubmenuIcon_%d"), SlotIndex)));
		UTextBlock* TextWidget = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*FString::Printf(TEXT("ActionSubmenuText_%d"), SlotIndex)));
		UIndexedButtonWidget* Button = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(), FName(*FString::Printf(TEXT("ActionSubmenuButton_%d"), SlotIndex)));
		if (Panel == nullptr || Icon == nullptr || TextWidget == nullptr || Button == nullptr)
		{
			continue;
		}
		Panel->SetPadding(FMargin(5.0f));
		Panel->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.070f, 0.96f));
		Panel->SetVisibility(ESlateVisibility::Collapsed);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		TextWidget->SetJustification(ETextJustify::Left);
		TextWidget->SetAutoWrapText(false);
		TextWidget->SetLineHeightPercentage(0.90f);
		FSlateFontInfo Font = TextWidget->GetFont();
		Font.Size = 14;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		TextWidget->SetFont(Font);
		TextWidget->SetVisibility(ESlateVisibility::Collapsed);
		Button->SetButtonIndex(SlotIndex);
		Button->SetBackgroundColor(GetTransparentInputButtonColor());
		Button->OnIndexedClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleActionSubmenuClicked);
		Button->SetVisibility(ESlateVisibility::Collapsed);
		GetSkinTargetCanvas()->AddChildToCanvas(Panel);
		GetSkinTargetCanvas()->AddChildToCanvas(Icon);
		GetSkinTargetCanvas()->AddChildToCanvas(TextWidget);
		GetSkinTargetCanvas()->AddChildToCanvas(Button);
		mActionSubmenuPanels.Add(Panel);
		mActionSubmenuIcons.Add(Icon);
		mActionSubmenuTexts.Add(TextWidget);
		mActionSubmenuButtons.Add(Button);
	}
	RefreshActionSubmenuWidgets();
}

void UCombatTileMapHUDWidget::RefreshActionSubmenuWidgets()
{
	mActionSubmenuSkillIndices.Reset();
	mActionSubmenuDesiredPowers.Reset();
	mActionSubmenuModes.Reset();
	mActionSubmenuNames.Reset();
	const bool bCanShow = mExpandedActionFamily != INDEX_NONE
		&& mCombatControlsHidden == false
		&& mCombatUIModel != nullptr;
	if (bCanShow)
	{
		const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
		auto AddEntry = [this, &Skills](
			int32 SkillIndex,
			const TCHAR* Name,
			const TCHAR* Hint,
			int32 DesiredPower,
			ECombatSubactionMode Mode)
		{
			if (Skills.IsValidIndex(SkillIndex) == false || Skills[SkillIndex].mIsUsable == false
				|| mActionSubmenuSkillIndices.Num() >= ActionSubmenuSlotCount)
			{
				return;
			}
			mActionSubmenuSkillIndices.Add(SkillIndex);
			mActionSubmenuDesiredPowers.Add(DesiredPower);
			mActionSubmenuModes.Add(Mode);
			mActionSubmenuNames.Add(FText::FromString(Name));
			const int32 AddedIndex = mActionSubmenuSkillIndices.Num() - 1;
			if (mActionSubmenuTexts.IsValidIndex(AddedIndex) && mActionSubmenuTexts[AddedIndex] != nullptr)
			{
				mActionSubmenuTexts[AddedIndex]->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), Name, Hint)));
			}
		};
		const int32 PullIndex = FindDirectSkillIndex(&FSkillUI::mIsPullSkill);
		const int32 ThrowIndex = FindDirectSkillIndex(&FSkillUI::mIsThrowSkill);
		const int32 StaggerIndex = FindDirectSkillIndex(&FSkillUI::mIsStaggerSkill);
		const int32 SwapIndex = FindDirectSkillIndex(&FSkillUI::mIsSwapSkill);
		switch (mExpandedActionFamily)
		{
		case 0:
			AddEntry(0, TEXT("베기"), TEXT("적 탭 · 즉시 공격"), 3, ECombatSubactionMode::BasicAttack);
			AddEntry(0, TEXT("회전베기"), TEXT("주변 8칸 · 모든 적 8 피해"), 3, ECombatSubactionMode::Whirlwind);
			AddEntry(0, TEXT("충격파"), TEXT("주변 8칸 · 4 피해 + 바깥으로 밀침"), 4, ECombatSubactionMode::Shockwave);
			break;
		case 1:
			AddEntry(PullIndex, TEXT("끌어오기"), TEXT("적 드래그 · 기사 주변 배치"), 6, ECombatSubactionMode::Pull);
			AddEntry(ThrowIndex, TEXT("집어던지기"), TEXT("인접 적 드래그 · 충돌"), 6, ECombatSubactionMode::Throw);
			AddEntry(SwapIndex, TEXT("자리 바꾸기"), TEXT("인접 적 탭 · 즉시 교환"), 4, ECombatSubactionMode::Swap);
			break;
		case 2:
			AddEntry(StaggerIndex, TEXT("다리 걸기"), TEXT("적 탭 · 다음 이동 감소"), 6, ECombatSubactionMode::Stagger);
			AddEntry(ThrowIndex, TEXT("방패 밀치기"), TEXT("인접 적 드래그 · 진형 붕괴"), 1, ECombatSubactionMode::ShortThrow);
			break;
		case 3:
			AddEntry(1, TEXT("전투 스텝"), TEXT("기사를 드래그 · 1칸 재배치"), 1, ECombatSubactionMode::Move);
			AddEntry(1, TEXT("어깨 돌진"), TEXT("기사를 직선 드래그 · 1칸 충돌"), 6, ECombatSubactionMode::Charge);
			AddEntry(1, TEXT("도약"), TEXT("기사를 드래그 · 장애물 넘어 3칸 착지"), 3, ECombatSubactionMode::Leap);
			break;
		default:
			break;
		}
	}

	if (mActionSubmenuTitleText != nullptr)
	{
		mActionSubmenuTitleText->SetText(FText::FromString(FString::Printf(
			TEXT("%s · 세부 행동"), *GetActionFamilyName(mExpandedActionFamily))));
		mActionSubmenuTitleText->SetVisibility(bCanShow
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const TArray<FSkillUI>* Skills = mCombatUIModel != nullptr ? &mCombatUIModel->GetSkillUIs() : nullptr;
	for (int32 SlotIndex = 0; SlotIndex < mActionSubmenuButtons.Num(); ++SlotIndex)
	{
		const bool bVisible = bCanShow && mActionSubmenuSkillIndices.IsValidIndex(SlotIndex);
		const int32 SkillIndex = bVisible ? mActionSubmenuSkillIndices[SlotIndex] : INDEX_NONE;
		const FSkillUI* Skill = Skills != nullptr && Skills->IsValidIndex(SkillIndex) ? &(*Skills)[SkillIndex] : nullptr;
		const bool bSelected = bVisible && mActionSubmenuModes.IsValidIndex(SlotIndex)
			&& mSelectedSubactionMode == mActionSubmenuModes[SlotIndex]
			&& mActiveActionFamily == mExpandedActionFamily;
		if (mActionSubmenuPanels.IsValidIndex(SlotIndex) && mActionSubmenuPanels[SlotIndex] != nullptr)
		{
			mActionSubmenuPanels[SlotIndex]->SetBrushColor(bSelected
				? FLinearColor(0.52f, 0.34f, 0.045f, 0.98f)
				: FLinearColor(0.025f, 0.055f, 0.070f, 0.96f));
			mActionSubmenuPanels[SlotIndex]->SetVisibility(bVisible
				? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (mActionSubmenuIcons.IsValidIndex(SlotIndex) && mActionSubmenuIcons[SlotIndex] != nullptr)
		{
			if (bVisible && Skill != nullptr && Skill->mIcon != nullptr)
			{
				mActionSubmenuIcons[SlotIndex]->SetBrushFromTexture(Skill->mIcon, false);
				mActionSubmenuIcons[SlotIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				mActionSubmenuIcons[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		if (mActionSubmenuTexts.IsValidIndex(SlotIndex) && mActionSubmenuTexts[SlotIndex] != nullptr)
		{
			mActionSubmenuTexts[SlotIndex]->SetColorAndOpacity(FSlateColor(bSelected
				? FLinearColor(1.0f, 0.88f, 0.34f, 1.0f)
				: FLinearColor(0.88f, 0.96f, 1.0f, 1.0f)));
			mActionSubmenuTexts[SlotIndex]->SetVisibility(bVisible
				? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (mActionSubmenuButtons[SlotIndex] != nullptr)
		{
			mActionSubmenuButtons[SlotIndex]->SetVisibility(bVisible
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UCombatTileMapHUDWidget::OpenActionFamily(int32 RailSlotIndex)
{
	if (mCombatUIModel == nullptr || RailSlotIndex < 0 || RailSlotIndex >= CombatSkillSlotCount)
	{
		return;
	}
	if (mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
	{
		mCombatUIModel->RequestCancel();
	}
	const bool bClose = mExpandedActionFamily == RailSlotIndex;
	mExpandedActionFamily = bClose ? INDEX_NONE : RailSlotIndex;
	mActiveActionFamily = RailSlotIndex;
	mSelectedSubactionMode = ECombatSubactionMode::None;
	mSelectedSubactionName = FText::GetEmpty();
	RefreshSkillRailWidgets();
	UpdateEnemyIntentTutorial();
}

void UCombatTileMapHUDWidget::HandleActionSubmenuClicked(int32 SubactionSlotIndex)
{
	if (mCombatUIModel == nullptr
		|| mActionSubmenuSkillIndices.IsValidIndex(SubactionSlotIndex) == false
		|| mActionSubmenuDesiredPowers.IsValidIndex(SubactionSlotIndex) == false
		|| mActionSubmenuModes.IsValidIndex(SubactionSlotIndex) == false
		|| mActionSubmenuNames.IsValidIndex(SubactionSlotIndex) == false)
	{
		return;
	}
	const int32 SkillIndex = mActionSubmenuSkillIndices[SubactionSlotIndex];
	const int32 DesiredPower = mActionSubmenuDesiredPowers[SubactionSlotIndex];
	const ECombatSubactionMode Mode = mActionSubmenuModes[SubactionSlotIndex];
	const FText DisplayName = mActionSubmenuNames[SubactionSlotIndex];
	if (mCombatUIModel->GetSelectedSkillIndex() != INDEX_NONE)
	{
		mCombatUIModel->RequestCancel();
	}
	mSelectedSubactionMode = Mode;
	mSelectedSubactionName = DisplayName;
	mSelectedSubactionDesiredPower = DesiredPower;
	if (Mode == ECombatSubactionMode::Whirlwind || Mode == ECombatSubactionMode::Shockwave)
	{
		mExpandedActionFamily = INDEX_NONE;
		RefreshSkillRailWidgets();
		mCombatUIModel->RequestWarriorAreaAction(
			Mode == ECombatSubactionMode::Whirlwind
				? ESRPGWarriorAreaActionType::Whirlwind
				: ESRPGWarriorAreaActionType::Shockwave);
		return;
	}
	if (SelectSkillWithActionPower(SkillIndex, DesiredPower) == false)
	{
		mSelectedSubactionMode = ECombatSubactionMode::None;
		mSelectedSubactionName = FText::GetEmpty();
		RefreshActionSubmenuWidgets();
		return;
	}
	mExpandedActionFamily = INDEX_NONE;
	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
	if (Mode == ECombatSubactionMode::Move || Mode == ECombatSubactionMode::Charge
		|| Mode == ECombatSubactionMode::Leap)
	{
		RefreshDirectMoveRangeHighlight();
	}
	UpdateEnemyIntentTutorial();
}

/** @details 개별 밀기/교환 데이터는 손아귀의 내부 실행 수단으로만 남기고 레일에는 노출하지 않는다. */
int32 UCombatTileMapHUDWidget::GetSkillDataIndexForRailSlot(int32 RailSlotIndex) const
{
	if (mCombatUIModel == nullptr)
	{
		return INDEX_NONE;
	}
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	int32 SkillDataIndex = INDEX_NONE;
	if (RailSlotIndex == 0)
	{
		SkillDataIndex = 0; // 기본 공격
	}
	else if (RailSlotIndex == CombatSkillSlotCount - 1)
	{
		SkillDataIndex = 1; // 이동
	}
	else
	{
		const bool FSkillUI::* WantedFlag = RailSlotIndex == 1
			? &FSkillUI::mIsPullSkill
			: &FSkillUI::mIsStaggerSkill;
		for (int32 CandidateIndex = 0; CandidateIndex < Skills.Num(); ++CandidateIndex)
		{
			if (Skills[CandidateIndex].*WantedFlag)
			{
				SkillDataIndex = CandidateIndex;
				break;
			}
		}
	}
	if (Skills.IsValidIndex(SkillDataIndex) == false || Skills[SkillDataIndex].mName.IsEmpty() == true)
	{
		return INDEX_NONE;
	}
	return SkillDataIndex;
}

/** @details 시안 라벨 폴백 없이 뷰모델의 보유 스킬 이름만 반환한다(없으면 빈 텍스트). */
FText UCombatTileMapHUDWidget::GetOwnedSkillLabel(int32 SkillIndex) const
{
	if (mCombatUIModel != nullptr)
	{
		const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
		if (Skills.IsValidIndex(SkillIndex))
		{
			return Skills[SkillIndex].mName;
		}
	}
	return FText::GetEmpty();
}

void UCombatTileMapHUDWidget::EnsureSkillInputButtons()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr || mSkillInputButtons.Num() == CombatSkillSlotCount)
	{
		return;
	}

	for (UIndexedButtonWidget* SkillInputButton : mSkillInputButtons)
	{
		if (SkillInputButton != nullptr)
		{
			SkillInputButton->RemoveFromParent();
		}
	}
	mSkillInputButtons.Reset();

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UIndexedButtonWidget* SkillInputButton = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(),
			FName(*FString::Printf(TEXT("SkillInputButton_%d"), SkillIndex))
		);
		if (SkillInputButton == nullptr)
		{
			continue;
		}

		SkillInputButton->SetBackgroundColor(GetTransparentInputButtonColor());
		SkillInputButton->SetVisibility(ESlateVisibility::Collapsed);
		SkillInputButton->SetButtonIndex(SkillIndex);
		SkillInputButton->OnIndexedPressed.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillButtonPressed);
		SkillInputButton->OnReleased.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillButtonReleased);
		GetSkinTargetCanvas()->AddChildToCanvas(SkillInputButton);   // 렌더(패널)와 같은 좌표계 — 16:9 외 화면에서 탭 영역 어긋남 방지
		mSkillInputButtons.Add(SkillInputButton);
	}
}

int32 UCombatTileMapHUDWidget::FindSkillRailIndexAtScreenPosition(const FVector2D& ScreenPosition) const
{
	// 렌더된 입력 버튼 자신의 geometry로 판정한다 — 좌표계(엣지 피닝/레터박스/레거시)와 무관하게
	// 렌더와 히트테스트가 항상 일치한다. 첫 페인트 전(크기 0)이나 비표시는 건너뛴다.
	for (int32 SkillIndex = 0; SkillIndex < mSkillInputButtons.Num(); ++SkillIndex)
	{
		const UWidget* InputButton = mSkillInputButtons[SkillIndex];
		if (InputButton == nullptr || InputButton->IsVisible() == false)
		{
			continue;
		}
		const FGeometry& ButtonGeometry = InputButton->GetCachedGeometry();
		const FVector2D LocalSize = ButtonGeometry.GetLocalSize();
		if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
		{
			continue;
		}
		if (ButtonGeometry.IsUnderLocation(ScreenPosition))
		{
			return SkillIndex;
		}
	}
	return INDEX_NONE;
}
