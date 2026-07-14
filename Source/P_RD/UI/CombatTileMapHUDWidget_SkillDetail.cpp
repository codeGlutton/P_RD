#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleSkillDetailDismissButtonClicked()
{
	HideSkillDetail();
}

void UCombatTileMapHUDWidget::SetDetailOverlayVisible(bool bVisible) const
{
	// 오버레이 WBP는 보이되 입력을 안 먹게(HitTestInvisible), 그 위 투명 버튼만 입력을 받아 "아무 데나 탭 = 닫기".
	if (mDetailOverlay != nullptr)
	{
		mDetailOverlay->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mSkillDetailDismissButton != nullptr)
	{
		mSkillDetailDismissButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 뒤 풀뷰포트 회색 딤(주사위 배경과 동일) — HP바/스킬레일 등 뒤 HUD를 덮는다.
	if (mSkillDetailBackdropPanels.IsValidIndex(0) && mSkillDetailBackdropPanels[0] != nullptr)
	{
		mSkillDetailBackdropPanels[0]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCombatTileMapHUDWidget::ShowSkillDetail(int32 SkillIndex)
{
	if (mCombatUIModel == nullptr || SkillIndex == INDEX_NONE)
	{
		return;
	}

	mCombatUIModel->RequestLongPressSkill(SkillIndex);
	const FSkillDetailUI& Detail = mCombatUIModel->GetSkillDetail();

	if (mDetailIconImage != nullptr)
	{
		mDetailIconImage->SetBrushFromTexture(Detail.mIcon, false);
		// WBP 기본 틴트(위젯 색상/브러시 틴트) 제거 → 아이콘이 원색으로(teal 덮임 방지).
		mDetailIconImage->SetColorAndOpacity(FLinearColor::White);
		mDetailIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
	}
	if (mDetailTitleText != nullptr)
	{
		mDetailTitleText->SetText(Detail.mName.IsEmpty() ? GetOwnedSkillLabel(SkillIndex) : Detail.mName);
	}
	if (mDetailSubtitleText != nullptr)
	{
		mDetailSubtitleText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "SkillDetailMeta", "SKILL · 주사위 {0}"),
			FText::AsNumber(Detail.mDiceCost)));
	}
	if (mDetailBodyText != nullptr) { mDetailBodyText->SetText(Detail.mDescription); }

	SetDetailOverlayVisible(true);
	ApplyRuntimeWidgetLayout();
}

void UCombatTileMapHUDWidget::ShowUnitDetail(int32 UnitId)
{
	if (mCombatUIModel == nullptr || UnitId == INDEX_NONE)
	{
		return;
	}

	mCombatUIModel->RequestLongPressUnit(UnitId);
	ShowCachedUnitDetail();
}

void UCombatTileMapHUDWidget::ShowCachedUnitDetail()
{
	if (mCombatUIModel == nullptr)
	{
		return;
	}

	const FUnitDetailUI& Detail = mCombatUIModel->GetUnitDetail();
	if (Detail.mUnitId == INDEX_NONE)
	{
		return;
	}

	if (mDetailIconImage != nullptr)
	{
		mDetailIconImage->SetBrushFromTexture(Detail.mPortrait, false);
		mDetailIconImage->SetColorAndOpacity(FLinearColor::White);
		mDetailIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
	}
	if (mDetailTitleText != nullptr) { mDetailTitleText->SetText(Detail.mName); }
	if (mDetailSubtitleText != nullptr)
	{
		mDetailSubtitleText->SetText(FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "UnitDetailMeta", "UNIT · Lv.{0}"),
			FText::AsNumber(Detail.mLevel)));
	}
	if (mDetailBodyText != nullptr)
	{
		FString Joined;
		for (const FText& Passive : Detail.mPassiveDescriptions)
		{
			if (Joined.IsEmpty() == false) { Joined += TEXT("\n"); }
			Joined += Passive.ToString();
		}
		mDetailBodyText->SetText(FText::FromString(Joined));
	}

	SetDetailOverlayVisible(true);
	ApplyRuntimeWidgetLayout();
}

void UCombatTileMapHUDWidget::ShowEquipmentDetail(int32 SlotIndex)
{
	if (mCombatUIModel == nullptr || SlotIndex == INDEX_NONE)
	{
		return;
	}

	// 빈 슬롯(장착 안 됨)은 상세를 열지 않는다 — 명령/상세 push를 아예 트리거하지 않아 크래시를 막는다.
	const FEquipmentUI* Equip = nullptr;
	for (const FEquipmentUI& Candidate : mCombatUIModel->GetEquipmentUIs())
	{
		if (Candidate.mSlotIndex == SlotIndex) { Equip = &Candidate; break; }
	}
	if (Equip == nullptr || Equip->mIsEquipped == false) { return; }

	mCombatUIModel->RequestLongPressEquip(SlotIndex);

	if (mDetailIconImage != nullptr)
	{
		mDetailIconImage->SetBrushFromTexture(Equip->mIcon, false);
		mDetailIconImage->SetColorAndOpacity(FLinearColor::White);
		mDetailIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
	}
	if (mDetailTitleText != nullptr) { mDetailTitleText->SetText(Equip->mName); }
	if (mDetailSubtitleText != nullptr)
	{
		mDetailSubtitleText->SetText(Equip->mIsEquipped
			? NSLOCTEXT("CombatTileMapHUDWidget", "EquipDetailEquipped", "EQUIPMENT · 장착 중")
			: NSLOCTEXT("CombatTileMapHUDWidget", "EquipDetail", "EQUIPMENT"));
	}
	if (mDetailBodyText != nullptr) { mDetailBodyText->SetText(FText::GetEmpty()); }

	SetDetailOverlayVisible(true);
	ApplyRuntimeWidgetLayout();
}

void UCombatTileMapHUDWidget::HideSkillDetail() const
{
	SetDetailOverlayVisible(false);
	ApplyRuntimeWidgetLayout();
}

bool UCombatTileMapHUDWidget::IsSkillDetailVisible() const
{
	return mDetailOverlay != nullptr
		&& mDetailOverlay->GetVisibility() != ESlateVisibility::Collapsed
		&& mDetailOverlay->GetVisibility() != ESlateVisibility::Hidden;
}

bool UCombatTileMapHUDWidget::IsScreenPositionInSkillDetailPanel(const FVector2D& ScreenPosition) const
{
	if (IsSkillDetailVisible() == false || mDetailOverlay == nullptr)
	{
		return false;
	}

	const FGeometry DetailGeometry = mDetailOverlay->GetCachedGeometry();
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
