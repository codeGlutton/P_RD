#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

namespace
{
	/** @brief 보유 주사위 썸네일에서 숫자 면 판독성이 idle 포즈보다 우선인 주사위 종류. */
	bool ShouldUseFaceOnOwnedPose(const FDiceViewData& DiceView)
	{
		return DiceView.mFaceCount == 2 || DiceView.mFaceCount == 4;
	}

	/** @brief 보유 주사위 카드의 기본 회전값을 입장 굴림 프리뷰와 같은 규칙으로 맞춘다. */
	FRotator GetOwnedDiceReadyRotation(const FDiceViewData& DiceView, int32 DiceIndex, const ACombatDiceCaptureActor* DiceActor)
	{
		if (ShouldUseFaceOnOwnedPose(DiceView) && DiceActor != nullptr)
		{
			return DiceActor->GetSettledFaceRotation(1);
		}
		return GetReadableDiceIdleRotation(DiceIndex);
	}
}

/** @brief 전투 뷰모델의 이벤트를 HUD 생명주기에 맞춰 구독하고 현재 스냅샷을 즉시 반영한다. */
void UCombatTileMapHUDWidget::BindCombatUIModel(UCombatUIModel* InUIModel)
{
	if (mCombatUIModel == InUIModel)
	{
		return;
	}

	// 이전 뷰모델 구독 해제 후 교체.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		mCombatUIModel->OnUIChanged.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatUIChanged);
		mCombatUIModel->OnActionResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatActionResolved);
	}

	mCombatUIModel = InUIModel;

	if (mCombatUIModel != nullptr)
	{
		// 행동 큐 노드가 한 단위 해소될 때마다 전투 피드를 갱신한다.
		mCombatUIModel->OnQueueNodeResolved.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		// 메타/유닛/턴 갱신 시 상단 상태바를 다시 그린다.
		mCombatUIModel->OnUIChanged.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatUIChanged);
		// 액션 확정/취소 시 스킬·주사위 선택 강조를 푼다.
		mCombatUIModel->OnActionResolved.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatActionResolved);
		// 이미 어댑터가 push해 둔 현재 값을 즉시 반영(구독 전 발생분 보강).
		RefreshCombatStatusBar();

		// 이 시점(BeginRoom, InitCombat 이후)엔 전투 모델이 준비돼 있다 — 승리 후 월드맵 흐름을 HUD가 구독한다.
		BindVictoryFlowEvents();
	}
}

/** @brief 보유 주사위 표시 데이터의 출처를 뷰모델 우선, 단독 시안용 RunPersistData fallback 순서로 결정한다. */
void UCombatTileMapHUDWidget::RefreshDiceViewsFromRunData()
{
	mDiceUIs.Reset();

	// 보유 주사위는 개수 제한이 없다(런 중 무제한으로 늘 수 있음). 뷰모델이 연결돼 있으면
	// 종류/결과/희귀도/개수 모두 뷰모델(어댑터가 push한 보유 주사위)을 그대로 따른다.
	const TArray<FDiceSlotUI>* ViewModelDice =
		mCombatUIModel != nullptr ? &mCombatUIModel->GetDiceUIs() : nullptr;

	if (ViewModelDice != nullptr && ViewModelDice->Num() > 0)
	{
		for (const FDiceSlotUI& SlotView : *ViewModelDice)
		{
			FDiceViewData DiceView;
			DiceView.mDiceId = SlotView.mDiceId;
			DiceView.mRarityType = RDUIDice::ResolveDiceRarity(SlotView.mDiceId);
			DiceView.mResultValue = SlotView.mResultValue;
			DiceView.mRolledFaceIndex = SlotView.mRolledFaceIndex;
			DiceView.mIsRolled = SlotView.mIsRolled;
			DiceView.mIsUsed = SlotView.mIsUsed;
			DiceView.mIsSelected = SlotView.mIsSelected;
			DiceView.mFaceCount = SlotView.mFaceCount;
			DiceView.mFaceValues = SlotView.mFaceValues;
			DiceView.mFaceTextures = SlotView.mFaceTextures;
			mDiceUIs.Add(MoveTemp(DiceView));
		}
		return;
	}

	// 뷰모델 미연결(시안 단독 실행) — 런 데이터의 주사위 id로 임시 값 채워 데모 유지.
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
		DiceView.mRolledFaceIndex = DiceView.mResultValue - 1;
		DiceView.mFaceCount = 6;
		for (int32 FaceIndex = 0; FaceIndex < DiceView.mFaceCount; ++FaceIndex)
		{
			DiceView.mFaceValues.Add(FaceIndex + 1);
			DiceView.mFaceTextures.Add(nullptr);
		}
		mDiceUIs.Add(MoveTemp(DiceView));
	}
}

