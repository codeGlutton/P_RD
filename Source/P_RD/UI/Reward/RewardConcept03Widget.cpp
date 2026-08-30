#include "UI/Reward/RewardConcept03Widget.h"
#include "UI/DetailOverlayInputShield.h"

#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardUITypes.h"
#include "UI/RunOptionsRailWidget.h"

#define LOCTEXT_NAMESPACE "RewardConcept03Widget"

namespace RewardConcept03
{
	constexpr int32 FirstStep = 0;
	constexpr int32 ChestStep = 1;
	constexpr int32 GoldStep = 2;
	constexpr int32 ArtifactStep = 3;
	constexpr int32 SwitcherWarmupOffset = 1;
	constexpr int32 DefaultArtifact = 1;
	constexpr float ChoicePanelXs[] = { 120.f, 450.f, 780.f };
	constexpr float ChoicePanelWidth = 260.f;
	constexpr float ChoiceListWidth = 1160.f;
	constexpr float ChoiceSpacing = 330.f;
	constexpr int32 TripleBurstFrameCount = 33;
	constexpr int32 TripleBurstAtlasColumns = 6;
	constexpr int32 TripleBurstAtlasRows = 6;
	constexpr float ArtifactLongPressSeconds = .5f;
	constexpr float DefaultChestRevealDuration = 1.15f;
	// 33장을 약 24fps로 한 번만 보여 준다. 기존 4.06초는 보상 흐름을
	// 지나치게 늦추고 밝은 프레임의 중첩을 더 눈에 띄게 했다.
	constexpr float TripleBurstRevealDuration = 1.85f;
	constexpr float GoldRevealDuration = 1.65f;
	constexpr float ExperienceFillDuration = 1.10f;
	constexpr float ExperienceLevelUpPause = .28f;
	constexpr float ChestShakeEnd = .32f;
	constexpr float ArtifactRevealDuration = 1.20f;
	constexpr float ArtifactStagger = .12f;
	const FVector2D ArtifactStartTranslations[] = {
		FVector2D(330.f, 105.f), FVector2D(0.f, 105.f),
		FVector2D(-330.f, 105.f) };
	constexpr float ArtifactStartAngles[] = { -11.f, 0.f, 11.f };

	float GetChoicePanelX(const int32 ChoiceCount, const int32 ChoiceIndex)
	{
		const int32 Count = FMath::Clamp(ChoiceCount, 1,
			UE_ARRAY_COUNT(ChoicePanelXs));
		if (Count == 1)
		{
			return (ChoiceListWidth - ChoicePanelWidth) * .5f;
		}
		if (Count == 2)
		{
			const float FirstCenter = ChoiceListWidth * .5f - ChoiceSpacing * .5f;
			return FirstCenter - ChoicePanelWidth * .5f
				+ ChoiceIndex * ChoiceSpacing;
		}
		return ChoicePanelXs[FMath::Clamp(ChoiceIndex, 0, Count - 1)];
	}

	void SetPortraitCropped(UImage* Image, UTexture2D* Texture)
	{
		if (Image == nullptr || Texture == nullptr)
		{
			return;
		}
		const float Width = static_cast<float>(Texture->GetSizeX());
		const float Height = static_cast<float>(Texture->GetSizeY());
		if (Width <= 0.f || Height <= 0.f)
		{
			Image->SetBrushFromTexture(Texture, false);
			return;
		}
		FSlateBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		if (Width < Height)
		{
			// 캐릭터 원화의 얼굴은 위쪽에 있으므로 정사각 셀에서는 상단을 남긴다.
			Brush.ImageSize = FVector2D(Width, Width);
			Brush.SetUVRegion(FBox2f(FVector2f(0.f, 0.f),
				FVector2f(1.f, Width / Height)));
		}
		else
		{
			const float Margin = (1.f - Height / Width) * .5f;
			Brush.ImageSize = FVector2D(Height, Height);
			Brush.SetUVRegion(FBox2f(FVector2f(Margin, 0.f),
				FVector2f(1.f - Margin, 1.f)));
		}
		Image->SetBrush(Brush);
	}

	float EaseOutBack(const float Time)
	{
		const float T = FMath::Clamp(Time, 0.f, 1.f) - 1.f;
		constexpr float Overshoot = 1.70158f;
		return 1.f + (Overshoot + 1.f) * T * T * T
			+ Overshoot * T * T;
	}

	float Segment(const float Time, const float Start, const float End)
	{
		return FMath::Clamp((Time - Start) / FMath::Max(End - Start, .001f),
			0.f, 1.f);
	}

	float ImpactPulse(const float Time, const float Center, const float HalfWidth)
	{
		return 1.f - FMath::Clamp(FMath::Abs(Time - Center)
			/ FMath::Max(HalfWidth, .001f), 0.f, 1.f);
	}

	void SetAtlasFrame(UImage* Image, const int32 FrameIndex)
	{
		if (Image == nullptr)
		{
			return;
		}
		const int32 Frame = FMath::Clamp(FrameIndex, 0, TripleBurstFrameCount - 1);
		const int32 Column = Frame % TripleBurstAtlasColumns;
		const int32 Row = Frame / TripleBurstAtlasColumns;
		const FVector2f Min(
			static_cast<float>(Column) / TripleBurstAtlasColumns,
			static_cast<float>(Row) / TripleBurstAtlasRows);
		const FVector2f Max(
			static_cast<float>(Column + 1) / TripleBurstAtlasColumns,
			static_cast<float>(Row + 1) / TripleBurstAtlasRows);
		FSlateBrush Brush = Image->GetBrush();
		Brush.SetUVRegion(FBox2f(Min, Max));
		Image->SetBrush(Brush);
	}
}

void URewardConcept03Widget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ResolveWidgets();
	ResetRewardFlow();
}

void URewardConcept03Widget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveWidgets();
	// WBP 에 번역 키 없이(culture-invariant) 구워진 고정 라벨들을 로컬라이즈
	// 텍스트로 갈아 끼운다. 빌더 리베이크에는 이 PC 에 없는 소스 아트가
	// 필요해서(0823), 런타임에서 이름으로 찾아 덮는 쪽이 안전하다.
	{
		const TPair<const TCHAR*, FText> BakedLabels[] = {
			{ TEXT("NewTabText_1"), LOCTEXT("TabExp", "경험치") },
			{ TEXT("NewTabText_2"), LOCTEXT("TabChest", "상자") },
			{ TEXT("NewTabText_3"), LOCTEXT("TabGold", "골드") },
			{ TEXT("NewTabText_4"), LOCTEXT("TabArtifact", "아티팩트") },
			{ TEXT("NewButtonText_1"), LOCTEXT("NextButton", "다음") },
			{ TEXT("NewButtonText_2"), LOCTEXT("NextButton", "다음") },
			{ TEXT("NewButtonText_3"), UsesArtifactStep()
				? LOCTEXT("NextButton", "다음") : LOCTEXT("ConfirmButton", "확정") },
			{ TEXT("NewButtonText_4"), LOCTEXT("ConfirmButton", "확정") },
			{ TEXT("NewExperienceSummaryHeading"), LOCTEXT("ExpHeading", "경험치 획득") },
			{ TEXT("NewChestHeading"), LOCTEXT("ChestHeading", "보상 상자") },
			{ TEXT("NewChestMain"), LOCTEXT("ChestOpenLabel", "상자 열기") },
			{ TEXT("NewChestHint"), LOCTEXT("ChestHint", "상자를 눌러 여세요") },
			{ TEXT("NewGoldHeading"), LOCTEXT("GoldHeading", "획득 골드") },
			{ TEXT("NewGoldHint"), LOCTEXT("GoldGrantedHint", "보상이 지급되었습니다") },
		};
		for (const TPair<const TCHAR*, FText>& Label : BakedLabels)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(Label.Key)))
			{
				Text->SetText(Label.Value);
			}
		}
	}
	BindInput();
	EnsureRunOptionsRail();
	RefreshRewardData();
	// BindUIModel이 Construct보다 먼저 호출되는 일반 CreateWidget 흐름에서도
	// 최종값을 한 프레임 노출한 뒤 시작값으로 되감기지 않게 t=0을 다시 그린다.
	InitializeExperienceAnimation();
	ApplyVisualState();
}

void URewardConcept03Widget::NativeDestruct()
{
	CancelArtifactPress();
	UnbindInput();
	UnbindUIModel();
	ReleaseArtifactDetailOverlay();
	if (RunOptionsRailWidget != nullptr)
	{
		RunOptionsRailWidget->RemoveFromParent();
		RunOptionsRailWidget = nullptr;
	}
	Super::NativeDestruct();
}

void URewardConcept03Widget::BindUIModel(URewardUIModel* InUIModel)
{
	if (UIModel == InUIModel)
	{
		RefreshRewardData();
		InitializeExperienceAnimation();
		return;
	}

	UnbindUIModel();
	UIModel = InUIModel;
	if (UIModel != nullptr)
	{
		UIModel->OnUIChanged.AddUniqueDynamic(
			this, &URewardConcept03Widget::HandleRewardDataChanged);
		UIModel->OnChoicesChanged.AddUniqueDynamic(
			this, &URewardConcept03Widget::HandleRewardDataChanged);
		UIModel->OnRewardSelectionConfirmed.AddUniqueDynamic(
			this, &URewardConcept03Widget::HandleRewardSelectionConfirmed);
		UIModel->OnRewardGrantBundleConfirmed.AddUniqueDynamic(
			this, &URewardConcept03Widget::HandleRewardGrantBundleConfirmed);
	}
	RefreshRewardData();
	InitializeExperienceAnimation();
}

