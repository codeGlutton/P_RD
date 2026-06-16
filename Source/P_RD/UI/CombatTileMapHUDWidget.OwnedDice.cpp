#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "UI/Combat/CombatViewModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::BindCombatViewModel(UCombatViewModel* InViewModel)
{
	if (mCombatViewModel == InViewModel)
	{
		return;
	}

	// 이전 뷰모델 구독 해제 후 교체.
	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		mCombatViewModel->OnViewChanged.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatViewChanged);
	}

	mCombatViewModel = InViewModel;

	if (mCombatViewModel != nullptr)
	{
		// 행동 큐 노드가 한 단위 해소될 때마다 전투 피드를 갱신한다.
		mCombatViewModel->OnQueueNodeResolved.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		// 메타/유닛/턴 갱신 시 상단 상태바를 다시 그린다.
		mCombatViewModel->OnViewChanged.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatViewChanged);
		// 이미 어댑터가 push해 둔 현재 값을 즉시 반영(구독 전 발생분 보강).
		RefreshCombatStatusBar();
	}
}

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

	// 뷰모델이 연결돼 있으면 굴림 결과의 출처는 뷰모델(게임플레이/Mock). UI는 RNG를 갖지 않는다.
	// 미연결(시안 단독 실행)일 때만 기존처럼 임시 값을 채워 데모를 유지한다 → 회귀 없음.
	const TArray<FDiceSlotView>* ViewModelDice =
		mCombatViewModel != nullptr ? &mCombatViewModel->GetDiceViews() : nullptr;

	int32 DiceIndex = 0;
	for (const FPrimaryAssetId& DiceId : RunPersistData->GetDiceIds())
	{
		if (DiceId.IsValid() == false)
		{
			continue;
		}

		FDiceViewData DiceView;
		DiceView.mDiceId = DiceId;                                  // 정체성/희귀도는 런 데이터에서(읽기 전용)
		DiceView.mRarityType = RDUIDice::ResolveDiceRarity(DiceId);

		if (ViewModelDice != nullptr && ViewModelDice->IsValidIndex(DiceIndex))
		{
			DiceView.mResultValue = (*ViewModelDice)[DiceIndex].mResultValue;   // 굴림 결과는 뷰모델에서
			DiceView.mIsRolled = (*ViewModelDice)[DiceIndex].mIsRolled;
		}
		else
		{
			DiceView.mResultValue = FMath::RandRange(1, 6);         // 뷰모델 미연결 시 시안용 임시 값
		}

		mDiceViews.Add(MoveTemp(DiceView));
		++DiceIndex;
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

	// 스킬에 주사위 배치 의도를 게임플레이로 보낸다(STEP은 즉시 실행, BASIC은 적 탭 대기).
	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->RequestToggleDice(DiceIndex);
	}

	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
