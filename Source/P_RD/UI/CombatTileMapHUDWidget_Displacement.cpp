#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"

namespace
{
	void HidePreviewWidget(UWidget* Widget)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCombatTileMapHUDWidget::RefreshDisplacementPreview()
{
	// 직접 드래그는 손을 놓는 순간 실행된다. 확인 카드가 전장을 가리거나 한 번 더 누르게 하지 않는다.
	if (mDisplacementConfirmPanel != nullptr)
	{
		mDisplacementConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCombatTileMapHUDWidget::UpdateDisplacementPreviewVisuals(float InDeltaTime)
{
	(void)InDeltaTime;
	if (mCombatUIModel == nullptr || mCombatControlsHidden)
	{
		for (UBorder* Segment : mDisplacementPathSegments) { HidePreviewWidget(Segment); }
		for (UTextBlock* Label : mDisplacementDirectionLabels) { HidePreviewWidget(Label); }
		HidePreviewWidget(mDisplacementTargetLabel);
		HidePreviewWidget(mDisplacementLandingGhost);
		HidePreviewWidget(mDisplacementLandingLabel);
		HidePreviewWidget(mDisplacementCollisionLabel);
		return;
	}

	const FDisplacementPreviewUI& Preview = mCombatUIModel->GetDisplacementPreview();
	APlayerController* PlayerController = GetOwningPlayer();
	if (Preview.mIsActive == false || PlayerController == nullptr)
	{
		for (UBorder* Segment : mDisplacementPathSegments) { HidePreviewWidget(Segment); }
		for (UTextBlock* Label : mDisplacementDirectionLabels) { HidePreviewWidget(Label); }
		HidePreviewWidget(mDisplacementTargetLabel);
		HidePreviewWidget(mDisplacementLandingGhost);
		HidePreviewWidget(mDisplacementLandingLabel);
		HidePreviewWidget(mDisplacementCollisionLabel);
		return;
	}

	auto Project = [PlayerController](const FVector& World, FVector2D& OutScreen)
	{
		return UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, World, OutScreen, false);
	};
	const float Pulse = 0.78f + 0.22f * (0.5f + 0.5f * FMath::Sin(
		GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() * 6.0f : 0.0f));
	const FLinearColor PathColor = Preview.mIsPull
		? FLinearColor(0.28f, 1.0f, 0.92f, Pulse)
		: (Preview.mIsSwap
			? FLinearColor(0.72f, 0.42f, 1.0f, Pulse)
			: (Preview.mIsStagger
				? FLinearColor(1.0f, 0.82f, 0.18f, Pulse)
				: FLinearColor(1.0f, 0.48f, 0.06f, Pulse)));

	// 선택 결과는 화면 위 굵은 선으로 연결한다. 바닥의 작은 셀 색을 읽지 않아도 이동 방향이 보인다.
	int32 VisibleSegmentCount = 0;
	for (int32 Index = 0; Index + 1 < Preview.mTrajectoryWorldLocations.Num()
		&& VisibleSegmentCount < mDisplacementPathSegments.Num(); ++Index)
	{
		FVector2D Start;
		FVector2D End;
		if (Project(Preview.mTrajectoryWorldLocations[Index] + FVector(0.0f, 0.0f, 36.0f), Start) == false
			|| Project(Preview.mTrajectoryWorldLocations[Index + 1] + FVector(0.0f, 0.0f, 36.0f), End) == false)
		{
			continue;
		}
		UBorder* Segment = mDisplacementPathSegments[VisibleSegmentCount++];
		if (Segment == nullptr) { continue; }
		const FVector2D Delta = End - Start;
		const float Length = Delta.Size();
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Segment->Slot))
		{
			CanvasSlot->SetPosition((Start + End) * 0.5f);
			CanvasSlot->SetSize(FVector2D(Length, Preview.mIsThrow ? 8.0f : 6.0f));
		}
		FWidgetTransform Transform;
		Transform.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		Segment->SetRenderTransform(Transform);
		Segment->SetBrushColor(PathColor);
		Segment->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	for (int32 Index = VisibleSegmentCount; Index < mDisplacementPathSegments.Num(); ++Index)
	{
		HidePreviewWidget(mDisplacementPathSegments[Index]);
	}

	FVector2D TargetScreen;
	const bool bTargetOnScreen = Project(Preview.mTargetWorldLocation + FVector(0.0f, 0.0f, 70.0f), TargetScreen);
	if (mDisplacementTargetLabel != nullptr && bTargetOnScreen)
	{
		const FString TargetLabelText = Preview.mIsPull
			? FString::Printf(TEXT("당길 적 · %s"), *Preview.mTargetName.ToString())
			: (Preview.mIsSwap
				? FString::Printf(TEXT("자리 바꿀 적 · %s"), *Preview.mTargetName.ToString())
				: (Preview.mIsStagger
					? FString::Printf(TEXT("다리 걸 대상 · %s"), *Preview.mTargetName.ToString())
					: FString::Printf(TEXT("던질 적 · %s"), *Preview.mTargetName.ToString())));
		mDisplacementTargetLabel->SetText(FText::FromString(TargetLabelText));
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mDisplacementTargetLabel->Slot))
		{
			CanvasSlot->SetPosition(TargetScreen + FVector2D(0.0f, -28.0f));
		}
		mDisplacementTargetLabel->SetRenderOpacity(Pulse);
		mDisplacementTargetLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		HidePreviewWidget(mDisplacementTargetLabel);
	}

	const bool bChoosingDirection = Preview.mIsThrow
		&& mCombatUIModel->GetTurnUI().mPhase == ECombatBuildPhaseUI::ThrowDestinationSelection;
	int32 VisibleDirectionCount = 0;
	if (bChoosingDirection && bTargetOnScreen)
	{
		for (const FVector& CandidateWorld : Preview.mDirectionCandidateWorldLocations)
		{
			if (VisibleDirectionCount >= mDisplacementDirectionLabels.Num()) { break; }
			FVector2D CandidateScreen;
			if (Project(CandidateWorld + FVector(0.0f, 0.0f, 42.0f), CandidateScreen) == false) { continue; }
			UTextBlock* Label = mDisplacementDirectionLabels[VisibleDirectionCount++];
			if (Label == nullptr) { continue; }
			Label->SetText(FText::FromString(TEXT("➜")));
			const FVector2D Delta = CandidateScreen - TargetScreen;
			FWidgetTransform Transform;
			Transform.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			Label->SetRenderTransform(Transform);
			Label->SetRenderOpacity(Pulse);
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Label->Slot))
			{
				CanvasSlot->SetPosition(CandidateScreen);
			}
			Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	for (int32 Index = VisibleDirectionCount; Index < mDisplacementDirectionLabels.Num(); ++Index)
	{
		HidePreviewWidget(mDisplacementDirectionLabels[Index]);
	}

	FVector2D LandingScreen = FVector2D::ZeroVector;
	const bool bHasLanding = Preview.mLandingTile != FTileIndex::Invalid
		&& Project(Preview.mLandingWorldLocation + FVector(0.0f, 0.0f, 35.0f), LandingScreen);
	if (bHasLanding)
	{
		if (mDisplacementLandingGhost != nullptr)
		{
			const FUnitUI* TargetUI = mCombatUIModel->GetUnitUIs().FindByPredicate([&Preview](const FUnitUI& Unit)
			{
				return Unit.mUnitId == Preview.mTargetUnitId;
			});
			if (TargetUI != nullptr && TargetUI->mPortrait != nullptr)
			{
				mDisplacementLandingGhost->SetBrushFromTexture(TargetUI->mPortrait, false);
				mDisplacementLandingGhost->SetColorAndOpacity(Preview.mIsPull
					? FLinearColor(0.30f, 1.0f, 0.88f, 0.48f)
					: (Preview.mIsSwap
						? FLinearColor(0.72f, 0.42f, 1.0f, 0.48f)
						: FLinearColor(1.0f, 0.50f, 0.08f, 0.48f)));
				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mDisplacementLandingGhost->Slot))
				{
					CanvasSlot->SetPosition(LandingScreen);
				}
				mDisplacementLandingGhost->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				HidePreviewWidget(mDisplacementLandingGhost);
			}
		}
		if (mDisplacementLandingLabel != nullptr)
		{
			const FString LandingText = Preview.mIsPull
				? TEXT("여기로 당김")
				: (Preview.mIsSwap
					? TEXT("서로 자리 교환")
					: (Preview.mIsStagger
						? TEXT("다음 이동력 감소")
						: FString::Printf(TEXT("여기 착지 · %d칸"), Preview.mMoveDistance)));
			mDisplacementLandingLabel->SetText(FText::FromString(LandingText));
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mDisplacementLandingLabel->Slot))
			{
				CanvasSlot->SetPosition(LandingScreen + FVector2D(0.0f, 42.0f));
			}
			mDisplacementLandingLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	else
	{
		HidePreviewWidget(mDisplacementLandingGhost);
		HidePreviewWidget(mDisplacementLandingLabel);
	}

	FVector2D CollisionScreen = FVector2D::ZeroVector;
	const bool bHasCollision = Preview.mCollisionTile != FTileIndex::Invalid
		&& Project(Preview.mCollisionWorldLocation + FVector(0.0f, 0.0f, 60.0f), CollisionScreen);
	if (mDisplacementCollisionLabel != nullptr && bHasCollision)
	{
		mDisplacementCollisionLabel->SetText(FText::FromString(FString::Printf(
			TEXT("✹ 충돌!  %s"),
			Preview.mCollisionName.IsEmpty() ? TEXT("장애물") : *Preview.mCollisionName.ToString())));
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(mDisplacementCollisionLabel->Slot))
		{
			CanvasSlot->SetPosition(CollisionScreen);
		}
		mDisplacementCollisionLabel->SetRenderOpacity(Pulse);
		mDisplacementCollisionLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		HidePreviewWidget(mDisplacementCollisionLabel);
	}
}

void UCombatTileMapHUDWidget::HandleDisplacementConfirmClicked()
{
	if (mCombatUIModel != nullptr)
	{
		if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmDestination)
		{
			mEnemyIntentTutorialInterventionSubmitted = true;
		}
		else if (mEnemyIntentTutorialStage == EEnemyIntentTutorialStage::ConfirmThrow)
		{
			mEnemyIntentTutorialThrowSubmitted = true;
		}
		mCombatUIModel->RequestConfirmSkill();
	}
}

void UCombatTileMapHUDWidget::HandleDisplacementCancelClicked()
{
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestCancel();
	}
}
