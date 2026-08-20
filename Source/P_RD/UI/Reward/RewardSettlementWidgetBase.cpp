#include "UI/Reward/RewardSettlementWidgetBase.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/ViewportZOrderType.h"
#include "UI/Reward/RewardUIModel.h"

#define LOCTEXT_NAMESPACE "RewardSettlementWidget"

namespace
{
	FIntPoint NativeTextureSize(UTexture2D* Texture)
	{
		if (Texture == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		const FIntPoint ImportedSize = Texture->GetImportedSize();
		if (ImportedSize.X > 0 && ImportedSize.Y > 0)
		{
			return ImportedSize;
		}
		return FIntPoint(Texture->GetSizeX(), Texture->GetSizeY());
	}

	FSlateBrush TextureBrush(UTexture2D* Texture, const FBox2f* UV = nullptr)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		const FIntPoint NativeSize = NativeTextureSize(Texture);
		if (NativeSize.X > 0 && NativeSize.Y > 0)
		{
			Brush.ImageSize = FVector2D(NativeSize);
		}
		if (UV != nullptr)
		{
			Brush.SetUVRegion(*UV);
		}
		return Brush;
	}

	void SetImageTexture(UImage* Image, UTexture2D* Texture)
	{
		if (Image == nullptr)
		{
			return;
		}
		Image->SetBrush(TextureBrush(Texture));
		Image->SetVisibility(Texture != nullptr
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	FName IndexedWidgetName(const TCHAR* Prefix, const int32 Index)
	{
		return FName(*FString::Printf(TEXT("%s_%d"), Prefix, Index));
	}

	template <typename WidgetType>
	WidgetType* FindIndexedWidget(const URewardSettlementWidgetBase* Owner,
		const TCHAR* Prefix, const int32 Index)
	{
		return Owner != nullptr
			? Cast<WidgetType>(Owner->GetWidgetFromName(IndexedWidgetName(Prefix, Index)))
			: nullptr;
	}

	FText ChoiceFallback(const FRewardChoiceUI& Choice)
	{
		if (Choice.mName.IsEmpty() == false)
		{
			return Choice.mName;
		}
		switch (Choice.mKind)
		{
		case ERewardChoiceKind::Skill: return LOCTEXT("SkillReward", "스킬");
		case ERewardChoiceKind::Equipment: return LOCTEXT("EquipmentReward", "아티팩트");
		case ERewardChoiceKind::Gold: return LOCTEXT("GoldReward", "골드");
		default: return FText::GetEmpty();
		}
	}
}

URewardSettlementWidgetBase::URewardSettlementWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

#define RD_SETTLEMENT_TEX(Member, Path) Member = LoadObject<UTexture2D>(nullptr, TEXT(Path))
	RD_SETTLEMENT_TEX(mGoldIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_gold_icon.T_reward_v4_gold_icon");
	RD_SETTLEMENT_TEX(mEquipmentIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common");
	RD_SETTLEMENT_TEX(mChestClosedTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_WireframeV4_ChestClosed.T_RS_WireframeV4_ChestClosed");
	RD_SETTLEMENT_TEX(mChestHalfOpenTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_WireframeV4_ChestOpen.T_RS_WireframeV4_ChestOpen");
	RD_SETTLEMENT_TEX(mChestOpenTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_WireframeV4_ChestOpen.T_RS_WireframeV4_ChestOpen");
	RD_SETTLEMENT_TEX(mChestRevealAuraTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/T_RS_Generated_ChestRevealAuraV2.T_RS_Generated_ChestRevealAuraV2");
	RD_SETTLEMENT_TEX(mChoiceCardNormalTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V7/T_RS_V7_ArtifactCard.T_RS_V7_ArtifactCard");
	RD_SETTLEMENT_TEX(mChoiceCardSelectedTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V7/T_RS_V7_ArtifactCard.T_RS_V7_ArtifactCard");
	RD_SETTLEMENT_TEX(mStepCoinActiveTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinActive.T_C03_StepCoinActive");
	RD_SETTLEMENT_TEX(mStepCoinInactiveTexture, "/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinInactive.T_C03_StepCoinInactive");
	// 스킬 아이콘 그림은 지웠다(새 그림 예정). 투명 그림 하나를 세워 두는 대신
	// 아예 비워 둔다 -- 자리를 채우려고 만든 자산이 또 다른 지울 것이 된다.
	// 쓰는 쪽은 이미 nullptr 을 검사하고 그리지 않는다.
#undef RD_SETTLEMENT_TEX
}

void URewardSettlementWidgetBase::OpenUI(FOnEndUIOpenAnimation Callback)
{
	mContinueCommitted = false;
	mCurrentStep = 1;
	mSelectedChoice = INDEX_NONE;
	mChestOpening = false;
	ResetChestPresentation();
	StopAllAnimations();
	Super::OpenUI(MoveTemp(Callback));
	StopAllAnimations();
	FinishOpenUI();
}

void URewardSettlementWidgetBase::CloseUI(FOnEndUICloseAnimation Callback)
{
	StopAllAnimations();
	Super::CloseUI(MoveTemp(Callback));
	StopAllAnimations();
	FinishCloseUI();
}

void URewardSettlementWidgetBase::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void URewardSettlementWidgetBase::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

void URewardSettlementWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (mNextButton != nullptr)
	{
		mNextButton->OnClicked.AddUniqueDynamic(this, &URewardSettlementWidgetBase::HandleNextClicked);
		mNextButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (UButton* ChoiceButton = FindIndexedWidget<UButton>(this, TEXT("SettlementChoiceButton"), 0))
	{
		ChoiceButton->OnClicked.AddUniqueDynamic(this,
			&URewardSettlementWidgetBase::HandleChoiceClicked_0);
	}
	if (UButton* ChoiceButton = FindIndexedWidget<UButton>(this, TEXT("SettlementChoiceButton"), 1))
	{
		ChoiceButton->OnClicked.AddUniqueDynamic(this,
			&URewardSettlementWidgetBase::HandleChoiceClicked_1);
	}
	if (UButton* ChoiceButton = FindIndexedWidget<UButton>(this, TEXT("SettlementChoiceButton"), 2))
	{
		ChoiceButton->OnClicked.AddUniqueDynamic(this,
			&URewardSettlementWidgetBase::HandleChoiceClicked_2);
	}
	if (UButton* ChestButton = Cast<UButton>(GetWidgetFromName(TEXT("SettlementChestButton"))))
	{
		ChestButton->OnClicked.AddUniqueDynamic(this,
			&URewardSettlementWidgetBase::HandleChestClicked);
	}
	RefreshView();
}

void URewardSettlementWidgetBase::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mChestOpenTimer);
		World->GetTimerManager().ClearTimer(mChestRevealTimer);
		World->GetTimerManager().ClearTimer(mChestAnimationTimer);
		World->GetTimerManager().ClearTimer(mChoiceAnimationTimer);
	}
	UnbindUIModel();
	Super::NativeDestruct();
}

void URewardSettlementWidgetBase::BindUIModel(URewardUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		RefreshView();
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &URewardSettlementWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.AddDynamic(this, &URewardSettlementWidgetBase::HandleChoicesChanged);
	}
	RefreshView();
}

void URewardSettlementWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &URewardSettlementWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.RemoveDynamic(this, &URewardSettlementWidgetBase::HandleChoicesChanged);
	}
	mUIModel = nullptr;
}

void URewardSettlementWidgetBase::HandleUIChanged()
{
	RefreshView();
}

void URewardSettlementWidgetBase::HandleChoicesChanged()
{
	RefreshView();
}

void URewardSettlementWidgetBase::HandleNextClicked()
{
	ContinueToNext();
}

void URewardSettlementWidgetBase::ContinueToNext()
{
	if (mContinueCommitted || mRewardRevealPlaying)
	{
		return;
	}

	// 단계① -> 먼저 경험치 상승만 확인한다. 골드는 상자를 연 뒤 지급한다.
	if (mCurrentStep == 1 && mUIModel != nullptr)
	{
		const FRewardUI Reward = mUIModel->GetReward();
		if (Reward.mExpGained > 0)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Exp);
		}
		mCurrentStep = 2;
		mSelectedChoice = INDEX_NONE;
		ResetChestPresentation();
		RebuildStep();
		return;
	}

	// 단계②는 중앙 상자를 직접 눌러서만 진행한다.
	if (mCurrentStep == 2)
	{
		return;
	}

	// 단계③은 골드 전용이다. 아티팩트가 있는 방만 별도 단계④로 보낸다.
	if (mCurrentStep == 3 && mUIModel != nullptr)
	{
		const FRewardUI Reward = mUIModel->GetReward();
		if (Reward.mGoldGained > 0)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Gold);
		}
		if (mUIModel->GetRewardChoices().Num() > 0)
		{
			mCurrentStep = 4;
			mSelectedChoice = INDEX_NONE;
			RebuildStep();
			BeginChoiceReveal();
			return;
		}
	}

