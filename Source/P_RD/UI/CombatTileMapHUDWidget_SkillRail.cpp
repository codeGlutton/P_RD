#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

namespace
{
	/** @brief 뷰모델이 연결돼 있으면 스킬 이름을 거기서, 아니면 정적 시안 라벨을 쓴다. */
	FText ResolveSkillRailLabel(const UCombatUIModel* ViewModel, int32 SkillIndex)
	{
		if (ViewModel != nullptr)
		{
			const TArray<FSkillUI>& Skills = ViewModel->GetSkillUIs();
			if (Skills.IsValidIndex(SkillIndex) == false || Skills[SkillIndex].mIsUsable == false)
			{
				return FText::GetEmpty();
			}
			if (Skills.IsValidIndex(SkillIndex) && Skills[SkillIndex].mName.IsEmpty() == false)
			{
				return Skills[SkillIndex].mName;
			}
		}
		return RDCombatHUD::GetCombatSkillRailLabel(SkillIndex);
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
	mSkillRailPanels.Reset();
	mSkillRailTexts.Reset();

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UBorder* SkillRailPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailPanel_%d"), SkillIndex))
		);
		UTextBlock* SkillRailText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailText_%d"), SkillIndex))
		);
		if (SkillRailPanel == nullptr || SkillRailText == nullptr)
		{
			continue;
		}

		SkillRailPanel->SetPadding(GetCombatSkillRailPadding());
		SkillRailText->SetJustification(ETextJustify::Center);
		if (IsDesignerSkinActive())
		{
			// 디자이너 스킨: 레일 배경/라벨을 숨겨 WBP의 concept 스킬 아이콘이 그대로 보이게 한다(선택 강조만 Refresh에서 입힘).
			SkillRailPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			SkillRailText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(false));
			SkillRailText->SetColorAndOpacity(FSlateColor(GetCombatSkillRailTextColor(false)));
			SkillRailText->SetText(ResolveSkillRailLabel(mCombatUIModel, SkillIndex));
		}
		SkillRailPanel->AddChild(SkillRailText);
		RootCanvas->AddChildToCanvas(SkillRailPanel);

		mSkillRailPanels.Add(SkillRailPanel);
		mSkillRailTexts.Add(SkillRailText);
	}

	RefreshSkillRailWidgets();
}

void UCombatTileMapHUDWidget::RefreshSkillRailWidgets()
{
	for (int32 SkillIndex = 0; SkillIndex < mSkillRailPanels.Num(); ++SkillIndex)
	{
		const bool available = IsSkillSlotAvailable(SkillIndex);
		const bool selected = available && SkillIndex == mSelectedSkillIndex;
		if (UBorder* SkillRailPanel = mSkillRailPanels[SkillIndex])
		{
			if (IsDesignerSkinActive())
			{
				// 스킨 모드: 비선택은 아이콘만 보이고, 선택 슬롯은 아트 위에 확실히 보이는 금색 틴트를 얹는다.
				SkillRailPanel->SetBrushColor(selected ? FLinearColor(1.0f, 0.84f, 0.18f, 0.68f) : FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			}
			else
			{
				SkillRailPanel->SetBrushColor(available ? GetCombatSkillRailBrushColor(selected) : FLinearColor(0.04f, 0.06f, 0.07f, 0.45f));
			}
			SkillRailPanel->SetRenderScale(available ? GetCombatSkillRailScale(selected) : FVector2D(1.0f, 1.0f));
		}

		if (mSkillRailTexts.IsValidIndex(SkillIndex))
		{
			if (UTextBlock* SkillRailText = mSkillRailTexts[SkillIndex])
			{
				if (IsDesignerSkinActive() == false)
				{
					SkillRailText->SetText(ResolveSkillRailLabel(mCombatUIModel, SkillIndex));
				}
				SkillRailText->SetColorAndOpacity(FSlateColor(available ? GetCombatSkillRailTextColor(selected) : FLinearColor(0.48f, 0.55f, 0.55f, 0.70f)));
			}
		}

		if (mSkillInputButtons.IsValidIndex(SkillIndex))
		{
			if (UIndexedButtonWidget* SkillInputButton = mSkillInputButtons[SkillIndex])
			{
				SkillInputButton->SetIsEnabled(available);
			}
		}
	}
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
		RootCanvas->AddChildToCanvas(SkillInputButton);
		mSkillInputButtons.Add(SkillInputButton);
	}
}

int32 UCombatTileMapHUDWidget::FindSkillRailIndexAtScreenPosition(const FVector2D& ScreenPosition) const
{
	// 렌더는 스킬레일을 DesignCanvas(1920x1080, ScaleBox 레터박스) 좌표에 그린다. 히트테스트도 같은
	// DesignCanvas 지오메트리로 정규화해야 탭 영역이 렌더와 일치한다. (루트 GetCachedGeometry는 풀뷰포트라
	// 화면비 차이만큼 어긋난다.) 스킨 비활성 fallback은 루트 지오메트리.
	const UWidget* GeometrySource = (IsDesignerSkinActive() && DesignCanvas.Get() != nullptr)
		? StaticCast<const UWidget*>(DesignCanvas.Get())
		: StaticCast<const UWidget*>(this);
	const FGeometry CachedGeometry = GeometrySource->GetCachedGeometry();
	const FVector2D LocalPosition = CachedGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D LocalSize = CachedGeometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return INDEX_NONE;
	}

	const float NormalizedX = LocalPosition.X / LocalSize.X;
	const float NormalizedY = LocalPosition.Y / LocalSize.Y;
	// 렌더와 같은 영역 계산(GetSkillRailItemRect)을 써서 WBP 스킨 좌표에서도 탭 영역이 일치하게 한다.
	const FAnchors RailGroup = GetSkillRailGroupRect();
	if (NormalizedX < RailGroup.Minimum.X || NormalizedX > RailGroup.Maximum.X)
	{
		return INDEX_NONE;
	}

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		const FAnchors ItemRect = GetSkillRailItemRect(SkillIndex, CombatSkillSlotCount);
		if (NormalizedY >= ItemRect.Minimum.Y && NormalizedY <= ItemRect.Maximum.Y)
		{
			return SkillIndex;
		}
	}

	return INDEX_NONE;
}
