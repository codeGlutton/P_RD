#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "UI/CombatDiceCaptureActor.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::RefreshDiceViewsFromRunData()
{
	mDiceViews.Reset();

	const UWorld* World = GetWorld();
	const ARDGameModeBase* GameMode = World != nullptr ? World->GetAuthGameMode<ARDGameModeBase>() : nullptr;
	const URunPersistData* RunPersistData = GameMode != nullptr ? GameMode->GetRunPersistData() : nullptr;
	if (RunPersistData == nullptr || RunPersistData->IsActive() == false)
	{
		return;
	}

	for (const FPrimaryAssetId& DiceId : RunPersistData->GetDiceIds())
	{
		if (DiceId.IsValid() == false)
		{
			continue;
		}

		FDiceViewData DiceView;
		DiceView.mDiceId = DiceId;
		DiceView.mRarityType = RDUIDice::ResolveDiceRarity(DiceId);
		DiceView.mResultValue = FMath::RandRange(1, 6);

		mDiceViews.Add(MoveTemp(DiceView));
	}
}

void UCombatTileMapHUDWidget::RebuildOwnedDiceCards()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	for (UImage* OwnedDiceImage : mOwnedDiceImages)
	{
		if (OwnedDiceImage != nullptr)
		{
			OwnedDiceImage->RemoveFromParent();
		}
	}
	for (UIndexedButtonWidget* OwnedDiceCardWidget : mOwnedDiceCardWidgets)
	{
		if (OwnedDiceCardWidget != nullptr)
		{
			OwnedDiceCardWidget->RemoveFromParent();
		}
	}
	mOwnedDiceImages.Reset();
	DestroyDiceCaptureActors(mOwnedDicePreviewActors);
	mOwnedDiceCardWidgets.Reset();

	const int32 DiceCount = mDiceViews.Num();
	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		UImage* OwnedDiceImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("OwnedDiceImage_%d"), DiceIndex))
		);
		UIndexedButtonWidget* OwnedDiceCard = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(),
			FName(*FString::Printf(TEXT("OwnedDiceCard_%d"), DiceIndex))
		);
		if (OwnedDiceImage == nullptr || OwnedDiceCard == nullptr)
		{
			continue;
		}

		OwnedDiceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		OwnedDiceCard->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.01f));
		OwnedDiceCard->SetButtonIndex(DiceIndex);
		OwnedDiceCard->OnIndexedClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleOwnedDiceCardClicked);

		RootCanvas->AddChildToCanvas(OwnedDiceImage);
		RootCanvas->AddChildToCanvas(OwnedDiceCard);

		mOwnedDiceImages.Add(OwnedDiceImage);
		mOwnedDicePreviewActors.Add(nullptr);
		mOwnedDiceCardWidgets.Add(OwnedDiceCard);
	}

	ApplyRuntimeWidgetLayout();
	RefreshOwnedDiceCards();
}

void UCombatTileMapHUDWidget::RefreshOwnedDiceCards()
{
	for (int32 DiceIndex = 0; DiceIndex < mDiceViews.Num(); ++DiceIndex)
	{
		if (mDiceViews.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		const FDiceViewData& DiceView = mDiceViews[DiceIndex];
		const FLinearColor RarityColor = RDUIDice::GetDiceRarityColor(DiceView.mRarityType);
		const FLinearColor PendingColor(
			RarityColor.R * 0.55f,
			RarityColor.G * 0.55f,
			RarityColor.B * 0.55f,
			0.58f
		);

		FLinearColor DiceColor = DiceView.mIsRolled ? RarityColor : PendingColor;
		float DiceScale = DiceView.mIsRolled ? 0.76f : 0.68f;
		if (DiceIndex == mSelectedDiceIndex)
		{
			DiceColor = FLinearColor(1.0f, 0.82f, 0.30f, 1.0f);
			DiceScale = 0.98f;
		}

		if (mOwnedDicePreviewActors.IsValidIndex(DiceIndex))
		{
			if (IsValid(mOwnedDicePreviewActors[DiceIndex]) == false && mOwnedDiceImages.IsValidIndex(DiceIndex))
			{
				if (UImage* OwnedDiceImage = mOwnedDiceImages[DiceIndex])
				{
					mOwnedDicePreviewActors[DiceIndex] = SpawnDiceCaptureActor(1, DiceIndex, 384);
					if (ACombatDiceCaptureActor* OwnedDicePreviewActor = mOwnedDicePreviewActors[DiceIndex])
					{
						OwnedDicePreviewActor->SetBackdropVisible(false);
						OwnedDicePreviewActor->SetDiceRotation(GetReadableDiceIdleRotation(DiceIndex));
						OwnedDicePreviewActor->CaptureDice();
						ApplyDiceCaptureBrush(OwnedDiceImage, OwnedDicePreviewActor, FVector2D(384.0f, 384.0f));
					}
				}
			}

			if (ACombatDiceCaptureActor* OwnedDicePreviewActor = mOwnedDicePreviewActors[DiceIndex])
			{
				OwnedDicePreviewActor->SetDiceColor(DiceColor);
				OwnedDicePreviewActor->SetActorScale3D(FVector(DiceScale));
				OwnedDicePreviewActor->SetBackdropVisible(false);
				if (DiceView.mIsRolled == true)
				{
					OwnedDicePreviewActor->SettleToFace(DiceView.mResultValue);
				}
				else
				{
					OwnedDicePreviewActor->SetDiceRotation(GetReadableDiceIdleRotation(DiceIndex));
				}
				OwnedDicePreviewActor->CaptureDice();
				if (mOwnedDiceImages.IsValidIndex(DiceIndex))
				{
					ApplyDiceCaptureBrush(mOwnedDiceImages[DiceIndex], OwnedDicePreviewActor, FVector2D(384.0f, 384.0f));
				}
			}
		}
		if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex))
		{
			if (UIndexedButtonWidget* OwnedDiceCardWidget = mOwnedDiceCardWidgets[DiceIndex])
			{
				OwnedDiceCardWidget->SetBackgroundColor(DiceIndex == mSelectedDiceIndex
					? FLinearColor(1.0f, 0.78f, 0.20f, 0.34f)
					: FLinearColor(1.0f, 1.0f, 1.0f, 0.01f));
			}
		}
	}
}

void UCombatTileMapHUDWidget::HandleOwnedDiceCardClicked(int32 DiceIndex)
{
	if (mDiceViews.IsValidIndex(DiceIndex) == false || mDiceViews[DiceIndex].mIsRolled == false)
	{
		return;
	}

	if (mSelectedSkillIndex == INDEX_NONE)
	{
		mSelectedDiceIndex = INDEX_NONE;
		RefreshOwnedDiceCards();
		RefreshDiceAssignmentText();
		return;
	}

	mSelectedDiceIndex = DiceIndex;
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