	// 단계④는 아티팩트가 있는 방에만 존재하며 하나를 고른 뒤 확정한다.
	if (mCurrentStep == 4 && mUIModel != nullptr)
	{
		const TArray<FRewardChoiceUI> Choices = mUIModel->GetRewardChoices();
		if (Choices.IsValidIndex(mSelectedChoice) == false)
		{
			return;
		}
		if (Choices.IsValidIndex(mSelectedChoice))
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Choice,
				Choices[mSelectedChoice].mChoiceIndex);
		}
	}

	mContinueCommitted = true;
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaim();
	}
	OnClosed.Broadcast();
	CloseUI();
}

void URewardSettlementWidgetBase::RefreshView()
{
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(mUIModel != nullptr && mUIModel->GetReward().mTitle.IsEmpty() == false
			? mUIModel->GetReward().mTitle
			: LOCTEXT("DefaultTitle", "전투 보상"));
	}
	// 좌측 요약 기둥과 세로 상자 줄은 확정 시안(0809)에서 뺐다.
	if (mSummaryRowsBox != nullptr)
	{
		mSummaryRowsBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mMercenaryRowsBox != nullptr)
	{
		mMercenaryRowsBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mGoldBalanceText != nullptr)
	{
		mGoldBalanceText->SetVisibility(ESlateVisibility::Collapsed);
	}
	RebuildStep();
}

