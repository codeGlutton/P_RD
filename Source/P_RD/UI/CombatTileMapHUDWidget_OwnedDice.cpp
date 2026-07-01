#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "GameMode/CombatGameMode.h"
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
	}

	mCombatUIModel = InUIModel;

	if (mCombatUIModel != nullptr)
	{
		// 행동 큐 노드가 한 단위 해소될 때마다 전투 피드를 갱신한다.
		mCombatUIModel->OnQueueNodeResolved.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		// 이미 공급된 현재 값을 즉시 반영(구독 전 발생분 보강).
		RefreshCombatStatusBar();
	}
}

/** @brief 보유 주사위 표시 데이터를 전투 표시 모델에서 읽는다. */
void UCombatTileMapHUDWidget::RefreshDiceViewsFromUIModel()
{
	mDiceUIs.Reset();

	// 보유 주사위는 개수 제한이 없다(런 중 무제한으로 늘 수 있음).
	const TArray<FDiceSlotUI>* ViewModelDice =
		mCombatUIModel != nullptr ? &mCombatUIModel->GetDiceUIs() : nullptr;

	if (ViewModelDice != nullptr && ViewModelDice->Num() > 0)
	{
		for (const FDiceSlotUI& SlotView : *ViewModelDice)
		{
			FDiceViewData DiceView;
			DiceView.mDiceId = SlotView.mDiceId;
			DiceView.mRarityColor = SlotView.mRarityColor;
			DiceView.mRarityText = SlotView.mRarityText;
			DiceView.mResultValue = SlotView.mResultValue;
			DiceView.mRolledFaceIndex = SlotView.mRolledFaceIndex;
			DiceView.mIsRolled = SlotView.mIsRolled;
			DiceView.mIsUsed = SlotView.mIsUsed;
			DiceView.mFaceCount = SlotView.mFaceCount;
			DiceView.mFaceValues = SlotView.mFaceValues;
			DiceView.mFaceTextures = SlotView.mFaceTextures;
			mDiceUIs.Add(MoveTemp(DiceView));
		}
		return;
	}
}

int32 UCombatTileMapHUDWidget::GetCombatDiceViewCount() const
{
	return mDiceUIs.Num();
}

bool UCombatTileMapHUDWidget::GetCombatDiceView(int32 DiceIndex, FDiceViewData& OutDiceView) const
{
	if (mDiceUIs.IsValidIndex(DiceIndex) == false)
	{
		return false;
	}

	OutDiceView = mDiceUIs[DiceIndex];
	return true;
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

		// '굴림 완료' 모양 표시 여부 = 권위(게임플레이가 굴렸다) AND 연출 게이트(드러내기 단계 도달).
		const bool bShowRolled = DiceView.mIsRolled && mIntroDiceCardsRevealed;

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

		const FLinearColor RarityColor = RDUIDice::GetDiceRarityColor(DiceView);
		const FLinearColor PendingColor(
			RarityColor.R * 0.55f,
			RarityColor.G * 0.55f,
			RarityColor.B * 0.55f,
			0.58f
		);

		FLinearColor DiceColor = bShowRolled ? RarityColor : PendingColor;
		float DiceScale = bShowRolled ? 0.80f : 0.72f;
		if (DiceIndex == mSelectedDiceIndex)
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
				if (bShowRolled == true)
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
				FLinearColor CardColor = DiceIndex == mSelectedDiceIndex
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

/** @brief 굴린 주사위만 스킬 배치 후보가 되며, 실제 적용 가능 여부는 뷰모델/게임플레이가 최종 판단한다. */
void UCombatTileMapHUDWidget::HandleOwnedDiceCardClicked(int32 DiceIndex)
{
	if (mDiceUIs.IsValidIndex(DiceIndex) == false || mDiceUIs[DiceIndex].mIsRolled == false || mIntroDiceCardsRevealed == false)
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
		mSelectedDiceIndex = INDEX_NONE;
		RefreshOwnedDiceCards();
		RefreshDiceAssignmentText();
		return;
	}

	mSelectedDiceIndex = DiceIndex;

	if (ACombatGameMode* CombatGameMode = GetWorld()->GetAuthGameMode<ACombatGameMode>())
	{
		CombatGameMode->SelectDice(DiceIndex);
	}

	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
