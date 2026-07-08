#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/DiceCapturePreviewUtils.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

namespace
{
	constexpr int32 OwnedDiceRenderTargetSize = 256;

	float GetOwnedDiceCaptureScale(int32 FaceCount)
	{
		switch (FaceCount)
		{
		case 4:
			return 0.86f;
		case 12:
			return 1.16f;
		case 20:
			return 1.22f;
		default:
			return 1.0f;
		}
	}

	float GetOwnedDiceCaptureTextScale(int32 FaceCount)
	{
		switch (FaceCount)
		{
		case 12:
		case 20:
			return 1.50f;
		default:
			return 1.36f;
		}
	}
}

/** @brief 전투 뷰모델의 이벤트를 HUD 생명주기에 맞춰 구독하고 현재 스냅샷을 즉시 반영한다. */
void UCombatTileMapHUDWidget::BindCombatUIModel(UCombatUIModel* InUIModel)
{
	if (mCombatUIModel == InUIModel || InUIModel == nullptr)
	{
		return;
	}

	// 이전 뷰모델 구독 해제 후 교체.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved);
		mCombatUIModel->OnUIChanged.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatUIChanged);
		mCombatUIModel->OnActionResolved.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatActionResolved);
		mCombatUIModel->OnCombatFloatingLog.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLog);
		mCombatUIModel->OnCombatFloatingLogMotionFinished.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLogMotionFinished);
		mCombatUIModel->OnDiceRollRequested.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatDiceRollRequested);
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
		// HP 증감 등 전투 이벤트를 유닛 머리 위 플로팅 텍스트로 띄운다.
		mCombatUIModel->OnCombatFloatingLog.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLog);
		// 모션 연출 종료 시 해당 모션에 묶인 플로팅 로그를 정리한다.
		mCombatUIModel->OnCombatFloatingLogMotionFinished.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLogMotionFinished);
		// 시뮬레이션 전환/취소 시 현재 떠 있는 플로팅 로그를 한 번에 걷어낸다.
		mCombatUIModel->OnCombatFloatingLogsCleared.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatFloatingLogsCleared);
		// 턴 시작 주사위 굴림 요청 시 굴림 오버레이를 자동으로 연다(2턴째부터의 진행 멈춤 해소).
		mCombatUIModel->OnDiceRollRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatDiceRollRequested);

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

/** @brief 보유 주사위 카드의 3D 캡처/입력 위젯 배열을 mDiceUIs와 1:1로 다시 맞춘다. */
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

		// 주사위 종류 라벨(d6/d20)은 보유 주사위 카드에서는 보여주지 않는다.
		UTextBlock* OwnedDiceTypeText = nullptr;

		// 스킨 활성 시 DesignCanvas에 붙여 레터박스 스킨(주사위 트레이 아트)과 함께 움직이게 한다.
		// RootCanvas(뷰포트)에 붙이면 16:9가 아닐 때 트레이만 따로 노는 정렬 버그가 생긴다.
		UCanvasPanel* OwnedDiceCanvas = GetSkinTargetCanvas();
		OwnedDiceCanvas->AddChildToCanvas(OwnedDiceImage);
		OwnedDiceCanvas->AddChildToCanvas(OwnedDiceCard);
		if (OwnedDiceTypeText != nullptr)
		{
			OwnedDiceCanvas->AddChildToCanvas(OwnedDiceTypeText);
		}
		mOwnedDiceImages.Add(OwnedDiceImage);
		mOwnedDicePreviewActors.Add(nullptr);
		mOwnedDiceCardWidgets.Add(OwnedDiceCard);
		mOwnedDiceTypeTexts.Add(OwnedDiceTypeText);
	}

	ApplyRuntimeWidgetLayout();
	RefreshOwnedDiceCards();
}