void URewardConcept03Widget::UnbindUIModel()
{
	if (UIModel != nullptr)
	{
		UIModel->OnUIChanged.RemoveDynamic(
			this, &URewardConcept03Widget::HandleRewardDataChanged);
		UIModel->OnChoicesChanged.RemoveDynamic(
			this, &URewardConcept03Widget::HandleRewardDataChanged);
		UIModel->OnRewardSelectionConfirmed.RemoveDynamic(
			this, &URewardConcept03Widget::HandleRewardSelectionConfirmed);
		UIModel->OnRewardGrantBundleConfirmed.RemoveDynamic(
			this, &URewardConcept03Widget::HandleRewardGrantBundleConfirmed);
	}
	UIModel = nullptr;
}

void URewardConcept03Widget::HandleRewardDataChanged()
{
	RefreshRewardData();
	if (CurrentStepIndex == RewardConcept03::FirstStep
		&& !bExperienceClaimRequested)
	{
		InitializeExperienceAnimation();
	}
}

void URewardConcept03Widget::HandleRewardSelectionConfirmed(
	const FPrimaryAssetId RewardId)
{
	if (bRewardRequestPending == false
		|| UIModel == nullptr
		|| UIModel->GetAcquisitionPolicy() != ERewardAcquisitionPolicy::SelectOne
		|| PendingRewardId != RewardId)
	{
		return;
	}

	bRewardRequestPending = false;
	FinishRewardFlowAfterConfirmation();
}

void URewardConcept03Widget::HandleRewardGrantBundleConfirmed(
	const FRewardGrantBundleResultUI Result)
{
	if (bRewardRequestPending == false
		|| UIModel == nullptr
		|| UIModel->GetAcquisitionPolicy() != ERewardAcquisitionPolicy::GrantAll)
	{
		return;
	}

	// 부분 실패도 정책상 bundle 처리는 끝난 것이다. 실제 결과는
	// 게임플레이가 보관하고 UI는 완료 confirmation 이후에만 닫는다.
	bRewardRequestPending = false;
	FinishRewardFlowAfterConfirmation();
}

void URewardConcept03Widget::NativeTick(
	const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bManualPresentationTick)
	{
		UpdateExperienceAnimation(InDeltaTime);
		AdvanceRewardPresentation(InDeltaTime);
	}
}

void URewardConcept03Widget::ResolveWidgets()
{
	StepSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("NewRewardStepSwitcher")));
	ProgressSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("NewRewardProgressSwitcher")));
	TabSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("NewRewardTabSwitcher")));
	ButtonLabelSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("NewRewardButtonSwitcher")));
	ChestVisualSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("NewChestVisualSwitcher")));
	ChestBlendSwitcher = Cast<UWidgetSwitcher>(
		GetWidgetFromName(TEXT("NewChestBlendSwitcher")));
	ChestSequenceImage = Cast<UImage>(
		GetWidgetFromName(TEXT("NewChestSequenceImage")));
	ChestSequenceBlendImage = Cast<UImage>(
		GetWidgetFromName(TEXT("NewChestSequenceBlendImage")));
	BottomActionButton = Cast<UButton>(
		GetWidgetFromName(TEXT("NewBottomActionButton")));
	BottomButtonArt = Cast<UImage>(
		GetWidgetFromName(TEXT("NewBottomButtonArt")));
	BottomButtonPanel = GetWidgetFromName(TEXT("NewBottomButtonPanel"));
	ChestButton = Cast<UButton>(
		GetWidgetFromName(TEXT("NewChestOpenButton")));
	ChestVisualPanel = GetWidgetFromName(TEXT("NewChestVisualPanel"));
	// 아틀라스 셀은 자체적으로 투명 여백을 포함한다. Fit/Panel이 다시
	// 경계를 자르면 빛과 상자 가장자리가 직사각형으로 잘려 보인다.
	UWidget* SequenceWidgets[] = {
		ChestVisualPanel.Get(), static_cast<UWidget*>(ChestSequenceImage.Get()),
		static_cast<UWidget*>(ChestSequenceBlendImage.Get()),
		ChestSequenceImage != nullptr
			? static_cast<UWidget*>(ChestSequenceImage->GetParent()) : nullptr,
		ChestSequenceBlendImage != nullptr
			? static_cast<UWidget*>(ChestSequenceBlendImage->GetParent()) : nullptr };
	for (UWidget* SequenceWidget : SequenceWidgets)
	{
		if (SequenceWidget != nullptr)
		{
			SequenceWidget->SetClipping(EWidgetClipping::Inherit);
		}
	}
	for (int32 Wave = 0; Wave < 3; ++Wave)
	{
		ChestBurstGlows[Wave] = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChestBurstGlow_%d"), Wave)));
		ChestBurstRings[Wave] = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChestBurstRing_%d"), Wave)));
		ChestBurstRays[Wave] = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChestBurstRays_%d"), Wave)));
		ChestBurstSparks[Wave] = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChestBurstSpark_%d"), Wave)));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ChestBurstForegroundCoins);
		++Index)
	{
		ChestBurstForegroundCoins[Index] = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChestBurstForegroundCoin_%02d"), Index)));
	}
	ChestInfoPanel = GetWidgetFromName(TEXT("NewChestPanel"));
	GoldVisualPanel = GetWidgetFromName(TEXT("NewGoldVisualPanel"));
	GoldBackgroundChestImage = Cast<UImage>(
		GetWidgetFromName(TEXT("NewGoldBackgroundChestImage")));
	GoldChestBlur = Cast<UBackgroundBlur>(
		GetWidgetFromName(TEXT("NewGoldChestBlur")));
	GoldInfoPanel = GetWidgetFromName(TEXT("NewGoldPanel"));
	GoldCoinImage = Cast<UImage>(GetWidgetFromName(TEXT("NewGoldCoinImage")));
	GoldMainText = Cast<UTextBlock>(GetWidgetFromName(TEXT("NewGoldMain")));
	PresentationFlash = GetWidgetFromName(TEXT("NewRewardPresentationFlash"));
	ArtifactButtons[0] = Cast<UButton>(
		GetWidgetFromName(TEXT("NewArtifactChoiceButton_0")));
	ArtifactButtons[1] = Cast<UButton>(
		GetWidgetFromName(TEXT("NewArtifactChoiceButton_1")));
	ArtifactButtons[2] = Cast<UButton>(
		GetWidgetFromName(TEXT("NewArtifactChoiceButton_2")));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArtifactChoicePanels); ++Index)
	{
		ArtifactChoicePanels[Index] = GetWidgetFromName(
			*FString::Printf(TEXT("NewArtifactChoicePanel_%d"), Index));
	}
	SelectionOutline = Cast<UImage>(
		GetWidgetFromName(TEXT("NewChoiceSelection")));
	ChestMainText = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("NewChestMain")));
	ChestHintText = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("NewChestHint")));
	ConfirmButtonText = Cast<UTextBlock>(
		GetWidgetFromName(UsesArtifactStep()
			? TEXT("NewButtonText_4") : TEXT("NewButtonText_3")));
}

