#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Engine/Texture2D.h"
#include "GameMode/CombatGameMode.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/UIRuntimeLayout.h"

namespace
{
	constexpr int32 SkillDetailZOrder = 260;

	void SetDetailTextSize(UTextBlock* TextBlock, int32 Size)
	{
		if (TextBlock == nullptr)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		TextBlock->SetFont(Font);
	}

	FText BuildFallbackSkillDescription(const FSkillUI& Skill)
	{
		return FText::Format(
			NSLOCTEXT("CombatTileMapHUDWidget", "SkillDetailFallbackDescription", "Dice {0}\nAim {1} + {2}/dice\nArea {3} + {4}/dice"),
			FText::AsNumber(Skill.mDiceCost),
			FText::AsNumber(FMath::RoundToInt(Skill.mTargeting.mSelectRange)),
			FText::AsNumber(Skill.mTargeting.mSelectRangeRatio),
			FText::AsNumber(FMath::RoundToInt(Skill.mTargeting.mHitRange)),
			FText::AsNumber(Skill.mTargeting.mHitRangeRatio)
		);
	}
}

void UCombatTileMapHUDWidget::ShowSkillDetailPanel(int32 SkillIndex)
{
	if (WidgetTree == nullptr || mCombatUIModel == nullptr)
	{
		return;
	}

	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillIndex) == false || Skills[SkillIndex].mIsUsable == false)
	{
		return;
	}

	const FSkillEntry* SkillEntry = nullptr;
	const UStaticSkillData* StaticSkillData = nullptr;
	if (ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		SkillEntry = CombatGameMode->GetSkillDetail(SkillIndex);
		if (SkillEntry != nullptr && SkillEntry->IsValid())
		{
			StaticSkillData = SkillEntry->mData.Get();
		}
	}

	UCanvasPanel* TargetCanvas = IsDesignerSkinActive() && DesignCanvas != nullptr ? DesignCanvas.Get() : RootCanvas.Get();
	if (TargetCanvas == nullptr)
	{
		return;
	}

	if (mSkillDetailPanel == nullptr)
	{
		mSkillDetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SkillDetailPanel"));
		UVerticalBox* DetailStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SkillDetailStack"));
		mSkillDetailIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SkillDetailIcon"));
		mSkillDetailNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailName"));
		mSkillDetailDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailDescription"));
		mSkillDetailCloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillDetailCloseButton"));
		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDetailCloseText"));

		if (mSkillDetailPanel == nullptr || DetailStack == nullptr || mSkillDetailNameText == nullptr || mSkillDetailDescriptionText == nullptr || mSkillDetailCloseButton == nullptr || CloseText == nullptr)
		{
			return;
		}

		mSkillDetailPanel->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.070f, 0.95f));
		mSkillDetailPanel->SetPadding(FMargin(18.0f, 14.0f));

		if (mSkillDetailIconImage != nullptr)
		{
			mSkillDetailIconImage->SetDesiredSizeOverride(FVector2D(72.0f, 72.0f));
			DetailStack->AddChildToVerticalBox(mSkillDetailIconImage);
		}

		mSkillDetailNameText->SetJustification(ETextJustify::Center);
		mSkillDetailNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.58f, 1.0f)));
		SetDetailTextSize(mSkillDetailNameText, 24);
		DetailStack->AddChildToVerticalBox(mSkillDetailNameText);

		mSkillDetailDescriptionText->SetAutoWrapText(true);
		mSkillDetailDescriptionText->SetJustification(ETextJustify::Left);
		mSkillDetailDescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.98f, 0.96f, 1.0f)));
		SetDetailTextSize(mSkillDetailDescriptionText, 16);
		DetailStack->AddChildToVerticalBox(mSkillDetailDescriptionText);

		CloseText->SetText(NSLOCTEXT("CombatTileMapHUDWidget", "SkillDetailClose", "CLOSE"));
		CloseText->SetJustification(ETextJustify::Center);
		CloseText->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.07f, 0.07f, 1.0f)));
		SetDetailTextSize(CloseText, 15);

		mSkillDetailCloseButton->SetBackgroundColor(FLinearColor(0.94f, 0.84f, 0.56f, 0.96f));
		mSkillDetailCloseButton->AddChild(CloseText);
		mSkillDetailCloseButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDetailCloseButtonClicked);
		DetailStack->AddChildToVerticalBox(mSkillDetailCloseButton);

		mSkillDetailPanel->AddChild(DetailStack);
		TargetCanvas->AddChildToCanvas(mSkillDetailPanel);
	}
	else if (mSkillDetailPanel->GetParent() == nullptr)
	{
		TargetCanvas->AddChildToCanvas(mSkillDetailPanel);
	}

	const FSkillUI& Skill = Skills[SkillIndex];
	if (mSkillDetailNameText != nullptr)
	{
		const FText DetailName = StaticSkillData != nullptr && StaticSkillData->mName.IsEmpty() == false
			? StaticSkillData->mName
			: Skill.mName;
		mSkillDetailNameText->SetText(DetailName.IsEmpty()
			? FText::Format(NSLOCTEXT("CombatTileMapHUDWidget", "SkillDetailFallbackName", "Skill {0}"), FText::AsNumber(SkillIndex + 1))
			: DetailName);
	}
	if (mSkillDetailDescriptionText != nullptr)
	{
		const FText DetailDescription = StaticSkillData != nullptr && StaticSkillData->mDescription.IsEmpty() == false
			? StaticSkillData->mDescription
			: Skill.mDescription;
		mSkillDetailDescriptionText->SetText(DetailDescription.IsEmpty() ? BuildFallbackSkillDescription(Skill) : DetailDescription);
	}
	if (mSkillDetailIconImage != nullptr)
	{
		UTexture2D* DetailIcon = Skill.mIcon.Get();
		if (StaticSkillData != nullptr && StaticSkillData->mIcon.IsNull() == false)
		{
			DetailIcon = StaticSkillData->mIcon.LoadSynchronous();
		}

		if (DetailIcon != nullptr)
		{
			mSkillDetailIconImage->SetBrushFromTexture(DetailIcon, true);
			mSkillDetailIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			mSkillDetailIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	mSkillDetailPanel->SetVisibility(ESlateVisibility::Visible);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailPanel, GroupRect(TEXT("HUD_SkillDetail"), FAnchors(0.315f, 0.215f, 0.685f, 0.585f)), SkillDetailZOrder);
}

void UCombatTileMapHUDWidget::HideSkillDetailPanel()
{
	if (mSkillDetailPanel != nullptr)
	{
		mSkillDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCombatTileMapHUDWidget::HandleSkillDetailCloseButtonClicked()
{
	HideSkillDetailPanel();
}