/** @brief 보유 주사위 카드의 3D 캡처 숫자/색/선택/사용 상태를 현재 전투 스냅샷 기준으로 갱신한다. */
void UCombatTileMapHUDWidget::RefreshOwnedDiceCards()
{
	for (int32 DiceIndex = 0; DiceIndex < mDiceUIs.Num(); ++DiceIndex)
	{
		if (mDiceUIs.IsValidIndex(DiceIndex) == false)
		{
			continue;
		}

		const FDiceViewData& DiceView = mDiceUIs[DiceIndex];

		// 종류 라벨(d6/d20 등)은 보유 주사위 카드에서 항상 숨긴다.
		if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
		{
			if (UTextBlock* OwnedDiceTypeText = mOwnedDiceTypeTexts[DiceIndex])
			{
				OwnedDiceTypeText->SetText(FText::GetEmpty());
				OwnedDiceTypeText->SetVisibility(ESlateVisibility::Collapsed);
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
		if (DiceView.mIsSelected)
		{
			DiceColor = FLinearColor(1.0f, 0.82f, 0.30f, 1.0f);
		}
		if (DiceView.mIsUsed)
		{
			// 이번 턴에 쓴 주사위: 어둡게 비활성 표시.
			DiceColor = FLinearColor(0.28f, 0.28f, 0.30f, 0.5f);
		}

		if (mOwnedDicePreviewActors.IsValidIndex(DiceIndex) == false)
		{
			mOwnedDicePreviewActors.SetNum(DiceIndex + 1);
		}
		if (IsValid(mOwnedDicePreviewActors[DiceIndex]) == false)
		{
			mOwnedDicePreviewActors[DiceIndex] = SpawnDiceCaptureActor(0, DiceIndex, OwnedDiceRenderTargetSize);
		}

		ACombatDiceCaptureActor* OwnedDiceActor = mOwnedDicePreviewActors.IsValidIndex(DiceIndex)
			? mOwnedDicePreviewActors[DiceIndex].Get()
			: nullptr;
		if (OwnedDiceActor != nullptr)
		{
			const bool bHasRolledResult = DiceView.mIsRolled && DiceView.mResultValue > 0;
			const int32 FaceOrdinal = bHasRolledResult ? GetDiceSettledFaceOrdinal(DiceView) : INDEX_NONE;

			OwnedDiceActor->SetDiceType(DiceView.mFaceCount);
			OwnedDiceActor->SetFaceData(DiceView.mFaceValues, DiceView.mFaceTextures);
			OwnedDiceActor->SetDiceMaterialVariant(DiceIndex);
			OwnedDiceActor->SetDiceColor(DiceColor);
			OwnedDiceActor->SetFaceTextScale(GetOwnedDiceCaptureTextScale(DiceView.mFaceCount));
			OwnedDiceActor->SetActorScale3D(FVector(
				RDDiceCapturePreview::GetCombatPreviewDiceScale() *
				GetOwnedDiceCaptureScale(DiceView.mFaceCount)
			));
			OwnedDiceActor->SetBackdropVisible(false);
			OwnedDiceActor->SetHideNonHighlightedFaceTexts(true);
			if (bHasRolledResult)
			{
				OwnedDiceActor->SetHighlightedFace(FaceOrdinal);
				OwnedDiceActor->SettleToFace(FaceOrdinal);
			}
			else
			{
				OwnedDiceActor->ClearHighlightedFace();
				OwnedDiceActor->SettleToFace(1);
			}
			OwnedDiceActor->CaptureDice();
		}

		if (mOwnedDiceImages.IsValidIndex(DiceIndex))
		{
			if (UImage* OwnedDiceImage = mOwnedDiceImages[DiceIndex])
			{
				if (OwnedDiceActor != nullptr)
				{
					ApplyDiceCaptureBrush(OwnedDiceImage, OwnedDiceActor, FVector2D(OwnedDiceRenderTargetSize));
				}
				OwnedDiceImage->SetColorAndOpacity(FLinearColor::White);
				OwnedDiceImage->SetRenderScale(FVector2D::UnitVector);
				OwnedDiceImage->SetRenderOpacity(DiceView.mIsUsed ? 0.42f : 1.0f);
				OwnedDiceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
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