void URewardConcept03Widget::RefreshRewardData()
{
	ResolveWidgets();
	if (UIModel == nullptr)
	{
		DisplayedGoldAmount = 350;
		return;
	}

	const FRewardUI& Reward = UIModel->GetReward();
	DisplayedGoldAmount = FMath::Max(0, Reward.mGoldGained);

	if (UTextBlock* Title = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("NewTitleText"))))
	{
		Title->SetText(Reward.mTitle.IsEmpty()
			? LOCTEXT("DefaultRewardTitle", "전투 보상") : Reward.mTitle);
	}
	if (UTextBlock* ExpReward = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("NewExperienceRewardText"))))
	{
		ExpReward->SetText(FText::FromString(FString::Printf(
			TEXT("+%d XP"), FMath::Max(0, Reward.mExpGained))));
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasMercenary = Reward.mMercenaryExp.IsValidIndex(Index);
		if (UWidget* Row = GetWidgetFromName(
			*FString::Printf(TEXT("NewExperienceRow_%d"), Index)))
		{
			Row->SetVisibility(bHasMercenary
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (!bHasMercenary)
		{
			continue;
		}

		const FRewardMercenaryExpUI& Mercenary = Reward.mMercenaryExp[Index];
		UImage* PortraitImage = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewPortraitImage_%d"), Index)));
		UTextBlock* Name = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewPortraitLabel_%d"), Index)));
		if (Mercenary.mPortrait != nullptr && PortraitImage != nullptr)
		{
			RewardConcept03::SetPortraitCropped(
				PortraitImage, Mercenary.mPortrait.Get());
			PortraitImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (Name != nullptr)
			{
				Name->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			if (PortraitImage != nullptr)
			{
				PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (Name != nullptr)
			{
				Name->SetText(Mercenary.mName.IsEmpty()
					? FText::Format(
						LOCTEXT("MercenaryFallbackName", "용병 {0}"), Index + 1)
					: Mercenary.mName);
				Name->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
		if (UTextBlock* Level = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewLevel_%d"), Index))))
		{
			const int32 DisplayLevel = Mercenary.mLevelAfter > 1
				? Mercenary.mLevelAfter : Mercenary.mLevel;
			Level->SetText(FText::FromString(FString::Printf(
				TEXT("Lv.%d"), FMath::Max(1, DisplayLevel))));
		}
		if (UTextBlock* LevelUp = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewLevelUp_%d"), Index))))
		{
			LevelUp->SetVisibility(bExperienceAnimationInitialized
				&& bExperienceLevelUpRevealed[Index]
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UTextBlock* Progress = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewProgress_%d"), Index))))
		{
			Progress->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"),
				Mercenary.mExpAfter, FMath::Max(1.f, Mercenary.mMaxExp))));
		}
		UCanvasPanel* FillClip = Cast<UCanvasPanel>(GetWidgetFromName(
			*FString::Printf(TEXT("NewFillClip_%d"), Index)));
		UImage* FullFill = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewFill_%d"), Index)));
		UCanvasPanelSlot* ClipSlot = FillClip != nullptr
			? Cast<UCanvasPanelSlot>(FillClip->Slot) : nullptr;
		const UCanvasPanelSlot* FullFillSlot = FullFill != nullptr
			? Cast<UCanvasPanelSlot>(FullFill->Slot) : nullptr;
		if (ClipSlot != nullptr && FullFillSlot != nullptr)
		{
			const float Percent = FMath::Clamp(Mercenary.mExpAfter
				/ FMath::Max(1.f, Mercenary.mMaxExp), 0.f, 1.f);
			FVector2D ClipSize = ClipSlot->GetSize();
			ClipSize.X = FullFillSlot->GetSize().X * Percent;
			ClipSlot->SetSize(ClipSize);
		}
	}

	if (GoldMainText != nullptr)
	{
		GoldMainText->SetText(FText::FromString(FString::Printf(
			TEXT("+%d G"), DisplayedGoldAmount)));
	}
	if (UTextBlock* GoldHint = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("NewGoldHint"))))
	{
		GoldHint->SetText(FText::Format(
			LOCTEXT("GoldBalanceHint", "총 보유 {0} G"),
			FMath::Max(0, Reward.mGoldBalance)));
	}

	const TArray<FRewardChoiceUI>& Choices = UIModel->GetRewardChoices();
	const int32 VisibleChoiceCount = FMath::Min(Choices.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(ArtifactChoicePanels)));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasChoice = Choices.IsValidIndex(Index);
		if (UWidget* Panel = ArtifactChoicePanels[Index])
		{
			Panel->SetVisibility(bHasChoice
				? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (bHasChoice)
			{
				if (UCanvasPanelSlot* PanelSlot =
					Cast<UCanvasPanelSlot>(Panel->Slot))
				{
					FVector2D Position = PanelSlot->GetPosition();
					Position.X = RewardConcept03::GetChoicePanelX(
						VisibleChoiceCount, Index);
					PanelSlot->SetPosition(Position);
				}
			}
		}
		if (!bHasChoice)
		{
			continue;
		}
		if (UTextBlock* Name = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChoiceName_%d"), Index))))
		{
			Name->SetText(Choices[Index].mName);
		}
		if (UTextBlock* Type = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewChoiceType_%d"), Index))))
		{
			Type->SetText(GetRewardChoiceTypeText(Index));
		}
	}

	if (UsesArtifactStep() && Choices.Num() > 0
		&& UIModel != nullptr
		&& UIModel->GetAcquisitionPolicy() == ERewardAcquisitionPolicy::SelectOne
		&& !Choices.IsValidIndex(SelectedArtifactIndex))
	{
		SelectedArtifactIndex = FMath::Min(1, Choices.Num() - 1);
	}
	if (UIModel != nullptr
		&& UIModel->GetAcquisitionPolicy() != ERewardAcquisitionPolicy::SelectOne)
	{
		SelectedArtifactIndex = INDEX_NONE;
	}
	ApplyArtifactSelection();
}

void URewardConcept03Widget::InitializeExperienceAnimation()
{
	if (UIModel == nullptr)
	{
		bExperienceAnimationInitialized = false;
		return;
	}
	const FRewardUI& Reward = UIModel->GetReward();
	ExperienceAnimationElapsed = 0.f;
	bExperienceAnimationInitialized = true;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		ExperienceSteps[Index].Reset();
		bExperienceLevelUpRevealed[Index] = false;
		if (Reward.mMercenaryExp.IsValidIndex(Index))
		{
			const FRewardMercenaryExpUI& Mercenary = Reward.mMercenaryExp[Index];
			ExperienceSteps[Index] = Mercenary.mProgressSteps;
			if (ExperienceSteps[Index].IsEmpty())
			{
				// 구형/Blueprint 입력은 기존 단일 진행도 필드로 한 구간을 만든다.
				FRewardExpProgressStepUI& Step =
					ExperienceSteps[Index].AddDefaulted_GetRef();
				const int32 LegacyLevel = FMath::Max(1, Mercenary.mLevel);
				const bool bHasExplicitLevelRange = Mercenary.mLevelBefore > 0
					&& Mercenary.mLevelAfter >= Mercenary.mLevelBefore;
				Step.mLevelBefore = bHasExplicitLevelRange
					? Mercenary.mLevelBefore : LegacyLevel;
				Step.mLevelAfter = bHasExplicitLevelRange
					? Mercenary.mLevelAfter : LegacyLevel;
				Step.mExpBefore = FMath::Max(0.f, Mercenary.mExpBefore);
				Step.mExpAfter = FMath::Max(0.f, Mercenary.mExpAfter);
				Step.mMaxExp = FMath::Max(1.f, Mercenary.mMaxExp);
			}
		}
	}
	UpdateExperienceAnimation(0.f);
}

void URewardConcept03Widget::UpdateExperienceAnimation(const float DeltaSeconds)
{
	if (!bExperienceAnimationInitialized
		|| CurrentStepIndex != RewardConcept03::FirstStep)
	{
		return;
	}
	ExperienceAnimationElapsed += FMath::Max(0.f, DeltaSeconds);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const TArray<FRewardExpProgressStepUI>& Steps = ExperienceSteps[Index];
		if (Steps.IsEmpty())
		{
			continue;
		}

		float RemainingTime = ExperienceAnimationElapsed;
		const FRewardExpProgressStepUI* VisibleStep = &Steps.Last();
		float VisibleExp = FMath::Max(0.f, VisibleStep->mExpAfter);
		int32 VisibleLevel = FMath::Max(1, VisibleStep->mLevelAfter);
		bool bLevelUpVisible = false;
		int32 LevelUpCount = 0;
		for (const FRewardExpProgressStepUI& Step : Steps)
		{
			if (RemainingTime <= RewardConcept03::ExperienceFillDuration)
			{
				const float LinearT = FMath::Clamp(RemainingTime
					/ RewardConcept03::ExperienceFillDuration, 0.f, 1.f);
				const float T = 1.f - FMath::Pow(1.f - LinearT, 3.f);
				VisibleStep = &Step;
				VisibleExp = FMath::Lerp(FMath::Max(0.f, Step.mExpBefore),
					FMath::Max(0.f, Step.mExpAfter), T);
				VisibleLevel = LinearT >= 1.f && Step.IsLevelUp()
					? FMath::Max(1, Step.mLevelAfter)
					: FMath::Max(1, Step.mLevelBefore);
				if (LinearT >= 1.f && Step.IsLevelUp())
				{
					LevelUpCount += Step.mLevelAfter - Step.mLevelBefore;
					bLevelUpVisible = true;
				}
				break;
			}

			RemainingTime -= RewardConcept03::ExperienceFillDuration;
			VisibleStep = &Step;
			VisibleExp = FMath::Max(0.f, Step.mExpAfter);
			VisibleLevel = FMath::Max(1, Step.mLevelAfter);
			if (Step.IsLevelUp())
			{
				LevelUpCount += Step.mLevelAfter - Step.mLevelBefore;
				bLevelUpVisible = true;
				if (RemainingTime <= RewardConcept03::ExperienceLevelUpPause)
				{
					break;
				}
				RemainingTime -= RewardConcept03::ExperienceLevelUpPause;
			}
		}

		bExperienceLevelUpRevealed[Index] = bLevelUpVisible;
		if (UTextBlock* Level = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewLevel_%d"), Index))))
		{
			Level->SetText(FText::Format(LOCTEXT("ExperienceLevel", "Lv.{0}"),
				FText::AsNumber(VisibleLevel)));
		}
		if (UTextBlock* LevelUp = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewLevelUp_%d"), Index))))
		{
			LevelUp->SetVisibility(bLevelUpVisible
				? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			LevelUp->SetText(LevelUpCount > 1
				? FText::Format(LOCTEXT("MultipleLevelUp", "레벨 업! ×{0}"),
					FText::AsNumber(LevelUpCount))
				: LOCTEXT("LevelUp", "레벨 업!"));
		}
		const float Maximum = FMath::Max(1.f, VisibleStep->mMaxExp);
		if (UTextBlock* Progress = Cast<UTextBlock>(GetWidgetFromName(
			*FString::Printf(TEXT("NewProgress_%d"), Index))))
		{
			Progress->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"),
				VisibleExp, Maximum)));
		}
		UCanvasPanel* FillClip = Cast<UCanvasPanel>(GetWidgetFromName(
			*FString::Printf(TEXT("NewFillClip_%d"), Index)));
		UImage* FullFill = Cast<UImage>(GetWidgetFromName(
			*FString::Printf(TEXT("NewFill_%d"), Index)));
		UCanvasPanelSlot* ClipSlot = FillClip != nullptr
			? Cast<UCanvasPanelSlot>(FillClip->Slot) : nullptr;
		const UCanvasPanelSlot* FullFillSlot = FullFill != nullptr
			? Cast<UCanvasPanelSlot>(FullFill->Slot) : nullptr;
		if (ClipSlot != nullptr && FullFillSlot != nullptr)
		{
			FVector2D ClipSize = ClipSlot->GetSize();
			ClipSize.X = FullFillSlot->GetSize().X * FMath::Clamp(VisibleExp
				/ Maximum, 0.f, 1.f);
			ClipSlot->SetSize(ClipSize);
		}
	}
}

FText URewardConcept03Widget::GetRewardChoiceTypeText(
	const int32 ChoiceIndex) const
{
	if (UIModel == nullptr
		|| !UIModel->GetRewardChoices().IsValidIndex(ChoiceIndex))
	{
		return LOCTEXT("ChoiceTypeArtifact", "아티팩트");
	}

	switch (UIModel->GetRewardChoices()[ChoiceIndex].mKind)
	{
	case ERewardChoiceKind::Skill:
		return LOCTEXT("ChoiceTypeSkill", "스킬");
	case ERewardChoiceKind::Gold:
		return LOCTEXT("ChoiceTypeGold", "골드");
	case ERewardChoiceKind::Artifact:
		return LOCTEXT("ChoiceTypeArtifact", "아티팩트");
	default:
		return LOCTEXT("ChoiceTypeArtifact", "아티팩트");
	}
}

void URewardConcept03Widget::BindInput()
{
	UnbindInput();
	if (BottomActionButton != nullptr)
	{
		BottomActionButton->OnClicked.AddDynamic(
			this, &URewardConcept03Widget::HandleBottomActionClicked);
		BottomActionButton->OnHovered.AddDynamic(
			this, &URewardConcept03Widget::HandleBottomActionHovered);
		BottomActionButton->OnUnhovered.AddDynamic(
			this, &URewardConcept03Widget::HandleBottomActionUnhovered);
		BottomActionButton->OnPressed.AddDynamic(
			this, &URewardConcept03Widget::HandleBottomActionPressed);
		BottomActionButton->OnReleased.AddDynamic(
			this, &URewardConcept03Widget::HandleBottomActionReleased);
	}
	if (ChestButton != nullptr)
	{
		ChestButton->OnClicked.AddDynamic(
			this, &URewardConcept03Widget::HandleChestClicked);
	}
	if (ArtifactButtons[0] != nullptr)
	{
		ArtifactButtons[0]->OnClicked.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifact0Clicked);
		ArtifactButtons[0]->OnPressed.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifact0Pressed);
		ArtifactButtons[0]->OnReleased.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifactReleased);
	}
	if (ArtifactButtons[1] != nullptr)
	{
		ArtifactButtons[1]->OnClicked.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifact1Clicked);
		ArtifactButtons[1]->OnPressed.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifact1Pressed);
		ArtifactButtons[1]->OnReleased.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifactReleased);
	}
	if (ArtifactButtons[2] != nullptr)
	{
		ArtifactButtons[2]->OnClicked.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifact2Clicked);
		ArtifactButtons[2]->OnPressed.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifact2Pressed);
		ArtifactButtons[2]->OnReleased.AddDynamic(
			this, &URewardConcept03Widget::HandleArtifactReleased);
	}
}

