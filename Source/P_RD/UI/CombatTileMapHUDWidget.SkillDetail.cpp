#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked()
{
	HideSkillDetail();
}

void UCombatTileMapHUDWidget::ShowSkillDetail(int32 SkillIndex)
{
	if (mSkillDetailPanel == nullptr || mSkillDetailText == nullptr || SkillIndex == INDEX_NONE)
	{
		return;
	}

	// 뷰모델 연결 시 상세 요청은 의도로 보내고, 상세 내용도 뷰모델이 준 값으로 그린다.
	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->RequestLongPressSkill(SkillIndex);

		const FSkillDetailView& Detail = mCombatViewModel->GetSkillDetail();
		if (Detail.mSkillIndex == SkillIndex && (Detail.mName.IsEmpty() == false || Detail.mDescription.IsEmpty() == false))
		{
			mSkillDetailText->SetText(FText::Format(
				NSLOCTEXT("CombatTileMapHUDWidget", "SkillDetailFromViewModelFormat", "{0}\n{1}"),
				Detail.mName, Detail.mDescription
			));
			if (mSkillDetailDismissButton != nullptr)
			{
				mSkillDetailDismissButton->SetVisibility(ESlateVisibility::Visible);
			}
			mSkillDetailPanel->SetVisibility(ESlateVisibility::Visible);
			return;
		}
	}

	mSkillDetailText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "SkillDetailTextFormat", "{0}\nSkill detail preview\nAPI connection pending"),
		GetCombatSkillLabel(SkillIndex)
	));
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->SetVisibility(ESlateVisibility::Visible);
	}
	mSkillDetailPanel->SetVisibility(ESlateVisibility::Visible);
}

void UCombatTileMapHUDWidget::HideSkillDetail() const
{
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mSkillDetailPanel != nullptr)
	{
		mSkillDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UCombatTileMapHUDWidget::IsSkillDetailVisible() const
{
	return mSkillDetailPanel != nullptr && mSkillDetailPanel->GetVisibility() != ESlateVisibility::Collapsed && mSkillDetailPanel->GetVisibility() != ESlateVisibility::Hidden;
}

bool UCombatTileMapHUDWidget::IsScreenPositionInSkillDetailPanel(const FVector2D& ScreenPosition) const
{
	if (IsSkillDetailVisible() == false)
	{
		return false;
	}

	const FGeometry DetailGeometry = mSkillDetailPanel->GetCachedGeometry();
	const FVector2D LocalPosition = DetailGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D LocalSize = DetailGeometry.GetLocalSize();
	return LocalPosition.X >= 0.0f && LocalPosition.Y >= 0.0f && LocalPosition.X <= LocalSize.X && LocalPosition.Y <= LocalSize.Y;
}

bool UCombatTileMapHUDWidget::HideSkillDetailIfClickedOutside(const FVector2D& ScreenPosition)
{
	if (IsSkillDetailVisible() == false)
	{
		return false;
	}

	if (IsScreenPositionInSkillDetailPanel(ScreenPosition) == true)
	{
		return true;
	}

	HideSkillDetail();
	return true;
}
