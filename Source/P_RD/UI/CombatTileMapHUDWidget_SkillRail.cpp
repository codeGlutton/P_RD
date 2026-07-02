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

		RootCanvas->AddChildToCanvas(SkillRailPanel);
		RootCanvas->AddChildToCanvas(SkillRailIcon);
		RootCanvas->AddChildToCanvas(SkillRailText);

		mSkillRailPanels.Add(SkillRailPanel);
		mSkillRailIcons.Add(SkillRailIcon);
		mSkillRailTexts.Add(SkillRailText);
	}

	RefreshSkillRailWidgets();
}

/** @details 슬롯 3상태 - [보유+사용가능] 아이콘+라벨 / [보유+사용불가] 흐림 / [빈 슬롯] 어두운 커버.
 * 빈 슬롯 커버가 WBP에 박힌 concept 고정 아트를 가려 "실제 보유한 스킬만" 보이게 한다. 데이터 소스는 FSkillUI 단일. */
void UCombatTileMapHUDWidget::RefreshSkillRailWidgets()
{
	const TArray<FSkillUI>* Skills = mCombatUIModel != nullptr ? &mCombatUIModel->GetSkillUIs() : nullptr;

	for (int32 SkillIndex = 0; SkillIndex < mSkillRailPanels.Num(); ++SkillIndex)
	{
		const FSkillUI* Skill = (Skills != nullptr && Skills->IsValidIndex(SkillIndex)) ? &(*Skills)[SkillIndex] : nullptr;
		const bool bOwned = Skill != nullptr && Skill->mName.IsEmpty() == false;
		const bool bUsable = bOwned && Skill->mIsUsable;
		const bool bSelected = bOwned && SkillIndex == mSelectedSkillIndex;
		const float DimOpacity = bUsable ? 1.0f : 0.45f;

		if (UBorder* SkillRailPanel = mSkillRailPanels[SkillIndex])
		{
			if (bOwned == false)
			{
				// 빈 슬롯: WBP 고정 아트 위를 어둡게 덮어 "없는 스킬"이 보이지 않게 한다.
				SkillRailPanel->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.92f));
			}
			else if (IsDesignerSkinActive())
			{
				// 스킨 모드: 비선택은 투명(아이콘만), 선택은 옅은 금색 틴트로만 강조.
				SkillRailPanel->SetBrushColor(bSelected ? FLinearColor(1.0f, 0.95f, 0.55f, 0.30f) : FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			}
			else
			{
				SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(bSelected));
			}
			SkillRailPanel->SetRenderScale(GetCombatSkillRailScale(bSelected));
		}

		if (mSkillRailIcons.IsValidIndex(SkillIndex))
		{
			if (UImage* SkillRailIcon = mSkillRailIcons[SkillIndex])
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

		if (mSkillRailTexts.IsValidIndex(SkillIndex))
		{
			if (UTextBlock* SkillRailText = mSkillRailTexts[SkillIndex])
			{
				if (bOwned)
				{
					SkillRailText->SetText(FText::Format(
						NSLOCTEXT("CombatTileMapHUDWidget", "SkillRailSlotFormat", "{0}\n{1} DICE"),
						Skill->mName, FText::AsNumber(Skill->mDiceCost)));
					SkillRailText->SetColorAndOpacity(FSlateColor(GetCombatSkillRailTextColor(bSelected)));
					SkillRailText->SetRenderOpacity(DimOpacity);
					SkillRailText->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					SkillRailText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
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