void URewardConcept03Widget::UnbindInput()
{
	CancelArtifactPress();
	if (BottomActionButton != nullptr)
	{
		BottomActionButton->OnClicked.RemoveDynamic(
			this, &URewardConcept03Widget::HandleBottomActionClicked);
		BottomActionButton->OnHovered.RemoveDynamic(
			this, &URewardConcept03Widget::HandleBottomActionHovered);
		BottomActionButton->OnUnhovered.RemoveDynamic(
			this, &URewardConcept03Widget::HandleBottomActionUnhovered);
		BottomActionButton->OnPressed.RemoveDynamic(
			this, &URewardConcept03Widget::HandleBottomActionPressed);
		BottomActionButton->OnReleased.RemoveDynamic(
			this, &URewardConcept03Widget::HandleBottomActionReleased);
	}
	if (ChestButton != nullptr)
	{
		ChestButton->OnClicked.RemoveDynamic(
			this, &URewardConcept03Widget::HandleChestClicked);
	}
	if (ArtifactButtons[0] != nullptr)
	{
		ArtifactButtons[0]->OnClicked.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifact0Clicked);
		ArtifactButtons[0]->OnPressed.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifact0Pressed);
		ArtifactButtons[0]->OnReleased.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifactReleased);
	}
	if (ArtifactButtons[1] != nullptr)
	{
		ArtifactButtons[1]->OnClicked.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifact1Clicked);
		ArtifactButtons[1]->OnPressed.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifact1Pressed);
		ArtifactButtons[1]->OnReleased.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifactReleased);
	}
	if (ArtifactButtons[2] != nullptr)
	{
		ArtifactButtons[2]->OnClicked.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifact2Clicked);
		ArtifactButtons[2]->OnPressed.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifact2Pressed);
		ArtifactButtons[2]->OnReleased.RemoveDynamic(
			this, &URewardConcept03Widget::HandleArtifactReleased);
	}
}

void URewardConcept03Widget::ResetRewardFlow()
{
	CancelArtifactPress();
	bSuppressNextArtifactClick = false;
	CurrentStepIndex = RewardConcept03::FirstStep;
	const int32 ChoiceCount = UIModel != nullptr
		? UIModel->GetRewardChoices().Num() : 3;
	SelectedArtifactIndex = UsesArtifactStep() && ChoiceCount > 0
		&& (UIModel == nullptr
			|| UIModel->GetAcquisitionPolicy() == ERewardAcquisitionPolicy::SelectOne)
		? FMath::Min(RewardConcept03::DefaultArtifact, ChoiceCount - 1)
		: INDEX_NONE;
	bChestOpened = false;
	bFlowCompleted = false;
	bExperienceClaimRequested = false;
	bGoldClaimRequested = false;
	bRewardRequestPending = false;
	PendingRewardId = FPrimaryAssetId();
	PresentationState = EPresentationState::Idle;
	PresentationElapsed = 0.f;
	bExperienceAnimationInitialized = false;
	ExperienceAnimationElapsed = 0.f;
	HideArtifactDetails();
	RefreshRewardData();
	InitializeExperienceAnimation();
	ResetPresentationVisuals();
	ApplyVisualState();
}

void URewardConcept03Widget::AdvanceRewardFlow()
{
	if (bFlowCompleted || bRewardRequestPending)
	{
		return;
	}

	switch (CurrentStepIndex)
	{
	case RewardConcept03::FirstStep:
		ClaimExperienceReward();
		PresentationState = EPresentationState::ChestAwaitInput;
		SetCurrentStep(RewardConcept03::ChestStep);
		break;
	case RewardConcept03::GoldStep:
		if (PresentationState == EPresentationState::AwaitGoldContinue)
		{
			if (UsesArtifactStep())
			{
				StartArtifactReveal();
			}
			else
			{
				CompleteRewardFlow();
			}
		}
		else if (PresentationState == EPresentationState::AwaitConfirm)
		{
			CompleteRewardFlow();
		}
		break;
	case RewardConcept03::ArtifactStep:
		if (PresentationState == EPresentationState::AwaitArtifactChoice
			&& UIModel != nullptr
			&& (UIModel->GetAcquisitionPolicy() == ERewardAcquisitionPolicy::GrantAll
				|| SelectedArtifactIndex != INDEX_NONE))
		{
			CompleteRewardFlow();
		}
		break;
	default:
		break;
	}
}

void URewardConcept03Widget::OpenRewardChest()
{
	if (CurrentStepIndex != RewardConcept03::ChestStep
		|| PresentationState != EPresentationState::ChestAwaitInput)
	{
		return;
	}
	StartChestOpening();
}

bool URewardConcept03Widget::IsRewardPresentationPlaying() const
{
	return PresentationState == EPresentationState::ChestOpening
		|| PresentationState == EPresentationState::GoldReveal
		|| PresentationState == EPresentationState::ArtifactReveal;
}

void URewardConcept03Widget::AdvanceRewardPresentation(const float DeltaSeconds)
{
	if (!IsRewardPresentationPlaying() || DeltaSeconds <= 0.f)
	{
		return;
	}

	PresentationElapsed += DeltaSeconds;
	switch (PresentationState)
	{
	case EPresentationState::ChestOpening:
	{
		const bool bUsesTripleBurstFrames = ChestSequenceImage != nullptr
			|| (ChestVisualSwitcher != nullptr
			&& ChestVisualSwitcher->GetNumWidgets()
				== RewardConcept03::TripleBurstFrameCount);
		const float Duration = bUsesTripleBurstFrames
			? RewardConcept03::TripleBurstRevealDuration
			: RewardConcept03::DefaultChestRevealDuration;
		UpdateChestOpening(PresentationElapsed / Duration);
		if (PresentationElapsed >= Duration)
		{
			FinishChestOpening();
		}
		break;
	}
	case EPresentationState::GoldReveal:
		UpdateGoldReveal(PresentationElapsed / RewardConcept03::GoldRevealDuration);
		if (PresentationElapsed >= RewardConcept03::GoldRevealDuration)
		{
			FinishGoldReveal();
		}
		break;
	case EPresentationState::ArtifactReveal:
		UpdateArtifactReveal(
			PresentationElapsed / RewardConcept03::ArtifactRevealDuration);
		if (PresentationElapsed >= RewardConcept03::ArtifactRevealDuration)
		{
			FinishArtifactReveal();
		}
		break;
	default:
		break;
	}
}

void URewardConcept03Widget::SkipRewardPresentation()
{
	switch (PresentationState)
	{
	case EPresentationState::ChestOpening:
		FinishChestOpening();
		break;
	case EPresentationState::GoldReveal:
		FinishGoldReveal();
		break;
	case EPresentationState::ArtifactReveal:
		FinishArtifactReveal();
		break;
	default:
		break;
	}
}