void URewardSettlementWidgetBase::RefreshStepCoins()
{
	if (UTextBlock* StepText = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("SettlementStepText"))))
	{
		const bool bHasChoices = mUIModel != nullptr
			&& mUIModel->GetRewardChoices().Num() > 0;
		const FText LabelsWithArtifact[] = {
			LOCTEXT("ExpStepC03", "경험치"),
			LOCTEXT("ChestStepC03", "상자"),
			LOCTEXT("GoldStepC03", "골드"),
			LOCTEXT("ArtifactStepC03", "아티팩트") };
		const FText LabelsGoldOnly[] = {
			LOCTEXT("ExpStepGoldOnlyC03", "경험치"),
			LOCTEXT("ChestStepGoldOnlyC03", "상자"),
			LOCTEXT("GoldStepGoldOnlyC03", "골드") };
		StepText->SetText(bHasChoices
			? LabelsWithArtifact[FMath::Clamp(mCurrentStep - 1, 0, 3)]
			: LabelsGoldOnly[FMath::Clamp(mCurrentStep - 1, 0, 2)]);
	}

	const float StepCenters[] = { 530.f, 706.f, 882.f, 1058.f };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const bool bActive = Index == mCurrentStep - 1;
		if (UImage* Coin = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("SettlementStepCoin_%d"), Index))))
		{
			SetImageTexture(Coin, bActive
				? mStepCoinActiveTexture.Get() : mStepCoinInactiveTexture.Get());
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Coin->Slot))
			{
				const float Extent = bActive ? 92.f : 64.f;
				CanvasSlot->SetPosition(FVector2D(StepCenters[Index] - Extent * .5f,
					269.f - Extent * .5f));
				CanvasSlot->SetSize(FVector2D(Extent, Extent));
			}
		}
	}

	if (UCanvasPanel* FillClip = Cast<UCanvasPanel>(
		GetWidgetFromName(TEXT("SettlementStepFillClip"))))
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FillClip->Slot))
		{
			const float Widths[] = { 100.f, 276.f, 452.f, 628.f };
			CanvasSlot->SetSize(FVector2D(
				Widths[FMath::Clamp(mCurrentStep - 1, 0, 3)], 16.f));
		}
	}
}

