#include "UI/Reward/RewardSettlementWidgetBase.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
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
	RD_SETTLEMENT_TEX(mGoldIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Combat/Result/Rewards/T_reward_v4_gold_icon.T_reward_v4_gold_icon");
	RD_SETTLEMENT_TEX(mEquipmentIconTexture, "/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Items/Equipment/T_equip_weapon_common.T_equip_weapon_common");
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
	RefreshView();
}

void URewardSettlementWidgetBase::NativeDestruct()
{
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
	if (mContinueCommitted)
	{
		return;
	}

	// 단계① -> 경험치·골드를 받고, 아티팩트가 걸려 있으면 단계②로 넘어간다.
	if (mCurrentStep == 1 && mUIModel != nullptr)
	{
		const FRewardUI Reward = mUIModel->GetReward();
		if (Reward.mGoldGained > 0)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Gold);
		}
		if (Reward.mExpGained > 0)
		{
			mUIModel->RequestClaimReward(ERewardClaimKind::Exp);
		}
		if (mUIModel->GetRewardChoices().Num() > 0)
		{
			mCurrentStep = 2;
			mSelectedChoice = INDEX_NONE;
			RebuildStep();
			return;
		}
	}

	// 단계② -> 고른 것 하나만 받는다. (고르기 전에는 단추가 잠겨 있다)
	if (mCurrentStep == 2 && mUIModel != nullptr)
	{
		const TArray<FRewardChoiceUI> Choices = mUIModel->GetRewardChoices();
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
	const FLinearColor Lit(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor Dim(0.45f, 0.42f, 0.38f, 1.0f);
	if (UImage* Coin1 = Cast<UImage>(GetWidgetFromName(TEXT("StepCoinArt_1"))))
	{
		Coin1->SetColorAndOpacity(mCurrentStep == 1 ? Lit : Dim);
	}
	if (UImage* Coin2 = Cast<UImage>(GetWidgetFromName(TEXT("StepCoinArt_2"))))
	{
		Coin2->SetColorAndOpacity(mCurrentStep == 2 ? Lit : Dim);
	}
	if (UTextBlock* Number1 = Cast<UTextBlock>(GetWidgetFromName(TEXT("StepCoinNumber_1"))))
	{
		Number1->SetRenderOpacity(mCurrentStep == 1 ? 1.0f : 0.58f);
	}
	if (UTextBlock* Number2 = Cast<UTextBlock>(GetWidgetFromName(TEXT("StepCoinNumber_2"))))
	{
		Number2->SetRenderOpacity(mCurrentStep == 2 ? 1.0f : 0.58f);
	}
}

void URewardSettlementWidgetBase::RebuildStep()
{
	RefreshStepCoins();

	const int32 ChoiceCount = mUIModel != nullptr ? mUIModel->GetRewardChoices().Num() : 0;
	const int32 VisibleChoiceCount = FMath::Min(ChoiceCount, 3);
	if (mCurrentStep == 2
		&& (mSelectedChoice < 0 || mSelectedChoice >= VisibleChoiceCount))
	{
		mSelectedChoice = INDEX_NONE;
	}
	if (mCurrentStep == 1)
	{
		BuildResultStep();
	}
	else
	{
		BuildChoiceStep();
	}
	if (mNextButtonText != nullptr)
	{
		mNextButtonText->SetText(mCurrentStep == 1 && ChoiceCount > 0
			? LOCTEXT("Next", "다음") : LOCTEXT("Take", "받기"));
	}
	if (mNextButton != nullptr)
	{
		// 3중 1택은 고르기 전에는 못 넘어간다 (0801 확정).
		mNextButton->SetIsEnabled(mCurrentStep == 1 || mSelectedChoice != INDEX_NONE);
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
		Coin->SetVisibility(Reward != nullptr
			? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (UTextBlock* GoldText = Cast<UTextBlock>(GetWidgetFromName(TEXT("SettlementGoldGain"))))
	{
		GoldText->SetText(Reward != nullptr
			? FText::Format(LOCTEXT("GoldGain", "+{0}"),
				FText::AsNumber(Reward->mGoldGained))
			: FText::GetEmpty());
		GoldText->SetVisibility(Reward != nullptr
			? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
		StepSwitcher->SetActiveWidgetIndex(1);
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
		const bool bPicked = mSelectedChoice == INDEX_NONE || Card == mSelectedChoice;
		if (UWidget* Mount = FindIndexedWidget<UWidget>(
			this, TEXT("SettlementChoiceMount"), Card))
		{
			Mount->SetRenderOpacity(bPicked ? 1.0f : 0.55f);
		}
	}
	if (mNextButton != nullptr)
	{
		mNextButton->SetIsEnabled(mCurrentStep == 1 || mSelectedChoice != INDEX_NONE);
	}
}

void URewardSettlementWidgetBase::HandleChoiceClicked_0() { SelectChoice(0); }
void URewardSettlementWidgetBase::HandleChoiceClicked_1() { SelectChoice(1); }
void URewardSettlementWidgetBase::HandleChoiceClicked_2() { SelectChoice(2); }

#undef LOCTEXT_NAMESPACE