void URewardConcept03Widget::StartChestOpening()
{
	bChestOpened = true;
	ResolveWidgets();
	PresentationState = EPresentationState::ChestOpening;
	PresentationElapsed = 0.f;
	ApplyVisualState();
	UpdateChestOpening(0.f);
}

void URewardConcept03Widget::UpdateChestOpening(const float NormalizedTime)
{
	const float T = FMath::Clamp(NormalizedTime, 0.f, 1.f);
	const bool bUsesAtlas = ChestSequenceImage != nullptr;
	const bool bUsesTripleBurstFrames = bUsesAtlas || (ChestVisualSwitcher != nullptr
		&& ChestVisualSwitcher->GetNumWidgets()
			== RewardConcept03::TripleBurstFrameCount);
	const float SequenceT = bUsesTripleBurstFrames
		? RewardConcept03::Segment(T, RewardConcept03::ChestShakeEnd, 1.f) : T;
	const float FramePosition = SequenceT * static_cast<float>(
		RewardConcept03::TripleBurstFrameCount - 1);
	if (bUsesAtlas)
	{
		ChestSequenceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (ChestSequenceBlendImage != nullptr)
		{
			ChestSequenceBlendImage->SetVisibility(ESlateVisibility::Collapsed);
			ChestSequenceBlendImage->SetRenderOpacity(0.f);
		}
		if (ChestVisualSwitcher != nullptr)
		{
			ChestVisualSwitcher->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (ChestBlendSwitcher != nullptr)
		{
			ChestBlendSwitcher->SetVisibility(ESlateVisibility::Collapsed);
		}
		const int32 Frame = FMath::Clamp(FMath::FloorToInt(FramePosition),
			0, RewardConcept03::TripleBurstFrameCount - 1);
		RewardConcept03::SetAtlasFrame(ChestSequenceImage, Frame);
		ChestSequenceImage->SetRenderOpacity(1.f);
	}
	else if (ChestVisualSwitcher != nullptr)
	{
		ChestVisualSwitcher->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (ChestBlendSwitcher != nullptr)
		{
			ChestBlendSwitcher->SetVisibility(ESlateVisibility::Collapsed);
			ChestBlendSwitcher->SetRenderOpacity(0.f);
		}
		if (ChestSequenceImage != nullptr)
		{
			ChestSequenceImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (ChestSequenceBlendImage != nullptr)
		{
			ChestSequenceBlendImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		const int32 Frame = bUsesTripleBurstFrames
			? FMath::Clamp(FMath::FloorToInt(FramePosition),
				0, RewardConcept03::TripleBurstFrameCount - 1)
			: T < .24f ? 0 : T < .39f ? 1
				: T < .54f ? 2 : T < .69f ? 3 : 4;
		ChestVisualSwitcher->SetActiveWidgetIndex(Frame);
		ChestVisualSwitcher->SetRenderOpacity(1.f);
		if (ChestBlendSwitcher != nullptr)
		{
			ChestBlendSwitcher->SetRenderOpacity(0.f);
		}
	}
	if (ChestVisualSwitcher != nullptr || ChestSequenceImage != nullptr)
	{
		float Scale = 1.f;
		float Shake = 0.f;
		float VerticalKick = 0.f;
		if (bUsesTripleBurstFrames)
		{
			// Hold the closed frame while the shake builds, then release the atlas in
			// one clear impact. The previous sequence began opening immediately.
			const float ShakeT = RewardConcept03::Segment(T, 0.f,
				RewardConcept03::ChestShakeEnd);
			if (T < RewardConcept03::ChestShakeEnd)
			{
				const float Amplitude = FMath::Lerp(2.f, 15.f, ShakeT * ShakeT);
				Shake = FMath::Sin(T * 155.f) * Amplitude;
				VerticalKick = -FMath::Abs(FMath::Sin(T * 77.5f))
					* FMath::Lerp(0.f, 5.f, ShakeT);
				Scale = FMath::Lerp(.94f, .89f, ShakeT);
			}
			else
			{
				const float BangT = RewardConcept03::Segment(T,
					RewardConcept03::ChestShakeEnd,
					RewardConcept03::ChestShakeEnd + .16f);
				Scale = FMath::Lerp(1.08f, .92f, BangT);
				VerticalKick = FMath::Lerp(-18.f, 0.f, BangT);
			}
		}
		else
		{
			const float Press = RewardConcept03::Segment(T, 0.f, .14f);
			const float Release = RewardConcept03::Segment(T, .14f, .32f);
			Scale = T < .14f
				? FMath::Lerp(1.f, .92f, Press)
				: FMath::Lerp(.92f, 1.08f,
					RewardConcept03::EaseOutBack(Release));
			Shake = T < .62f
				? FMath::Sin(T * 78.f) * (1.f - T / .62f) * 8.f : 0.f;
		}
		UWidget* SequenceLayers[] = {
			bUsesAtlas ? static_cast<UWidget*>(ChestSequenceImage.Get())
				: static_cast<UWidget*>(ChestVisualSwitcher.Get()),
			bUsesAtlas ? static_cast<UWidget*>(ChestSequenceBlendImage.Get())
				: static_cast<UWidget*>(ChestBlendSwitcher.Get()) };
		for (UWidget* SequenceLayer : SequenceLayers)
		{
			if (SequenceLayer != nullptr)
			{
				SequenceLayer->SetRenderTransformPivot(FVector2D(.5f, .5f));
				SequenceLayer->SetRenderScale(FVector2D(Scale));
				SequenceLayer->SetRenderTranslation(FVector2D(Shake, VerticalKick));
			}
		}
	}
	// Atlas 자체에 빛과 코인이 들어 있으므로 모든 UMG 보조광을 끈다.
	for (int32 Wave = 0; Wave < 3; ++Wave)
	{
		if (UImage* Glow = ChestBurstGlows[Wave])
		{
			Glow->SetVisibility(ESlateVisibility::Collapsed);
			Glow->SetRenderOpacity(0.f);
		}
		if (UImage* Ring = ChestBurstRings[Wave])
		{
			Ring->SetVisibility(ESlateVisibility::Collapsed);
			Ring->SetRenderOpacity(0.f);
		}
		if (UImage* Rays = ChestBurstRays[Wave])
		{
			Rays->SetVisibility(ESlateVisibility::Collapsed);
			Rays->SetRenderOpacity(0.f);
		}
		if (UImage* Spark = ChestBurstSparks[Wave])
		{
			Spark->SetVisibility(ESlateVisibility::Collapsed);
			Spark->SetRenderOpacity(0.f);
		}
	}
	for (UImage* Coin : ChestBurstForegroundCoins)
	{
		if (Coin != nullptr)
		{
			Coin->SetVisibility(ESlateVisibility::Collapsed);
			Coin->SetRenderOpacity(0.f);
		}
	}
	if (ChestInfoPanel != nullptr)
	{
		ChestInfoPanel->SetRenderOpacity(1.f - RewardConcept03::Segment(T, 0.f, .2f));
	}
	if (PresentationFlash != nullptr)
	{
		const float Flash = T < .58f ? 0.f
				: T < .70f ? RewardConcept03::Segment(T, .58f, .70f)
				: 1.f - RewardConcept03::Segment(T, .70f, .94f);
		// Frameless 연출은 전체 화면 사각 플래시를 사용하지 않는다.
		const float AtlasBang = bUsesTripleBurstFrames
			? RewardConcept03::ImpactPulse(T,
				RewardConcept03::ChestShakeEnd + .035f, .055f) : 0.f;
		PresentationFlash->SetRenderOpacity(
			bUsesTripleBurstFrames ? AtlasBang * .70f : Flash * .72f);
	}
}

void URewardConcept03Widget::FinishChestOpening()
{
	UpdateChestOpening(1.f);
	StartGoldReveal();
}

void URewardConcept03Widget::StartGoldReveal()
{
	PresentationState = EPresentationState::GoldReveal;
	PresentationElapsed = 0.f;
	SetCurrentStep(RewardConcept03::GoldStep);
	UpdateGoldReveal(0.f);
}

void URewardConcept03Widget::UpdateGoldReveal(const float NormalizedTime)
{
	const float T = FMath::Clamp(NormalizedTime, 0.f, 1.f);
	if (GoldChestBlur != nullptr && GoldBackgroundChestImage != nullptr)
	{
		const float BlurT = RewardConcept03::Segment(T, 0.f, .30f);
		const float RewardT = RewardConcept03::Segment(T, .10f, .66f);
		const float RewardEase = RewardConcept03::EaseOutBack(RewardT);
		GoldChestBlur->SetBlurStrength(0.f);
		GoldChestBlur->SetRenderOpacity(0.f);
		GoldChestBlur->SetVisibility(ESlateVisibility::Collapsed);

		GoldBackgroundChestImage->SetRenderTransformPivot(FVector2D(.5f, .5f));
		GoldBackgroundChestImage->SetRenderOpacity(
			FMath::Lerp(.96f, .82f, BlurT));
		GoldBackgroundChestImage->SetRenderScale(FVector2D(
			FMath::Lerp(1.f, .98f, BlurT)));
		GoldBackgroundChestImage->SetRenderTranslation(FVector2D::ZeroVector);

		if (GoldVisualPanel != nullptr)
		{
			GoldVisualPanel->SetRenderTransformPivot(FVector2D(.5f, .5f));
			GoldVisualPanel->SetRenderOpacity(
				RewardConcept03::Segment(T, .08f, .24f));
			GoldVisualPanel->SetRenderScale(FVector2D(
				FMath::Lerp(.18f, 1.f, RewardEase)));
			GoldVisualPanel->SetRenderTranslation(FVector2D(
				0.f, FMath::Lerp(86.f, 0.f, RewardEase)));
		}
		if (GoldCoinImage != nullptr)
		{
			GoldCoinImage->SetRenderTransformPivot(FVector2D(.5f, .5f));
			GoldCoinImage->SetRenderTransformAngle(
				FMath::Lerp(-28.f, 0.f, RewardEase));
		}
		if (GoldMainText != nullptr)
		{
			const float CountT = RewardConcept03::Segment(T, .40f, .90f);
			const int32 Amount = FMath::RoundToInt(FMath::InterpEaseOut(
				0.f, static_cast<float>(DisplayedGoldAmount), CountT, 3.f));
			GoldMainText->SetText(FText::FromString(
				FString::Printf(TEXT("+%d G"), Amount)));
		}
		if (PresentationFlash != nullptr)
		{
			PresentationFlash->SetRenderOpacity(0.f);
		}
		return;
	}

	const float CoinT = RewardConcept03::Segment(T, .02f, .65f);
	const float CoinEase = RewardConcept03::EaseOutBack(CoinT);
	if (GoldCoinImage != nullptr)
	{
		GoldCoinImage->SetRenderTransformPivot(FVector2D(.5f, .5f));
		GoldCoinImage->SetRenderOpacity(RewardConcept03::Segment(T, 0.f, .12f));
		GoldCoinImage->SetRenderScale(FVector2D(FMath::Lerp(.20f, 1.f, CoinEase)));
		GoldCoinImage->SetRenderTranslation(FVector2D(
			0.f, FMath::Lerp(115.f, 0.f, CoinEase)));
		GoldCoinImage->SetRenderTransformAngle(FMath::Lerp(-18.f, 0.f, CoinEase));
	}
	if (GoldInfoPanel != nullptr)
	{
		const float InfoT = RewardConcept03::Segment(T, .28f, .66f);
		GoldInfoPanel->SetRenderOpacity(InfoT);
		GoldInfoPanel->SetRenderTranslation(FVector2D(
			FMath::Lerp(42.f, 0.f, FMath::InterpEaseOut(0.f, 1.f, InfoT, 3.f)), 0.f));
	}
	if (GoldMainText != nullptr)
	{
		const float CountT = RewardConcept03::Segment(T, .36f, .88f);
		const int32 Amount = FMath::RoundToInt(FMath::InterpEaseOut(
			0.f, static_cast<float>(DisplayedGoldAmount), CountT, 3.f));
		GoldMainText->SetText(FText::FromString(
			FString::Printf(TEXT("+%d G"), Amount)));
	}
	if (PresentationFlash != nullptr)
	{
		const float Flash = 1.f - RewardConcept03::Segment(T, 0.f, .24f);
		PresentationFlash->SetRenderOpacity(Flash * .28f);
	}
}

void URewardConcept03Widget::FinishGoldReveal()
{
	UpdateGoldReveal(1.f);
	ClaimGoldReward();
	// Keep the awarded amount on screen until an explicit tap. Auto-advancing here
	// made the gold reward unreadable on a fast device.
	PresentationState = EPresentationState::AwaitGoldContinue;
	PresentationElapsed = 0.f;
	ApplyVisualState();
}

void URewardConcept03Widget::StartArtifactReveal()
{
	PresentationState = EPresentationState::ArtifactReveal;
	PresentationElapsed = 0.f;
	SetCurrentStep(RewardConcept03::ArtifactStep);
	UpdateArtifactReveal(0.f);
}

void URewardConcept03Widget::UpdateArtifactReveal(const float NormalizedTime)
{
	const float T = FMath::Clamp(NormalizedTime, 0.f, 1.f);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArtifactChoicePanels); ++Index)
	{
		if (UWidget* Panel = ArtifactChoicePanels[Index])
		{
			const float LocalStart = Index * RewardConcept03::ArtifactStagger;
			const float LocalT = RewardConcept03::Segment(T, LocalStart, LocalStart + .58f);
			const float Ease = RewardConcept03::EaseOutBack(LocalT);
			Panel->SetRenderTransformPivot(FVector2D(.5f, .5f));
			Panel->SetRenderOpacity(RewardConcept03::Segment(
				T, LocalStart, LocalStart + .16f));
			Panel->SetRenderScale(FVector2D(FMath::Lerp(.18f, 1.f, Ease)));
			Panel->SetRenderTranslation(
				RewardConcept03::ArtifactStartTranslations[Index] * (1.f - Ease));
			Panel->SetRenderTransformAngle(
				RewardConcept03::ArtifactStartAngles[Index] * (1.f - Ease));
		}
	}
	if (PresentationFlash != nullptr)
	{
		const float Flash = T < .16f ? RewardConcept03::Segment(T, 0.f, .16f)
			: 1.f - RewardConcept03::Segment(T, .16f, .44f);
		PresentationFlash->SetRenderOpacity(
			GoldChestBlur != nullptr ? 0.f : Flash * .22f);
	}
}

void URewardConcept03Widget::FinishArtifactReveal()
{
	UpdateArtifactReveal(1.f);
	PresentationState = EPresentationState::AwaitArtifactChoice;
	PresentationElapsed = 0.f;
	ApplyVisualState();
}

void URewardConcept03Widget::ResetPresentationVisuals()
{
	RewardConcept03::SetAtlasFrame(ChestSequenceImage, 0);
	RewardConcept03::SetAtlasFrame(ChestSequenceBlendImage, 0);
	const bool bUsesAtlas = ChestSequenceImage != nullptr;
	if (ChestSequenceImage != nullptr)
	{
		ChestSequenceImage->SetVisibility(bUsesAtlas
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		ChestSequenceImage->SetRenderOpacity(1.f);
	}
	if (ChestSequenceBlendImage != nullptr)
	{
		ChestSequenceBlendImage->SetVisibility(ESlateVisibility::Collapsed);
		ChestSequenceBlendImage->SetRenderOpacity(0.f);
	}
	if (ChestVisualSwitcher != nullptr)
	{
		ChestVisualSwitcher->SetVisibility(bUsesAtlas
			? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		ChestVisualSwitcher->SetActiveWidgetIndex(0);
	}
	if (ChestBlendSwitcher != nullptr)
	{
		ChestBlendSwitcher->SetVisibility(ESlateVisibility::Collapsed);
		ChestBlendSwitcher->SetActiveWidgetIndex(0);
		ChestBlendSwitcher->SetRenderOpacity(0.f);
	}
	UWidget* WidgetsToReset[] = {
		ChestVisualPanel.Get(), ChestVisualSwitcher.Get(), ChestSequenceImage.Get(),
		ChestInfoPanel.Get(),
		GoldVisualPanel.Get(), GoldBackgroundChestImage.Get(),
		GoldChestBlur.Get(), GoldCoinImage.Get(), GoldInfoPanel.Get(),
		PresentationFlash.Get() };
	for (UWidget* Widget : WidgetsToReset)
	{
		if (Widget != nullptr)
		{
			Widget->SetRenderOpacity(Widget == PresentationFlash ? 0.f : 1.f);
			Widget->SetRenderTranslation(FVector2D::ZeroVector);
			Widget->SetRenderScale(FVector2D(1.f));
			Widget->SetRenderTransformAngle(0.f);
		}
	}
	// 보조 레이어는 새 개봉이 시작될 때까지 완전히 숨긴다.
	if (ChestBlendSwitcher != nullptr)
	{
		ChestBlendSwitcher->SetRenderOpacity(0.f);
		ChestBlendSwitcher->SetRenderTranslation(FVector2D::ZeroVector);
		ChestBlendSwitcher->SetRenderScale(FVector2D(1.f));
		ChestBlendSwitcher->SetRenderTransformAngle(0.f);
	}
	if (GoldChestBlur != nullptr)
	{
		GoldChestBlur->SetBlurStrength(0.f);
		GoldChestBlur->SetRenderOpacity(0.f);
		GoldChestBlur->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (int32 Wave = 0; Wave < 3; ++Wave)
	{
		for (UImage* Effect : {
			ChestBurstGlows[Wave].Get(), ChestBurstRings[Wave].Get(),
			ChestBurstRays[Wave].Get(), ChestBurstSparks[Wave].Get() })
		{
			if (Effect != nullptr)
			{
				Effect->SetVisibility(ESlateVisibility::Collapsed);
				Effect->SetRenderOpacity(0.f);
				Effect->SetRenderTranslation(FVector2D::ZeroVector);
				Effect->SetRenderScale(FVector2D(1.f));
				Effect->SetRenderTransformAngle(0.f);
			}
		}
	}
	for (UImage* Coin : ChestBurstForegroundCoins)
	{
		if (Coin != nullptr)
		{
			Coin->SetVisibility(ESlateVisibility::Collapsed);
			Coin->SetRenderOpacity(0.f);
			Coin->SetRenderTranslation(FVector2D::ZeroVector);
			Coin->SetRenderScale(FVector2D(1.f));
			Coin->SetRenderTransformAngle(0.f);
		}
	}
	if (GoldMainText != nullptr)
	{
		GoldMainText->SetText(FText::FromString(FString::Printf(
			TEXT("+%d G"), DisplayedGoldAmount)));
	}
	for (UWidget* Panel : ArtifactChoicePanels)
	{
		if (Panel != nullptr)
		{
			Panel->SetRenderOpacity(1.f);
			Panel->SetRenderTranslation(FVector2D::ZeroVector);
			Panel->SetRenderScale(FVector2D(1.f));
			Panel->SetRenderTransformAngle(0.f);
		}
	}
}

void URewardConcept03Widget::SelectArtifact(const int32 ArtifactIndex)
{
	if (bSuppressNextArtifactClick)
	{
		bSuppressNextArtifactClick = false;
		return;
	}
	if (!UsesArtifactStep()
		|| CurrentStepIndex != RewardConcept03::ArtifactStep
		|| PresentationState != EPresentationState::AwaitArtifactChoice
		|| UIModel == nullptr
		|| UIModel->GetAcquisitionPolicy() != ERewardAcquisitionPolicy::SelectOne
		|| ArtifactIndex < 0 || ArtifactIndex >= UE_ARRAY_COUNT(ArtifactButtons)
		|| !UIModel->GetRewardChoices().IsValidIndex(ArtifactIndex))
	{
		return;
	}
	const TArray<FRewardChoiceUI>& Choices = UIModel->GetRewardChoices();
	if (Choices.Num() == 1)
	{
		SelectedArtifactIndex = 0;
		ApplyArtifactSelection();
		ShowArtifactDetails(0);
		return;
	}
	SelectedArtifactIndex = ArtifactIndex;
	ApplyArtifactSelection();
	OnArtifactSelected.Broadcast(SelectedArtifactIndex);
}

void URewardConcept03Widget::BeginArtifactPress(const int32 ArtifactIndex)
{
	CancelArtifactPress();
	if (!UsesArtifactStep()
		|| CurrentStepIndex != RewardConcept03::ArtifactStep
		|| PresentationState != EPresentationState::AwaitArtifactChoice
		|| UIModel == nullptr
		|| !UIModel->GetRewardChoices().IsValidIndex(ArtifactIndex))
	{
		return;
	}
	PressedArtifactIndex = ArtifactIndex;
	// With one artifact a normal tap opens details. Long press remains the
	// inspection gesture only when a tap is needed to choose among several cards.
	if (UIModel->GetRewardChoices().Num() <= 1)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ArtifactLongPressTimer,
			FTimerDelegate::CreateWeakLambda(this, [this, ArtifactIndex]()
			{
				if (PressedArtifactIndex == ArtifactIndex)
				{
					bSuppressNextArtifactClick = true;
					ShowArtifactDetails(ArtifactIndex);
				}
			}), RewardConcept03::ArtifactLongPressSeconds, false);
	}
}

void URewardConcept03Widget::EndArtifactPress()
{
	CancelArtifactPress();
}

void URewardConcept03Widget::CancelArtifactPress()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArtifactLongPressTimer);
	}
	PressedArtifactIndex = INDEX_NONE;
}