/** @brief 보유 주사위 카드의 위젯/캡처 액터 배열을 mDiceUIs와 1:1로 다시 맞춘다. */
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
	for (UTextBlock* OwnedDiceTypeText : mOwnedDiceTypeTexts)
	{
		if (OwnedDiceTypeText != nullptr)
		{
			OwnedDiceTypeText->RemoveFromParent();
		}
	}
	mOwnedDiceImages.Reset();
	DestroyDiceCaptureActors(mOwnedDicePreviewActors);
	mOwnedDiceCardWidgets.Reset();
	mOwnedDiceTypeTexts.Reset();

	const int32 DiceCount = mDiceUIs.Num();
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

		// 주사위 종류 라벨(동전/d4/d6 등).
		UTextBlock* OwnedDiceTypeText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("OwnedDiceType_%d"), DiceIndex))
		);
		if (OwnedDiceTypeText != nullptr)
		{
			OwnedDiceTypeText->SetVisibility(ESlateVisibility::HitTestInvisible);
			OwnedDiceTypeText->SetJustification(ETextJustify::Center);
			OwnedDiceTypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 1.0f, 0.85f, 1.0f)));
		}

		RootCanvas->AddChildToCanvas(OwnedDiceImage);
		RootCanvas->AddChildToCanvas(OwnedDiceCard);
		if (OwnedDiceTypeText != nullptr)
		{
			RootCanvas->AddChildToCanvas(OwnedDiceTypeText);
		}

		mOwnedDiceImages.Add(OwnedDiceImage);
		mOwnedDicePreviewActors.Add(nullptr);
		mOwnedDiceCardWidgets.Add(OwnedDiceCard);
		mOwnedDiceTypeTexts.Add(OwnedDiceTypeText);
	}

	ApplyRuntimeWidgetLayout();
	RefreshOwnedDiceCards();
}

