#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "UI/Combat/CombatUIModel.h"

void UCombatTileMapHUDWidget::RebuildUnitHpBars()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	const int32 DesiredCount = mCombatUIModel != nullptr ? mCombatUIModel->GetUnitUIs().Num() : 0;

	// 유닛 수보다 많은 바는 제거.
	for (int32 BarIndex = mUnitHpBars.Num() - 1; BarIndex >= DesiredCount; --BarIndex)
	{
		if (mUnitHpBars[BarIndex] != nullptr)
		{
			mUnitHpBars[BarIndex]->RemoveFromParent();
		}
		mUnitHpBars.RemoveAt(BarIndex);
	}

	// 모자란 바는 새로 만든다.
	for (int32 BarIndex = mUnitHpBars.Num(); BarIndex < DesiredCount; ++BarIndex)
	{
		UProgressBar* HpBar = WidgetTree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(),
			FName(*FString::Printf(TEXT("UnitHpBar_%d"), BarIndex)));
		if (HpBar == nullptr)
		{
			continue;
		}

		HpBar->SetVisibility(ESlateVisibility::Collapsed);
		RootCanvas->AddChildToCanvas(HpBar);
		if (UCanvasPanelSlot* HpBarSlot = Cast<UCanvasPanelSlot>(HpBar->Slot))
		{
			HpBarSlot->SetAutoSize(false);
			HpBarSlot->SetAlignment(FVector2D(0.5f, 1.0f));   // 가로 중앙, 세로 아래 기준(유닛 머리 위에 놓기).
			HpBarSlot->SetSize(FVector2D(90.0f, 12.0f));
			HpBarSlot->SetZOrder(210);
		}
		mUnitHpBars.Add(HpBar);
	}

	UpdateUnitHpBars();
}

void UCombatTileMapHUDWidget::UpdateUnitHpBars()
{
	if (mCombatUIModel == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return;
	}

	const TArray<FUnitUI>& Units = mCombatUIModel->GetUnitUIs();
	for (int32 BarIndex = 0; BarIndex < mUnitHpBars.Num(); ++BarIndex)
	{
		UProgressBar* HpBar = mUnitHpBars[BarIndex];
		if (HpBar == nullptr || Units.IsValidIndex(BarIndex) == false)
		{
			continue;
		}
		const FUnitUI& Unit = Units[BarIndex];

		const float Percent = Unit.mMaxHP > 0.0f ? FMath::Clamp(Unit.mHP / Unit.mMaxHP, 0.0f, 1.0f) : 0.0f;
		HpBar->SetPercent(Percent);
		HpBar->SetFillColorAndOpacity(Unit.mIsPlayer
			? FLinearColor(0.35f, 0.85f, 0.40f, 1.0f)
			: FLinearColor(0.90f, 0.30f, 0.28f, 1.0f));

		// 유닛 월드 위치를 화면(위젯) 좌표로 투영. 화면 밖이면 숨긴다.
		FVector2D ScreenPosition;
		const bool bOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, Unit.mWorldLocation, ScreenPosition, false);
		if (bOnScreen == false)
		{
			HpBar->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		if (UCanvasPanelSlot* HpBarSlot = Cast<UCanvasPanelSlot>(HpBar->Slot))
		{
			HpBarSlot->SetPosition(ScreenPosition + FVector2D(0.0f, -70.0f));   // 유닛 머리 위로 띄운다.
		}
		HpBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