bool URewardConcept03Widget::EnsureArtifactDetailOverlay()
{
	if (ArtifactDetailOverlayWidget != nullptr)
	{
		return true;
	}
	static const TCHAR* DetailClassPath =
		TEXT("/Game/UI/CombatDetail/WBP_CombatDetailOverlay.WBP_CombatDetailOverlay_C");
	UClass* DetailClass = LoadClass<UUserWidget>(nullptr, DetailClassPath);
	if (DetailClass == nullptr)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Reward artifact detail WBP unavailable: %s"), DetailClassPath);
		return false;
	}
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		ArtifactDetailOverlayWidget =
			CreateWidget<UUserWidget>(OwningPlayer, DetailClass);
	}
	else if (UWorld* World = GetWorld())
	{
		ArtifactDetailOverlayWidget = CreateWidget<UUserWidget>(World, DetailClass);
	}
	if (ArtifactDetailOverlayWidget == nullptr)
	{
		return false;
	}

	// Combat reward itself is placed at Z=10000, so the shared detail must sit
	// above it. Treasure reward uses the same instance and therefore the same Z.
	ArtifactDetailOverlayWidget->AddToViewport(10010);
	ArtifactDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	RDDetailOverlay::EnsureModalInputShield(ArtifactDetailOverlayWidget);
	if (UButton* CloseButton = Cast<UButton>(
		ArtifactDetailOverlayWidget->GetWidgetFromName(TEXT("DetailCloseButton"))))
	{
		CloseButton->OnClicked.AddUniqueDynamic(
			this, &URewardConcept03Widget::HandleArtifactDetailCloseClicked);
	}
	return true;
}

