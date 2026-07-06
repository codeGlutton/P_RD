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
	// 보유 주사위 카드의 2D 면 텍스처. 3D 캡처(주사위별 SceneCapture+RT)를 쓰지 않고
	// 면 판만 그린다 - 값 판독성(큰 숫자)과 모바일 비용(캡처 7개 제거) 모두를 위해서다.
	const TCHAR* const OwnedDiceFaceTexturePath = TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_Base.T_DiceFace_Base");

	/** @brief 보유 주사위 값 텍스트를 크고 진하게(볼드) 맞춘다 - 면 판 위에서 한눈에 읽히게. */
	void SetOwnedDiceValueFont(UTextBlock* Text, int32 Size)
	{
		if (Text == nullptr)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		Text->SetFont(Font);
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
	for (UTextBlock* OwnedDiceValueText : mOwnedDiceValueTexts)
	{
		if (OwnedDiceValueText != nullptr)
		{
			OwnedDiceValueText->RemoveFromParent();
		}
	}
	mOwnedDiceImages.Reset();
	DestroyDiceCaptureActors(mOwnedDicePreviewActors);
	mOwnedDiceCardWidgets.Reset();
	mOwnedDiceTypeTexts.Reset();
	mOwnedDiceValueTexts.Reset();

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

		// 굴림 값 텍스트: 2D 면 판 중앙에 크고 진하게.
		UTextBlock* OwnedDiceValueText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("OwnedDiceValue_%d"), DiceIndex))
		);
		if (OwnedDiceValueText != nullptr)
		{
			OwnedDiceValueText->SetVisibility(ESlateVisibility::HitTestInvisible);
			OwnedDiceValueText->SetJustification(ETextJustify::Center);
			SetOwnedDiceValueFont(OwnedDiceValueText, 30);
		}

		// 스킨 활성 시 DesignCanvas에 붙여 레터박스 스킨(주사위 트레이 아트)과 함께 움직이게 한다.
		// RootCanvas(뷰포트)에 붙이면 16:9가 아닐 때 트레이만 따로 노는 정렬 버그가 생긴다.
		UCanvasPanel* OwnedDiceCanvas = GetSkinTargetCanvas();
		OwnedDiceCanvas->AddChildToCanvas(OwnedDiceImage);
		OwnedDiceCanvas->AddChildToCanvas(OwnedDiceCard);
		if (OwnedDiceTypeText != nullptr)
		{
			OwnedDiceCanvas->AddChildToCanvas(OwnedDiceTypeText);
		}
		if (OwnedDiceValueText != nullptr)
		{
			OwnedDiceCanvas->AddChildToCanvas(OwnedDiceValueText);
		}

		mOwnedDiceImages.Add(OwnedDiceImage);
		mOwnedDicePreviewActors.Add(nullptr);
		mOwnedDiceCardWidgets.Add(OwnedDiceCard);
		mOwnedDiceTypeTexts.Add(OwnedDiceTypeText);
		mOwnedDiceValueTexts.Add(OwnedDiceValueText);
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

		// 2D 면 판: 3D 캡처 대신 면 텍스처에 상태색(레어도/선택/사용)을 틴트로 얹는다.
		if (mOwnedDiceImages.IsValidIndex(DiceIndex))
		{
			if (UImage* OwnedDiceImage = mOwnedDiceImages[DiceIndex])
			{
				if (mOwnedDiceFaceTexture == nullptr)
				{
					mOwnedDiceFaceTexture = LoadObject<UTexture2D>(nullptr, OwnedDiceFaceTexturePath);
				}
				if (mOwnedDiceFaceTexture != nullptr)
				{
					OwnedDiceImage->SetBrushFromTexture(mOwnedDiceFaceTexture, false);
				}
				OwnedDiceImage->SetColorAndOpacity(DiceColor);
				OwnedDiceImage->SetRenderScale(FVector2D(DiceScale, DiceScale));
			}
		}

		// 굴림 값: 면 판 중앙에 크고 진하게. 안 굴렸으면 물음표, 사용했으면 흐리게.
		if (mOwnedDiceValueTexts.IsValidIndex(DiceIndex))
		{
			if (UTextBlock* OwnedDiceValueText = mOwnedDiceValueTexts[DiceIndex])
			{
				OwnedDiceValueText->SetText(DiceView.mIsRolled
					? FText::AsNumber(DiceView.mResultValue)
					: NSLOCTEXT("CombatTileMapHUDWidget", "OwnedDiceNotRolled", "?"));
				OwnedDiceValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.07f, 0.06f, 0.11f, 1.0f)));
				OwnedDiceValueText->SetRenderOpacity(DiceView.mIsUsed ? 0.35f : 1.0f);
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