void URewardSettlementWidgetBase::RebuildStep()
{
	RefreshStepCoins();

	const int32 ChoiceCount = mUIModel != nullptr ? mUIModel->GetRewardChoices().Num() : 0;
	const int32 VisibleChoiceCount = FMath::Min(ChoiceCount, 3);
	if (mCurrentStep == 4
		&& (mSelectedChoice < 0 || mSelectedChoice >= VisibleChoiceCount))
	{
		mSelectedChoice = INDEX_NONE;
	}
	if (mCurrentStep == 1)
	{
		BuildResultStep();
	}
	else if (mCurrentStep == 2)
	{
		BuildChestStep();
	}
	else if (mCurrentStep == 3)
	{
		BuildGoldStep();
	}
	else
	{
		BuildChoiceStep();
	}
	if (mNextButtonText != nullptr)
	{
		const bool bGoldLeadsToArtifact = mCurrentStep == 3 && VisibleChoiceCount > 0;
		mNextButtonText->SetText(mCurrentStep == 4
			? LOCTEXT("Confirm", "확정")
			: (mCurrentStep <= 2 || bGoldLeadsToArtifact
				? LOCTEXT("Next", "다음") : LOCTEXT("Take", "받기")));
	}
	if (mNextButton != nullptr)
	{
		mNextButton->SetVisibility(ESlateVisibility::Visible);
		mNextButton->SetIsEnabled(mCurrentStep == 1
			|| (mCurrentStep == 3 && !mRewardRevealPlaying)
			|| (mCurrentStep == 4 && !mRewardRevealPlaying
				&& mSelectedChoice != INDEX_NONE));
	}
	if (UWidget* Holder = GetWidgetFromName(TEXT("NextButtonHolder")))
	{
		Holder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void URewardSettlementWidgetBase::BuildResultStep()
{
	if (UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("SettlementStepSwitcher"))))
	{
		StepSwitcher->SetActiveWidgetIndex(0);
	}
	else
	{
		if (UWidget* ResultStep = GetWidgetFromName(TEXT("SettlementResultStep")))
		{
			ResultStep->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		if (UWidget* ChoiceStep = GetWidgetFromName(TEXT("SettlementChoiceStep")))
		{
			ChoiceStep->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const FRewardUI* Reward = mUIModel != nullptr ? &mUIModel->GetReward() : nullptr;
	if (UImage* Coin = Cast<UImage>(GetWidgetFromName(TEXT("SettlementGoldCoin"))))
	{
		Coin->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UTextBlock* GoldText = Cast<UTextBlock>(GetWidgetFromName(TEXT("SettlementGoldGain"))))
	{
		GoldText->SetVisibility(ESlateVisibility::Collapsed);
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (Reward != nullptr && Reward->mMercenaryExp.IsValidIndex(Index))
		{
			RefreshMercenaryRow(Index, Reward->mMercenaryExp[Index], Reward->mExpGained);
		}
		else if (UWidget* Row = FindIndexedWidget<UWidget>(this, TEXT("SettlementExpRow"), Index))
		{
			Row->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void URewardSettlementWidgetBase::BuildChestStep()
{
	if (UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("SettlementStepSwitcher"))))
	{
		StepSwitcher->SetActiveWidgetIndex(1);
	}
	if (UWidget* Hint = GetWidgetFromName(TEXT("SettlementChestHint")))
	{
		Hint->SetVisibility(mChestOpening
			? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (UButton* ChestButton = Cast<UButton>(GetWidgetFromName(TEXT("SettlementChestButton"))))
	{
		ChestButton->SetIsEnabled(!mChestOpening);
	}
}

void URewardSettlementWidgetBase::BuildGoldStep()
{
	if (UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("SettlementStepSwitcher"))))
	{
		StepSwitcher->SetActiveWidgetIndex(2);
	}
	const FRewardUI* Reward = mUIModel != nullptr ? &mUIModel->GetReward() : nullptr;
	const int32 Gold = Reward != nullptr ? FMath::Max(0, Reward->mGoldGained) : 0;
	if (UWidget* GoldPanel = GetWidgetFromName(TEXT("SettlementGuaranteedGoldPanel")))
	{
		GoldPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (UImage* Coin = Cast<UImage>(GetWidgetFromName(TEXT("SettlementGoldCoin"))))
	{
		Coin->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (UTextBlock* GoldText = Cast<UTextBlock>(GetWidgetFromName(TEXT("SettlementGoldGain"))))
	{
		GoldText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		GoldText->SetText(FText::Format(LOCTEXT("GoldGain", "+{0} G"),
			FText::AsNumber(Gold)));
	}
}

void URewardSettlementWidgetBase::ResetChestPresentation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mChestOpenTimer);
		World->GetTimerManager().ClearTimer(mChestRevealTimer);
		World->GetTimerManager().ClearTimer(mChestAnimationTimer);
		World->GetTimerManager().ClearTimer(mChoiceAnimationTimer);
	}
	mChestOpening = false;
	mRewardRevealPlaying = false;
	mChestAnimationElapsed = 0.f;
	mChoiceAnimationElapsed = 0.f;
	SetImageTexture(Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestArt"))),
		mChestClosedTexture.Get());
	if (UWidget* ChestFit = GetWidgetFromName(TEXT("SettlementChestFit")))
	{
		ChestFit->SetRenderTransform(FWidgetTransform());
		ChestFit->SetRenderTransformPivot(FVector2D(.5f, .5f));
		ChestFit->SetRenderOpacity(1.f);
	}
	if (UImage* Aura = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestAura"))))
	{
		SetImageTexture(Aura, mChestRevealAuraTexture.Get());
		Aura->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Aura->SetRenderTransform(FWidgetTransform());
		Aura->SetRenderTransformPivot(FVector2D(.5f, .5f));
		Aura->SetRenderOpacity(.18f);
	}
	if (UImage* Burst = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestBurst"))))
	{
		Burst->SetVisibility(ESlateVisibility::Collapsed);
		Burst->SetRenderTransform(FWidgetTransform());
		Burst->SetRenderTransformPivot(FVector2D(.5f, .5f));
		Burst->SetRenderOpacity(0.f);
	}
}

void URewardSettlementWidgetBase::HandleChestClicked()
{
	if (mCurrentStep != 2 || mChestOpening)
	{
		return;
	}
	mChestOpening = true;
	mChestAnimationElapsed = 0.f;
	BuildChestStep();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(mChestAnimationTimer, this,
			&URewardSettlementWidgetBase::TickChestReveal, 1.f / 60.f, true);
		TickChestReveal();
	}
	else
	{
		AdvanceChestToOpen();
		FinishChestReveal();
	}
}

void URewardSettlementWidgetBase::TickChestReveal()
{
	if (!mChestOpening || mCurrentStep != 2)
	{
		return;
	}
	const float Delta = GetWorld() != nullptr
		? FMath::Clamp(GetWorld()->GetDeltaSeconds(), 1.f / 120.f, 1.f / 30.f)
		: 1.f / 60.f;
	mChestAnimationElapsed += Delta;
	const float Time = mChestAnimationElapsed;
	UWidget* ChestFit = GetWidgetFromName(TEXT("SettlementChestFit"));
	UImage* ChestArt = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestArt")));
	UImage* Aura = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestAura")));
	UImage* Burst = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestBurst")));
	FWidgetTransform ChestTransform;

	// 1) anticipation: 터치 직후 짧고 무거운 눌림만 준다.
	if (Time < .12f)
	{
		const float Alpha = FMath::SmoothStep(0.f, 1.f, Time / .12f);
		ChestTransform.Scale = FVector2D(
			FMath::Lerp(1.f, 1.045f, Alpha), FMath::Lerp(1.f, .91f, Alpha));
		ChestTransform.Translation = FVector2D(0.f, FMath::Lerp(0.f, 12.f, Alpha));
	}
	// 2) 잠금 해제: 긴 흔들기 대신 2회만 빠르게 저항한다.
	else if (Time < .36f)
	{
		SetImageTexture(ChestArt, mChestHalfOpenTexture.Get());
		const float Alpha = (Time - .12f) / .24f;
		const float Envelope = 1.f - Alpha;
		const float Shake = FMath::Sin(Alpha * UE_TWO_PI * 2.f) * 10.f * Envelope;
		ChestTransform.Translation = FVector2D(Shake, FMath::Lerp(12.f, 0.f, Alpha));
		ChestTransform.Angle = Shake * .09f;
		ChestTransform.Scale = FVector2D(1.025f, FMath::Lerp(.93f, 1.02f, Alpha));
	}
	// 3) impact + settle: 개방 충격 뒤 반드시 열린 상자가 읽히는 여운을 둔다.
	else
	{
		SetImageTexture(ChestArt, mChestOpenTexture.Get());
		const float Impact = FMath::Clamp((Time - .36f) / .18f, 0.f, 1.f);
		const float Settle = FMath::Clamp((Time - .54f) / .45f, 0.f, 1.f);
		const float SettleEase = 1.f - FMath::Pow(1.f - Settle, 3.f);
		const float ImpactScale = FMath::Lerp(1.19f, 1.08f, Impact);
		const float FinalScale = FMath::Lerp(ImpactScale, 1.f, SettleEase);
		ChestTransform.Scale = FVector2D(FinalScale, FinalScale);
		ChestTransform.Translation = FVector2D(0.f,
			FMath::Lerp(-24.f, 0.f, SettleEase));
		if (Aura != nullptr)
		{
			Aura->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Aura->SetRenderOpacity(FMath::Min(1.f, Impact * 3.f)
				* FMath::Lerp(1.f, .56f, Settle));
			FWidgetTransform AuraTransform;
			const float AuraScale = FMath::Lerp(.24f, 1.12f,
				FMath::Max(Impact, SettleEase));
			AuraTransform.Scale = FVector2D(AuraScale, AuraScale);
			AuraTransform.Angle = FMath::Lerp(-5.f, 3.f, Settle);
			Aura->SetRenderTransform(AuraTransform);
		}
		if (Burst != nullptr)
		{
			Burst->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			const float Flash = FMath::Clamp(1.f - FMath::Abs(Impact - .42f) / .42f, 0.f, 1.f);
			Burst->SetRenderOpacity(Flash);
			FWidgetTransform BurstTransform;
			const float BurstScale = FMath::Lerp(.30f, 1.18f, Impact);
			BurstTransform.Scale = FVector2D(BurstScale, BurstScale);
			Burst->SetRenderTransform(BurstTransform);
		}
	}
	if (ChestFit != nullptr)
	{
		ChestFit->SetRenderTransform(ChestTransform);
	}
	if (Time >= 1.08f)
	{
		FinishChestReveal();
	}
}

void URewardSettlementWidgetBase::AdvanceChestToOpen()
{
	SetImageTexture(Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestArt"))),
		mChestOpenTexture.Get());
	if (UImage* Burst = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestBurst"))))
	{
		Burst->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Burst->SetRenderOpacity(1.f);
	}
	if (UImage* Aura = Cast<UImage>(GetWidgetFromName(TEXT("SettlementChestAura"))))
	{
		Aura->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Aura->SetRenderOpacity(1.f);
	}
}

void URewardSettlementWidgetBase::FinishChestReveal()
{
	if (mCurrentStep != 2)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mChestAnimationTimer);
	}
	mCurrentStep = 3;
	mChestOpening = false;
	RebuildStep();
	BeginChoiceReveal();
}

void URewardSettlementWidgetBase::BeginChoiceReveal()
{
	mRewardRevealPlaying = true;
	mChoiceAnimationElapsed = 0.f;
	const bool bGoldStep = mCurrentStep == 3;
	if (bGoldStep)
	{
		if (UWidget* GoldPanel = GetWidgetFromName(TEXT("SettlementGuaranteedGoldPanel")))
		{
			GoldPanel->SetRenderOpacity(0.f);
			FWidgetTransform Transform;
			Transform.Translation = FVector2D(0.f, 52.f);
			Transform.Scale = FVector2D(.72f, .72f);
			GoldPanel->SetRenderTransformPivot(FVector2D(.5f, .5f));
			GoldPanel->SetRenderTransform(Transform);
		}
	}
	if (!bGoldStep)
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (UWidget* Mount = FindIndexedWidget<UWidget>(this, TEXT("SettlementChoiceMount"), Index))
			{
				Mount->SetRenderOpacity(0.f);
				FWidgetTransform Transform;
				Transform.Translation = FVector2D(0.f, 34.f);
				Transform.Scale = FVector2D(.94f, .94f);
				Mount->SetRenderTransformPivot(FVector2D(.5f, .5f));
				Mount->SetRenderTransform(Transform);
			}
		}
	}
	if (UWidget* NextHolder = GetWidgetFromName(TEXT("NextButtonHolder")))
	{
		NextHolder->SetRenderOpacity(0.f);
		FWidgetTransform Transform;
		Transform.Translation = FVector2D(0.f, 30.f);
		NextHolder->SetRenderTransform(Transform);
	}
	if (mNextButton != nullptr)
	{
		mNextButton->SetIsEnabled(false);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(mChoiceAnimationTimer, this,
			&URewardSettlementWidgetBase::TickChoiceReveal, 1.f / 60.f, true);
		TickChoiceReveal();
	}
	else
	{
		FinishChoiceReveal();
	}
}

void URewardSettlementWidgetBase::TickChoiceReveal()
{
	const float Delta = GetWorld() != nullptr
		? FMath::Clamp(GetWorld()->GetDeltaSeconds(), 1.f / 120.f, 1.f / 30.f)
		: 1.f / 60.f;
	mChoiceAnimationElapsed += Delta;
	const bool bGoldStep = mCurrentStep == 3;
	const int32 ChoiceCount = mUIModel != nullptr
		? FMath::Min(mUIModel->GetRewardChoices().Num(), 3) : 0;
	const int32 GoldGained = mUIModel != nullptr
		? FMath::Max(0, mUIModel->GetReward().mGoldGained) : 0;
	const float GoldAlpha = FMath::Clamp(mChoiceAnimationElapsed / .48f, 0.f, 1.f);
	const float GoldEase = 1.f - FMath::Pow(1.f - GoldAlpha, 3.f);
	const float GoldBounce = FMath::Sin(GoldAlpha * PI) * .05f;
	const float GoldScale = FMath::Lerp(.72f, 1.f, GoldEase) + GoldBounce;
	const int32 DisplayGold = FMath::RoundToInt(GoldGained * GoldEase);

	if (bGoldStep)
	{
		if (UWidget* GoldPanel = GetWidgetFromName(TEXT("SettlementGuaranteedGoldPanel")))
		{
			FWidgetTransform Transform;
			Transform.Translation = FVector2D(0.f, FMath::Lerp(52.f, 0.f, GoldEase));
			Transform.Scale = FVector2D(GoldScale, GoldScale);
			GoldPanel->SetRenderTransform(Transform);
			GoldPanel->SetRenderOpacity(GoldAlpha);
		}
		if (UTextBlock* Text = Cast<UTextBlock>(
			GetWidgetFromName(TEXT("SettlementGoldGain"))))
		{
			Text->SetText(FText::Format(LOCTEXT("GoldGainAnimated", "+{0} G"),
				FText::AsNumber(DisplayGold)));
		}
	}

	if (!bGoldStep)
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UWidget* Mount = FindIndexedWidget<UWidget>(this, TEXT("SettlementChoiceMount"), Index);
			if (Mount == nullptr || Mount->GetVisibility() == ESlateVisibility::Collapsed)
			{
				continue;
			}
			const float Alpha = FMath::Clamp(
				(mChoiceAnimationElapsed - .18f - Index * .14f) / .34f, 0.f, 1.f);
			const float Ease = 1.f - FMath::Pow(1.f - Alpha, 3.f);
			const float Overshoot = FMath::Sin(Alpha * PI) * .035f;
			FWidgetTransform Transform;
			Transform.Translation = FVector2D(0.f, FMath::Lerp(34.f, 0.f, Ease));
			const float Scale = FMath::Lerp(.94f, 1.f, Ease) + Overshoot;
			Transform.Scale = FVector2D(Scale, Scale);
			Mount->SetRenderTransform(Transform);
			Mount->SetRenderOpacity(Alpha);
		}
	}
	const float NextStart = bGoldStep
		? .56f : .18f + FMath::Max(0, ChoiceCount - 1) * .14f + .42f;
	const float NextAlpha = FMath::Clamp(
		(mChoiceAnimationElapsed - NextStart) / .28f, 0.f, 1.f);
	if (UWidget* NextHolder = GetWidgetFromName(TEXT("NextButtonHolder")))
	{
		const float Ease = 1.f - FMath::Pow(1.f - NextAlpha, 3.f);
		FWidgetTransform Transform;
		Transform.Translation = FVector2D(0.f, FMath::Lerp(30.f, 0.f, Ease));
		NextHolder->SetRenderTransform(Transform);
		NextHolder->SetRenderOpacity(NextAlpha);
	}
	if (mChoiceAnimationElapsed >= NextStart + .28f)
	{
		FinishChoiceReveal();
	}
}