void URewardConcept03Widget::ReleaseArtifactDetailOverlay()
{
	if (ArtifactDetailOverlayWidget == nullptr)
	{
		return;
	}
	for (const TCHAR* ButtonName : { TEXT("DetailCloseCatch"),
		TEXT("DetailCloseButton") })
	{
		if (UButton* Button = Cast<UButton>(
			ArtifactDetailOverlayWidget->GetWidgetFromName(ButtonName)))
		{
			Button->OnClicked.RemoveDynamic(
				this, &URewardConcept03Widget::HandleArtifactDetailCloseClicked);
		}
	}
	ArtifactDetailOverlayWidget->RemoveFromParent();
	ArtifactDetailOverlayWidget = nullptr;
}

void URewardConcept03Widget::ShowArtifactDetails(const int32 ArtifactIndex)
{
	if (UIModel == nullptr
		|| !UIModel->GetRewardChoices().IsValidIndex(ArtifactIndex)
		|| EnsureArtifactDetailOverlay() == false)
	{
		return;
	}
	const FRewardChoiceUI& Choice = UIModel->GetRewardChoices()[ArtifactIndex];
	auto Find = [this](const TCHAR* Name) -> UWidget*
	{
		return ArtifactDetailOverlayWidget->GetWidgetFromName(Name);
	};
	auto Show = [&Find](const TCHAR* Name, const bool bShown,
		const ESlateVisibility ShownVisibility = ESlateVisibility::SelfHitTestInvisible)
	{
		if (UWidget* Widget = Find(Name))
		{
			Widget->SetVisibility(bShown ? ShownVisibility
				: ESlateVisibility::Collapsed);
		}
	};
	auto SetText = [&Find](const TCHAR* Name, const FText& Text)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Find(Name)))
		{
			TextBlock->SetText(Text);
		}
	};

	SetText(TEXT("DetailTitleText"), Choice.mName);
	SetText(TEXT("DetailSubtitleText"), Choice.mRarityName.IsEmpty()
		? GetRewardChoiceTypeText(ArtifactIndex) : Choice.mRarityName);
	const FText EffectText = Choice.mDescription.IsEmpty()
		? LOCTEXT("ArtifactFallbackEffect", "파티 전체에 적용됩니다.")
		: Choice.mDescription;
	SetText(TEXT("DetailBodyText"), EffectText);
	SetText(TEXT("DetailExtraHeading"), LOCTEXT("EffectHeading", "효과"));
	SetText(TEXT("DetailExtraText"), EffectText);

	if (UImage* Icon = Cast<UImage>(Find(TEXT("DetailIconImage"))))
	{
		Icon->SetVisibility(Choice.mIcon != nullptr
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Choice.mIcon != nullptr)
		{
			RewardConcept03::SetPortraitCropped(Icon, Choice.mIcon.Get());
		}
	}

	// Match Combat HUD's artifact presentation: identity + wide effect column,
	// no stat/skill columns or free-form skill plate.
	Show(TEXT("DetailIdentityColumn"), true);
	Show(TEXT("DetailStatColumn"), false);
	Show(TEXT("DetailRightColumn"), false);
	Show(TEXT("DetailWideColumn"), true);
	Show(TEXT("DetailStatBlock"), false);
	Show(TEXT("DetailTargetBlock"), false);
	Show(TEXT("DetailSkillBlock"), false);
	Show(TEXT("DetailExtraBlock"), false);
	Show(TEXT("DetailFreePlate"), false);
	Show(TEXT("DetailBodyText"), true);
	Show(TEXT("DetailSkillRowHost"), false);
	// 아티팩트의 아이콘/등급/효과는 한 정보면으로 읽힌다. 공용 상세판의
	// 가로 구분선이 내용을 위아래로 쪼개지 않도록 둘 다 접는다.
	Show(TEXT("DetailDivider_0"), false);
	Show(TEXT("DetailDivider_1"), false);

	static const int32 LitByRarity[] = { 1, 3, 5 };
	const int32 LitCount = LitByRarity[
		FMath::Clamp(Choice.mRarityLevel, 0, 2)];
	for (int32 Index = 0; Index < 5; ++Index)
	{
		if (UImage* Gem = Cast<UImage>(Find(
			*FString::Printf(TEXT("DetailRarityGem_%d"), Index))))
		{
			Gem->SetVisibility(ESlateVisibility::HitTestInvisible);
			Gem->SetColorAndOpacity(Index < LitCount
				? FLinearColor::White
				: FLinearColor(0.18f, 0.16f, 0.14f, 1.f));
		}
	}
	ArtifactDetailOverlayWidget->SetVisibility(
		ESlateVisibility::SelfHitTestInvisible);
}

void URewardConcept03Widget::HandleArtifact0Pressed() { BeginArtifactPress(0); }
void URewardConcept03Widget::HandleArtifact1Pressed() { BeginArtifactPress(1); }
void URewardConcept03Widget::HandleArtifact2Pressed() { BeginArtifactPress(2); }
void URewardConcept03Widget::HandleArtifactReleased() { EndArtifactPress(); }

void URewardConcept03Widget::HandleArtifactDetailCloseClicked()
{
	HideArtifactDetails();
}

