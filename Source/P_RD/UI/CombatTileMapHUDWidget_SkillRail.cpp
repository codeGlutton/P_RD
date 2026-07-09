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
		SkillRailText->SetJustification(ETextJustify::Center);
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

/** @details 레일 고정 배치 - 맨 위 칸=기본 공격(평타), 맨 아래 칸=STEP, 중간 4칸=추가 스킬(시각 슬롯 매핑).
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
		const bool bSelected = bOwned && SkillDataIndex == mSelectedSkillIndex;
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
				if (IsDesignerSkinActive())
				{
					// 스킨 모드: 비선택은 투명(프레임+아이콘만), 선택은 옅은 금색 틴트로만 강조.
					SkillRailPanel->SetBrushColor(bSelected ? FLinearColor(1.0f, 0.95f, 0.55f, 0.30f) : FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
				}
				else
				{
					SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(bSelected));
				}
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
				// 레일에는 아이콘만 보여준다 - 이름/코스트는 롱프레스 상세 카드가 담당한다.
				SkillRailText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

/** @details 시각 슬롯 규칙은 헤더 주석 참고. 반환 전에 보유 여부(FSkillUI 이름)까지 검증해 미보유면 INDEX_NONE. */
int32 UCombatTileMapHUDWidget::GetSkillDataIndexForRailSlot(int32 RailSlotIndex) const
{
	int32 SkillDataIndex = INDEX_NONE;
	if (RailSlotIndex == 0)
	{
		SkillDataIndex = 0;                                // 맨 위 고정: 기본 공격(평타)
	}
	else if (RailSlotIndex == CombatSkillSlotCount - 1)
	{
		SkillDataIndex = 1;                                // 맨 아래 고정: 기본 이동(STEP)
	}
	else if (RailSlotIndex >= 1 && RailSlotIndex < CombatSkillSlotCount - 1)
	{
		SkillDataIndex = RailSlotIndex + 1;                // 중간 4칸: 추가 스킬(데이터 2..5)을 위에서부터
	}

	if (mCombatUIModel == nullptr)
	{
		return INDEX_NONE;
	}
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
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
		SkillInputButton->SetVisibility(ESlateVisibility::Visible);
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