/** @brief 보유 주사위 카드의 숫자/색/선택/사용 상태를 현재 전투 스냅샷 기준으로 갱신한다. */
void UCombatTileMapHUDWidget::RefreshOwnedDiceCards()
{
	for (int32 DiceIndex = 0; DiceIndex < mDiceUIs.Num(); ++DiceIndex)
	{
		if (mDiceUIs.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		const FDiceViewData& DiceView = mDiceUIs[DiceIndex];

		// 종류 라벨 갱신: 2면은 "동전", 그 외는 "d{면수}"(d4/d6/.../d20).
		if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
		{
			if (UTextBlock* OwnedDiceTypeText = mOwnedDiceTypeTexts[DiceIndex])
			{
				const FString TypeLabel = DiceView.mFaceCount == 2
					? TEXT("동전")
					: FString::Printf(TEXT("d%d"), DiceView.mFaceCount);
				OwnedDiceTypeText->SetText(FText::FromString(TypeLabel));
			}
		}

		const FLinearColor RarityColor = RDUIDice::GetDiceRarityColor(DiceView.mRarityType);
		const FLinearColor PendingColor(
			RarityColor.R * 0.55f,
			RarityColor.G * 0.55f,
			RarityColor.B * 0.55f,
			0.58f
		);

		FLinearColor DiceColor = DiceView.mIsRolled ? RarityColor : PendingColor;
		float DiceScale = DiceView.mIsRolled ? 0.80f : 0.72f;
		if (DiceView.mIsSelected)
		{
			DiceColor = FLinearColor(1.0f, 0.82f, 0.30f, 1.0f);
			DiceScale = 1.02f;
		}
		if (DiceView.mIsUsed)
		{
			// 이번 턴에 쓴 주사위: 어둡게 비활성 표시.
			DiceColor = FLinearColor(0.28f, 0.28f, 0.30f, 0.5f);
			DiceScale = 0.64f;
		}

		if (mOwnedDicePreviewActors.IsValidIndex(DiceIndex))
		{
			if (IsValid(mOwnedDicePreviewActors[DiceIndex]) == false && mOwnedDiceImages.IsValidIndex(DiceIndex))
			{
				if (UImage* OwnedDiceImage = mOwnedDiceImages[DiceIndex])
				{
					// [합의필요] 384는 보유 주사위 카드 전용 RT 크기. 모바일 메모리/선명도 기준이 정해지면 정책 함수로 올린다.
					mOwnedDicePreviewActors[DiceIndex] = SpawnDiceCaptureActor(1, DiceIndex, 384);
					if (ACombatDiceCaptureActor* OwnedDicePreviewActor = mOwnedDicePreviewActors[DiceIndex])
					{
						OwnedDicePreviewActor->SetDiceType(DiceView.mFaceCount);   // 면 수에 맞는 다면체 메시로 교체
						OwnedDicePreviewActor->SetFaceData(DiceView.mFaceValues, DiceView.mFaceTextures);
						OwnedDicePreviewActor->SetBackdropVisible(false);
						OwnedDicePreviewActor->SetDiceRotation(GetOwnedDiceReadyRotation(DiceView, DiceIndex, OwnedDicePreviewActor));
						OwnedDicePreviewActor->CaptureDice();
						ApplyDiceCaptureBrush(OwnedDiceImage, OwnedDicePreviewActor, FVector2D(384.0f, 384.0f));
					}
				}
			}

			if (ACombatDiceCaptureActor* OwnedDicePreviewActor = mOwnedDicePreviewActors[DiceIndex])
			{
				OwnedDicePreviewActor->SetFaceData(DiceView.mFaceValues, DiceView.mFaceTextures);
				OwnedDicePreviewActor->SetDiceColor(DiceColor);
				OwnedDicePreviewActor->SetActorScale3D(FVector(DiceScale));
				OwnedDicePreviewActor->SetBackdropVisible(false);
				if (DiceView.mIsRolled == true)
				{
					OwnedDicePreviewActor->SettleToFace(GetDiceSettledFaceOrdinal(DiceView));
				}
				else
				{
					OwnedDicePreviewActor->SetDiceRotation(GetOwnedDiceReadyRotation(DiceView, DiceIndex, OwnedDicePreviewActor));
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
				FLinearColor CardColor = DiceView.mIsSelected
					? FLinearColor(1.0f, 0.78f, 0.20f, 0.34f)
					: FLinearColor(1.0f, 1.0f, 1.0f, 0.01f);
				if (DiceView.mIsUsed)
				{
					CardColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.60f);   // 쓴 주사위: 어두운 오버레이
				}
				OwnedDiceCardWidget->SetBackgroundColor(CardColor);
			}
		}
	}
}

/** @brief 보유 주사위 카드의 선택 강조를 모두 끈다(스킬 변경/액션 확정·취소/무스킬 클릭 시).
    선택의 진실원본은 DicePoolModel이고, 이 함수는 다음 DTO push 전까지의 즉시 강조 해제만 담당한다. */
void UCombatTileMapHUDWidget::ClearOwnedDiceSelectionHighlight()
{
	for (FDiceViewData& DiceView : mDiceUIs)
	{
		DiceView.mIsSelected = false;
	}
}

/** @brief 굴린 주사위만 스킬 배치 후보가 되며, 실제 적용 가능 여부는 뷰모델/게임플레이가 최종 판단한다. */
void UCombatTileMapHUDWidget::HandleOwnedDiceCardClicked(int32 DiceIndex)
{
	if (mDiceUIs.IsValidIndex(DiceIndex) == false || mDiceUIs[DiceIndex].mIsRolled == false)
	{
		return;
	}

	// 이번 턴에 이미 쓴 주사위는 다시 배치할 수 없다(다음 굴림까지 잠금).
	if (mDiceUIs[DiceIndex].mIsUsed)
	{
		return;
	}

	if (mSelectedSkillIndex == INDEX_NONE)
	{
		ClearOwnedDiceSelectionHighlight();
		RefreshOwnedDiceCards();
		RefreshDiceAssignmentText();
		return;
	}

	// 주사위 토글(올림/내림) 의도만 게임플레이로 보낸다. 선택의 진실원본은 DicePoolModel이며,
	// 갱신된 선택은 FDiceSlotUI.mIsSelected로 되돌아와(RefreshDiceViewsFromRunData) 여러 칸이 동시에 강조된다.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestToggleDice(DiceIndex);
	}

	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
