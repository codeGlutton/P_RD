#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/DiceCapturePreviewUtils.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::ApplyDiceCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize) const
{
	RDDiceCapturePreview::ApplyCaptureBrush(DiceImage, DiceActor, BrushSize);
}

ACombatDiceCaptureActor* UCombatTileMapHUDWidget::SpawnDiceCaptureActor(int32 GroupIndex, int32 DiceIndex, int32 RenderTargetSize)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	return RDDiceCapturePreview::SpawnCaptureActor(World, this, RDDiceCapturePreview::GetCombatPreviewLocation(GroupIndex, DiceIndex), RenderTargetSize);
}

void UCombatTileMapHUDWidget::DestroyDiceCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors) const
{
	RDDiceCapturePreview::DestroyCaptureActors(DiceActors);
}

void UCombatTileMapHUDWidget::EnsureDicePreviewActors()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->RemoveFromParent();
		}
	}
	mDiceRollImages.Reset();
	DestroyDiceCaptureActors(mDicePreviewActors);
	mIntroDiceSettleStartRotations.Reset();

	const int32 DiceCount = FMath::Min(mDiceViews.Num(), MaxCombatDiceCardCount);
	if (DiceCount <= 0)
	{
		ApplyRuntimeWidgetLayout();
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		UImage* DiceImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("DiceRollImage_%d"), DiceIndex))
		);
		if (DiceImage == nullptr)
		{
			continue;
		}

		DiceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootCanvas->AddChildToCanvas(DiceImage);

		ACombatDiceCaptureActor* DicePreviewActor = SpawnDiceCaptureActor(0, DiceIndex, RDDiceCapturePreview::GetDefaultRenderTargetSize());
		if (DicePreviewActor != nullptr)
		{
			DicePreviewActor->SetActorScale3D(FVector(RDDiceCapturePreview::GetCombatPreviewDiceScale()));
			DicePreviewActor->SetDiceRotation(GetReadableDiceIdleRotation(DiceIndex));
			DicePreviewActor->SetBackdropVisible(false);
			DicePreviewActor->CaptureDice();
			ApplyDiceCaptureBrush(DiceImage, DicePreviewActor, RDDiceCapturePreview::GetDefaultBrushSize());
		}

		mDiceRollImages.Add(DiceImage);
		mDicePreviewActors.Add(DicePreviewActor);
		mIntroDiceSettleStartRotations.Add(GetReadableDiceIdleRotation(DiceIndex));
	}

	ApplyRuntimeWidgetLayout();
}

void UCombatTileMapHUDWidget::RefreshDicePreviewActors()
{
	for (int32 DiceIndex = 0; DiceIndex < mDiceRollImages.Num(); ++DiceIndex)
	{
		UImage* DiceImage = mDiceRollImages[DiceIndex];
		if (DiceImage == nullptr)
		{
			continue;
		}

		if (mDicePreviewActors.IsValidIndex(DiceIndex) == false)
		{
			mDicePreviewActors.SetNum(DiceIndex + 1);
		}

		if (IsValid(mDicePreviewActors[DiceIndex]) == false)
		{
			mDicePreviewActors[DiceIndex] = SpawnDiceCaptureActor(0, DiceIndex, RDDiceCapturePreview::GetDefaultRenderTargetSize());
			if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
			{
				const FRotator StartRotation = mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex)
					? mIntroDiceSettleStartRotations[DiceIndex]
					: GetReadableDiceIdleRotation(DiceIndex);
				const FLinearColor DiceColor = mDiceViews.IsValidIndex(DiceIndex)
					? RDUIDice::GetDiceRarityColor(mDiceViews[DiceIndex].mRarityType)
					: RDUIDice::GetDiceRarityColor(ERarityType::Common);

				DicePreviewActor->SetActorScale3D(FVector(RDDiceCapturePreview::GetCombatPreviewDiceScale()));
				DicePreviewActor->SetDiceRotation(StartRotation);
				DicePreviewActor->SetDiceColor(DiceColor);
				DicePreviewActor->SetBackdropVisible(false);
				DicePreviewActor->CaptureDice();
			}
		}

		if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
		{
			ApplyDiceCaptureBrush(DiceImage, DicePreviewActor, RDDiceCapturePreview::GetDefaultBrushSize());
		}
	}
}