void URewardSettlementWidgetBase::FinishChoiceReveal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mChoiceAnimationTimer);
	}
	mRewardRevealPlaying = false;
	for (const FName Name : { FName(TEXT("SettlementGuaranteedGoldPanel")),
		FName(TEXT("NextButtonHolder")) })
	{
		if (UWidget* Widget = GetWidgetFromName(Name))
		{
			Widget->SetRenderTransform(FWidgetTransform());
			Widget->SetRenderOpacity(1.f);
		}
	}
	const int32 GoldGained = mUIModel != nullptr
		? FMath::Max(0, mUIModel->GetReward().mGoldGained) : 0;
	if (UTextBlock* Text = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("SettlementGoldGain"))))
	{
		Text->SetText(FText::Format(LOCTEXT("GoldGainFinal", "+{0} G"),
			FText::AsNumber(GoldGained)));
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (UWidget* Mount = FindIndexedWidget<UWidget>(this, TEXT("SettlementChoiceMount"), Index))
		{
			Mount->SetRenderTransform(FWidgetTransform());
		}
	}
	if (mCurrentStep == 4)
	{
		SelectChoice(mSelectedChoice);
	}
	else if (mNextButton != nullptr)
	{
		mNextButton->SetIsEnabled(true);
	}
}

#if WITH_DEV_AUTOMATION_TESTS
void URewardSettlementWidgetBase::CompleteChestRevealForTest()
{
	if (mCurrentStep == 2 && mChestOpening == false)
	{
		HandleChestClicked();
	}
	AdvanceChestToOpen();
	FinishChestReveal();
	FinishChoiceReveal();
}