void URewardConcept03Widget::HideArtifactDetails()
{
	if (ArtifactDetailOverlayWidget != nullptr)
	{
		ArtifactDetailOverlayWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URewardConcept03Widget::SetCurrentStep(const int32 StepIndex)
{
	const int32 LastStep = UsesArtifactStep()
		? RewardConcept03::ArtifactStep : RewardConcept03::GoldStep;
	const int32 ClampedStep = FMath::Clamp(
		StepIndex, RewardConcept03::FirstStep, LastStep);
	if (CurrentStepIndex == ClampedStep)
	{
		return;
	}
	CurrentStepIndex = ClampedStep;
	ApplyVisualState();
	OnRewardStepChanged.Broadcast(CurrentStepIndex);
}

void URewardConcept03Widget::ClaimExperienceReward()
{
	if (bExperienceClaimRequested || UIModel == nullptr)
	{
		return;
	}
	bExperienceClaimRequested = true;
	if (UIModel->GetReward().mExpGained > 0)
	{
		UIModel->RequestClaimReward(ERewardClaimKind::Exp);
	}
}

void URewardConcept03Widget::ClaimGoldReward()
{
	if (bGoldClaimRequested || UIModel == nullptr)
	{
		return;
	}
	bGoldClaimRequested = true;
	if (UIModel->GetReward().mGoldGained > 0)
	{
		UIModel->RequestClaimReward(ERewardClaimKind::Gold);
	}
}

void URewardConcept03Widget::CompleteRewardFlow()
{
	if (bFlowCompleted || bRewardRequestPending)
	{
		return;
	}
	if (!UsesArtifactStep())
	{
		SelectedArtifactIndex = INDEX_NONE;
	}
	if (UIModel != nullptr)
	{
		const ERewardAcquisitionPolicy Policy =
			UIModel->GetAcquisitionPolicy();
		if (Policy == ERewardAcquisitionPolicy::SelectOne)
		{
			const TArray<FRewardChoiceUI>& Choices = UIModel->GetRewardChoices();
			if (!Choices.IsValidIndex(SelectedArtifactIndex))
			{
				return;
			}

			PendingRewardId = Choices[SelectedArtifactIndex].mSourceAssetId;
			bRewardRequestPending = true;
			if (UIModel->RequestSelectReward(PendingRewardId) == false)
			{
				bRewardRequestPending = false;
				PendingRewardId = FPrimaryAssetId();
			}
			return;
		}
		if (Policy == ERewardAcquisitionPolicy::GrantAll)
		{
			bRewardRequestPending = true;
			if (UIModel->RequestGrantBundle() == false)
			{
				bRewardRequestPending = false;
			}
			return;
		}
	}

	FinishRewardFlowAfterConfirmation();
}

void URewardConcept03Widget::EnsureRunOptionsRail()
{
	if (RunOptionsRailWidget != nullptr)
	{
		RunOptionsRailWidget->SetMapContext(false);
		RunOptionsRailWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}
	UClass* RailClass = LoadClass<URunOptionsRailWidget>(nullptr,
		TEXT("/Game/UI/Common/WBP_RunOptionsRail.WBP_RunOptionsRail_C"));
	if (RailClass == nullptr)
	{
		RailClass = URunOptionsRailWidget::StaticClass();
	}
	if (APlayerController* Owner = GetOwningPlayer())
	{
		RunOptionsRailWidget = CreateWidget<URunOptionsRailWidget>(Owner, RailClass);
	}
	else if (UWorld* World = GetWorld())
	{
		RunOptionsRailWidget = CreateWidget<URunOptionsRailWidget>(World, RailClass);
	}
	if (RunOptionsRailWidget != nullptr)
	{
		RunOptionsRailWidget->SetMapContext(false);
		if (GetOwningPlayer() != nullptr)
		{
			RunOptionsRailWidget->AddToViewport(10001);
		}
	}
}

void URewardConcept03Widget::FinishRewardFlowAfterConfirmation()
{
	if (bFlowCompleted)
	{
		return;
	}

	if (UIModel != nullptr)
	{
		UIModel->RequestFinishPresentation();
	}
	bFlowCompleted = true;
	PresentationState = EPresentationState::Completed;
	ApplyVisualState();
	if (BottomActionButton != nullptr)
	{
		BottomActionButton->SetIsEnabled(false);
	}
	OnRewardFlowCompleted.Broadcast(SelectedArtifactIndex);
}

void URewardConcept03Widget::ApplyVisualState()
{
	const int32 SwitcherIndex =
		CurrentStepIndex + RewardConcept03::SwitcherWarmupOffset;
	for (UWidgetSwitcher* Switcher : {
		StepSwitcher.Get(), ProgressSwitcher.Get(), TabSwitcher.Get(),
		ButtonLabelSwitcher.Get() })
	{
		if (Switcher != nullptr && SwitcherIndex < Switcher->GetNumWidgets())
		{
			Switcher->SetActiveWidgetIndex(SwitcherIndex);
		}
	}

	if (ChestMainText != nullptr)
	{
		ChestMainText->SetText(LOCTEXT("ChestOpen", "상자 열기"));
	}
	if (ChestHintText != nullptr)
	{
		ChestHintText->SetText(LOCTEXT("ChestOpenHint", "상자를 눌러 여세요"));
	}
	if (ConfirmButtonText != nullptr && !bFlowCompleted)
	{
		ConfirmButtonText->SetText(LOCTEXT("Confirm", "확정"));
	}

	const bool bBottomVisible =
		PresentationState == EPresentationState::Idle
		|| PresentationState == EPresentationState::AwaitGoldContinue
		|| PresentationState == EPresentationState::AwaitConfirm
		|| PresentationState == EPresentationState::AwaitArtifactChoice;
	if (BottomButtonPanel != nullptr)
	{
		BottomButtonPanel->SetVisibility(bBottomVisible
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (BottomActionButton != nullptr)
	{
		BottomActionButton->SetIsEnabled(
			bBottomVisible && !bFlowCompleted && !bRewardRequestPending);
	}
	ApplyBottomActionVisual();
	if (ChestButton != nullptr)
	{
		ChestButton->SetIsEnabled(
			PresentationState == EPresentationState::ChestAwaitInput);
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ArtifactButtons); ++Index)
	{
		UButton* ArtifactButton = ArtifactButtons[Index];
		if (ArtifactButton != nullptr)
		{
			// Choice cards live only on the artifact switcher page. Keep an
			// available card enabled throughout that page's lifecycle so GrantAll
			// cards can receive the same long-press detail input as SelectOne cards;
			// the handlers still guard the exact interaction state.
			ArtifactButton->SetIsEnabled(UIModel != nullptr
				&& UIModel->GetRewardChoices().IsValidIndex(Index)
				&& !bFlowCompleted && !bRewardRequestPending);
		}
	}
	ApplyArtifactSelection();
}

void URewardConcept03Widget::ApplyArtifactSelection()
{
	if (SelectionOutline == nullptr)
	{
		return;
	}
	if (PresentationState != EPresentationState::AwaitArtifactChoice
		|| UIModel == nullptr
		|| UIModel->GetAcquisitionPolicy() != ERewardAcquisitionPolicy::SelectOne
		|| SelectedArtifactIndex < 0
		|| SelectedArtifactIndex >= UE_ARRAY_COUNT(ArtifactChoicePanels))
	{
		SelectionOutline->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SelectionOutline->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(SelectionOutline->Slot);
	const UCanvasPanelSlot* PanelSlot = ArtifactChoicePanels[SelectedArtifactIndex]
		? Cast<UCanvasPanelSlot>(ArtifactChoicePanels[SelectedArtifactIndex]->Slot)
		: nullptr;
	if (CanvasSlot != nullptr && PanelSlot != nullptr)
	{
		FVector2D Position = CanvasSlot->GetPosition();
		// 카드와 outline 폭이 달라도 두 중심이 일치하게 한다.
		Position.X = PanelSlot->GetPosition().X
			+ (PanelSlot->GetSize().X - CanvasSlot->GetSize().X) * .5f;
		CanvasSlot->SetPosition(Position);
	}
}

void URewardConcept03Widget::ApplyBottomActionVisual(const bool bPressed)
{
	const bool bEnabled = BottomActionButton == nullptr
		|| BottomActionButton->GetIsEnabled();
	if (BottomButtonArt != nullptr)
	{
		BottomButtonArt->SetColorAndOpacity(!bEnabled
			? FLinearColor(.48f, .48f, .48f, 1.f)
			: bBottomActionHovered
				? FLinearColor(1.14f, 1.08f, .96f, 1.f)
				: FLinearColor::White);
	}
	if (BottomButtonPanel != nullptr)
	{
		BottomButtonPanel->SetRenderTransformPivot(FVector2D(.5f, .5f));
		BottomButtonPanel->SetRenderTranslation(
			bPressed ? FVector2D(0.f, 3.f) : FVector2D::ZeroVector);
		BottomButtonPanel->SetRenderScale(bPressed
			? FVector2D(.985f) : bBottomActionHovered && bEnabled
				? FVector2D(1.015f) : FVector2D(1.f));
	}
}

void URewardConcept03Widget::HandleBottomActionClicked()
{
	AdvanceRewardFlow();
}

void URewardConcept03Widget::HandleChestClicked()
{
	OpenRewardChest();
}

void URewardConcept03Widget::HandleArtifact0Clicked()
{
	SelectArtifact(0);
}

void URewardConcept03Widget::HandleArtifact1Clicked()
{
	SelectArtifact(1);
}

void URewardConcept03Widget::HandleArtifact2Clicked()
{
	SelectArtifact(2);
}

void URewardConcept03Widget::HandleBottomActionHovered()
{
	bBottomActionHovered = true;
	ApplyBottomActionVisual();
}

void URewardConcept03Widget::HandleBottomActionUnhovered()
{
	bBottomActionHovered = false;
	ApplyBottomActionVisual();
}

void URewardConcept03Widget::HandleBottomActionPressed()
{
	ApplyBottomActionVisual(true);
}

void URewardConcept03Widget::HandleBottomActionReleased()
{
	ApplyBottomActionVisual();
}

#undef LOCTEXT_NAMESPACE
