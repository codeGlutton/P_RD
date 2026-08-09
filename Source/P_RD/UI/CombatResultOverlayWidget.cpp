#include "UI/CombatResultOverlayWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "CombatResultOverlayWidget"

UCombatResultOverlayWidget::UCombatResultOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = 60;
	mRemoveFromParentOnClose = true;
}

void UCombatResultOverlayWidget::ShowVictoryReward(const FRewardUI& Reward, FSimpleDelegate ConfirmCallback)
{
	mMode = ECombatResultOverlayMode::VictoryReward;
	mReward = Reward;
	mTitleCallback = MoveTemp(ConfirmCallback);
	mRetryCallback.Unbind();
	RefreshWidget();
}

void UCombatResultOverlayWidget::ShowDefeatResult(
	const FCombatResultUI& Result,
	FSimpleDelegate TitleCallback,
	FSimpleDelegate RetryCallback)
{
	mMode = ECombatResultOverlayMode::DefeatContinue;
	mReward = FRewardUI();
	mCombatResult = Result;
	mTitleCallback = MoveTemp(TitleCallback);
	mRetryCallback = MoveTemp(RetryCallback);
	BindButtons();
	RefreshWidget();
}

void UCombatResultOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogRD, Display, TEXT("Combat defeat WBP construct: title=%s retry=%s"),
		mTitleButton != nullptr ? TEXT("bound") : TEXT("missing"),
		mRetryButton != nullptr ? TEXT("bound") : TEXT("missing"));

	BindButtons();

	RefreshWidget();
}

void UCombatResultOverlayWidget::NativeDestruct()
{
	if (mTitleButton != nullptr)
	{
		mTitleButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
	}
	if (mRetryButton != nullptr)
	{
		mRetryButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleRetryClicked);
	}

	Super::NativeDestruct();
}

void UCombatResultOverlayWidget::BindButtons()
{
	if (mTitleButton != nullptr)
	{
		mTitleButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
		mTitleButton->OnClicked.AddUniqueDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
	}
	if (mRetryButton != nullptr)
	{
		mRetryButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleRetryClicked);
		mRetryButton->OnClicked.AddUniqueDynamic(this, &UCombatResultOverlayWidget::HandleRetryClicked);
	}
}

void UCombatResultOverlayWidget::HandleTitleClicked()
{
	UE_LOG(LogRD, Display, TEXT("Combat defeat title button clicked."));
	FSimpleDelegate Callback = MoveTemp(mTitleCallback);
	mTitleCallback.Unbind();
	mRetryCallback.Unbind();
	Callback.ExecuteIfBound();
}

void UCombatResultOverlayWidget::HandleRetryClicked()
{
	UE_LOG(LogRD, Display, TEXT("Combat defeat retry button clicked."));
	FSimpleDelegate Callback = MoveTemp(mRetryCallback);
	mTitleCallback.Unbind();
	mRetryCallback.Unbind();
	Callback.ExecuteIfBound();
}

void UCombatResultOverlayWidget::RefreshWidget()
{
	if (mMode != ECombatResultOverlayMode::DefeatContinue)
	{
		return;
	}

	if (mLocationText != nullptr)
	{
		mLocationText->SetText(FText::Format(
			LOCTEXT("LocationFormat", "도달 지점    {0}"),
			mCombatResult.mLocationName.IsEmpty()
				? LOCTEXT("UnknownLocation", "현재 전투 지역")
				: mCombatResult.mLocationName));
	}
	if (mRoundText != nullptr)
	{
		mRoundText->SetText(FText::Format(
			LOCTEXT("RoundFormat", "진행 라운드    {0} 라운드"),
			FText::AsNumber(FMath::Max(1, mCombatResult.mRound))));
	}
	if (mEnemyText != nullptr)
	{
		mEnemyText->SetText(FText::Format(
			LOCTEXT("EnemyFormat", "처치한 몬스터    {0}"),
			FText::AsNumber(mCombatResult.mDefeatedMonsterCount)));
	}
	if (mGoldText != nullptr)
	{
		mGoldText->SetText(FText::Format(
			LOCTEXT("GoldFormat", "획득 골드    {0}"),
			FText::AsNumber(mCombatResult.mGoldGained)));
	}
	if (mExpText != nullptr)
	{
		mExpText->SetText(FText::Format(
			LOCTEXT("ExpFormat", "획득 경험치    +{0}"),
			FText::AsNumber(mCombatResult.mExpGained)));
	}

	// 파티에 없는 자리는 카드째로 접는다 -- WBP에 구워진 기본 초상(디자이너
	// 견본)이 실제 파티처럼 새어 나오던 문제(0809).
	UImage* PortraitImages[] = { mPartyPortrait0, mPartyPortrait1, mPartyPortrait2 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PortraitImages); ++Index)
	{
		UTexture2D* Portrait = mCombatResult.mPartyPortraits.IsValidIndex(Index)
			? mCombatResult.mPartyPortraits[Index] : nullptr;
		const bool bHasMember = Portrait != nullptr;
		if (UWidget* CardMount = GetWidgetFromName(FName(*FString::Printf(
			TEXT("DefeatCardFrame_%dMount"), Index))))
		{
			CardMount->SetVisibility(bHasMember
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
		UImage* PortraitImage = PortraitImages[Index];
		if (PortraitImage == nullptr)
		{
			continue;
		}
		PortraitImage->SetVisibility(bHasMember
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
		if (bHasMember == true)
		{
			PortraitImage->SetBrushFromTexture(Portrait, false);
		}
	}
}

#undef LOCTEXT_NAMESPACE