void URewardSettlementWidgetBase::AdvanceGoldToArtifactForTest()
{
	if (mCurrentStep == 3 && mUIModel != nullptr
		&& mUIModel->GetRewardChoices().Num() > 0)
	{
		ContinueToNext();
		FinishChoiceReveal();
	}
}
#endif

void URewardSettlementWidgetBase::RefreshMercenaryRow(const int32 RowIndex,
	const FRewardMercenaryExpUI& Mercenary, const int32 ExpGained)
{
	UWidget* Row = FindIndexedWidget<UWidget>(this, TEXT("SettlementExpRow"), RowIndex);
	if (Row == nullptr)
	{
		return;
	}
	Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	SetImageTexture(FindIndexedWidget<UImage>(this, TEXT("SettlementPortrait"), RowIndex),
		Mercenary.mPortrait.Get());
	if (UTextBlock* Level = FindIndexedWidget<UTextBlock>(
		this, TEXT("SettlementMercenaryLevel"), RowIndex))
	{
		Level->SetText(FText::Format(LOCTEXT("Level", "Lv.{0}"),
			FText::AsNumber(Mercenary.mLevel)));
	}

	const float Percent = Mercenary.mMaxExp > 0.0f
		? FMath::Clamp(Mercenary.mExpAfter / Mercenary.mMaxExp, 0.0f, 1.0f) : 0.0f;
	UCanvasPanel* FillClip = FindIndexedWidget<UCanvasPanel>(
		this, TEXT("SettlementMercenaryBarClip"), RowIndex);
	UImage* Fill = FindIndexedWidget<UImage>(this, TEXT("SettlementMercenaryBar"), RowIndex);
	UCanvasPanelSlot* ClipSlot = FillClip != nullptr
		? Cast<UCanvasPanelSlot>(FillClip->Slot) : nullptr;
	const UCanvasPanelSlot* FullFillSlot = Fill != nullptr
		? Cast<UCanvasPanelSlot>(Fill->Slot) : nullptr;
	if (ClipSlot != nullptr && FullFillSlot != nullptr)
	{
		FVector2D ClipSize = ClipSlot->GetSize();
		ClipSize.X = FullFillSlot->GetSize().X * Percent;
		ClipSlot->SetSize(ClipSize);
	}

	if (UTextBlock* BarText = FindIndexedWidget<UTextBlock>(
		this, TEXT("SettlementMercenaryBarText"), RowIndex))
	{
		BarText->SetText(FText::Format(LOCTEXT("ExpBar", "{0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Mercenary.mExpAfter)),
			FText::AsNumber(FMath::RoundToInt(Mercenary.mMaxExp))));
	}
	if (UTextBlock* XP = FindIndexedWidget<UTextBlock>(
		this, TEXT("SettlementXPText"), RowIndex))
	{
		XP->SetText(FText::Format(LOCTEXT("XPBadge", "+{0} XP"),
			FText::AsNumber(ExpGained)));
	}
}

void URewardSettlementWidgetBase::BuildChoiceStep()
{
	if (UWidgetSwitcher* StepSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("SettlementStepSwitcher"))))
	{
		StepSwitcher->SetActiveWidgetIndex(3);
	}
	else
	{
		if (UWidget* ResultStep = GetWidgetFromName(TEXT("SettlementResultStep")))
		{
			ResultStep->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* ChoiceStep = GetWidgetFromName(TEXT("SettlementChoiceStep")))
		{
			ChoiceStep->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	const TArray<FRewardChoiceUI> Choices = mUIModel != nullptr
		? mUIModel->GetRewardChoices() : TArray<FRewardChoiceUI>();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UWidget* Mount = FindIndexedWidget<UWidget>(this, TEXT("SettlementChoiceMount"), Index);
		if (Mount == nullptr)
		{
			continue;
		}
		if (Choices.IsValidIndex(Index) == false)
		{
			Mount->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Mount->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const FRewardChoiceUI& Choice = Choices[Index];
		UTexture2D* Icon = Choice.mIcon.Get();
		if (Icon == nullptr)
		{
			switch (Choice.mKind)
			{
			case ERewardChoiceKind::Skill: Icon = mSkillIconTexture.Get(); break;
			case ERewardChoiceKind::Gold: Icon = mGoldIconTexture.Get(); break;
			default: Icon = mEquipmentIconTexture.Get(); break;
			}
		}
		SetImageTexture(FindIndexedWidget<UImage>(this, TEXT("SettlementChoiceIcon"), Index), Icon);
		if (UTextBlock* Name = FindIndexedWidget<UTextBlock>(
			this, TEXT("SettlementChoiceName"), Index))
		{
			Name->SetText(ChoiceFallback(Choice));
		}
		if (UButton* PickButton = FindIndexedWidget<UButton>(
			this, TEXT("SettlementChoiceButton"), Index))
		{
			PickButton->SetIsEnabled(true);
		}
	}
	SelectChoice(mSelectedChoice);
}

void URewardSettlementWidgetBase::SelectChoice(const int32 ChoiceSlot)
{
	const int32 ChoiceCount = mUIModel != nullptr
		? FMath::Min(mUIModel->GetRewardChoices().Num(), 3) : 0;
	mSelectedChoice = ChoiceSlot >= 0 && ChoiceSlot < ChoiceCount
		? ChoiceSlot : INDEX_NONE;
	for (int32 Card = 0; Card < 3; ++Card)
	{
		const bool bSelected = Card == mSelectedChoice;
		const bool bPicked = mSelectedChoice == INDEX_NONE || bSelected;
		if (UImage* SelectedOverlay = FindIndexedWidget<UImage>(
			this, TEXT("SettlementCardSelectedOverlay"), Card))
		{
			SelectedOverlay->SetVisibility(bSelected
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UWidget* Mount = FindIndexedWidget<UWidget>(
			this, TEXT("SettlementChoiceMount"), Card))
		{
			Mount->SetRenderOpacity(bPicked ? 1.0f : 0.55f);
		}
	}
	if (mNextButton != nullptr)
	{
		mNextButton->SetIsEnabled(mCurrentStep == 1 || mCurrentStep == 3
			|| (mCurrentStep == 4 && mSelectedChoice != INDEX_NONE));
	}
}

void URewardSettlementWidgetBase::HandleChoiceClicked_0() { SelectChoice(0); }
void URewardSettlementWidgetBase::HandleChoiceClicked_1() { SelectChoice(1); }
void URewardSettlementWidgetBase::HandleChoiceClicked_2() { SelectChoice(2); }

#undef LOCTEXT_NAMESPACE
