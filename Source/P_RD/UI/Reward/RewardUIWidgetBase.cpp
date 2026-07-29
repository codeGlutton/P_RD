#include "UI/Reward/RewardUIWidgetBase.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardRowWidgetBase.h"
#include "UI/ViewportZOrderType.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "RewardUIWidgetBase"

namespace
{
	FText MakeRewardChoiceText(const FRewardChoiceUI& Item)
	{
		if (Item.mName.IsEmpty() == false)
		{
			return Item.mName;
		}

		switch (Item.mKind)
		{
		case ERewardChoiceKind::Equipment:
			return LOCTEXT("RewardChoiceEquipment", "장비");
		case ERewardChoiceKind::Skill:
			return LOCTEXT("RewardChoiceSkill", "스킬");
		case ERewardChoiceKind::Gold:
			return LOCTEXT("RewardChoiceGold", "골드");
		default:
			return FText::GetEmpty();
		}
	}
}

URewardUIWidgetBase::URewardUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

	// 바뀐 보상 시안에 실제로 포함된 아이콘만 참조한다. 선택 보상은 데이터
	// 에셋의 아이콘을 우선하고, 없을 때는 아래 공용 아이콘으로 폴백한다.
	mRewardGoldIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_gold_icon.T_reward_v4_gold_icon"));
	mRewardExpIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/RewardV4_11/Tex/T_reward_v4_exp_icon.T_reward_v4_exp_icon"));
	mEquipmentIcon = mRewardGoldIconTexture;
	mSkillIcon = mRewardGoldIconTexture;
	mGoldIcon = mRewardGoldIconTexture;
	mRewardRowWidgetClass = LoadClass<URewardRowWidgetBase>(nullptr, TEXT("/Game/UI/WBP_RewardRow.WBP_RewardRow_C"));

	static ConstructorHelpers::FObjectFinder<UTexture2D> RewardBackgroundFinder(
		TEXT("/Game/UI/Art/RunFlow/T_Reward_Background_Current.T_Reward_Background_Current"));
	if (RewardBackgroundFinder.Succeeded())
	{
		mRewardBackgroundTexture = RewardBackgroundFinder.Object;
	}
}

void URewardUIWidgetBase::OpenUI(FOnEndUIOpenAnimation Callback)
{
	// 보상 화면은 결과를 읽는 화면이다. 시간 기반 등장 연출 없이 같은 프레임에 완성된 값을 보여 준다.
	mCloseCommitted = false;
	StopAllAnimations();
	Super::OpenUI(MoveTemp(Callback));

	// 파생 WBP가 과거의 Blueprint 애니메이션 이벤트를 남겨 두었더라도 계약상 즉시 완료한다.
	StopAllAnimations();
	FinishOpenUI();
}

void URewardUIWidgetBase::CloseUI(FOnEndUICloseAnimation Callback)
{
	StopAllAnimations();
	Super::CloseUI(MoveTemp(Callback));
	StopAllAnimations();
	FinishCloseUI();
}

void URewardUIWidgetBase::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void URewardUIWidgetBase::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

/** @brief 받기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void URewardUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	StopAllAnimations();
	EnsureBackgroundArt();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &URewardUIWidgetBase::HandleCloseClicked);
		ApplyTransparentCloseButtonStyle();
	}

	RefreshRows();
	UpdateCloseButtonVisibility();
}

void URewardUIWidgetBase::EnsureBackgroundArt()
{
	if (WidgetTree == nullptr || mRewardBackgroundTexture == nullptr)
	{
		return;
	}

	UCanvasPanel* DesignCanvas = Cast<UCanvasPanel>(
		WidgetTree->FindWidget(TEXT("RewardDesignCanvas")));
	if (DesignCanvas == nullptr)
	{
		return;
	}

	// 기존 보상 시안의 불투명 금색/남색 판은 새 양피지 배경을 가린다.
	// 버튼·스크롤·텍스트는 그대로 두고 판 이미지만 접는다.
	if (UWidget* LegacyPanel = WidgetTree->FindWidget(
		TEXT("RewardElem_02_widget_00_widget")))
	{
		LegacyPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mRewardBackgroundImage == nullptr)
	{
		mRewardBackgroundImage = Cast<UImage>(
			WidgetTree->FindWidget(TEXT("RewardBackgroundImage")));
	}
	if (mRewardBackgroundImage == nullptr)
	{
		mRewardBackgroundImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("RewardBackgroundImage"));
		DesignCanvas->AddChildToCanvas(mRewardBackgroundImage);
	}

	mRewardBackgroundImage->SetBrushFromTexture(mRewardBackgroundTexture, true);
	mRewardBackgroundImage->SetColorAndOpacity(FLinearColor::White);
	mRewardBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(mRewardBackgroundImage->Slot))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackgroundSlot->SetAlignment(FVector2D::ZeroVector);
		BackgroundSlot->SetOffsets(FMargin(0.f));
		BackgroundSlot->SetAutoSize(false);
		BackgroundSlot->SetZOrder(-100);
	}
}

void URewardUIWidgetBase::HandleCloseClicked()
{
	if (mCloseCommitted || AreAllRewardRowsClaimed() == false)
	{
		return;
	}

	mCloseCommitted = true;
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaim();
	}
	OnClosed.Broadcast();
	CloseUI();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 보상 스냅샷도 즉시 한 번 그린다. */
void URewardUIWidgetBase::BindUIModel(URewardUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}

	UnbindUIModel();
	mUIModel = InUIModel;

	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.AddDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);
		mUIModel->OnRewardClaimConfirmed.AddDynamic(this, &URewardUIWidgetBase::HandleRewardClaimConfirmed);
		RefreshRows();
	}
}

/** @brief WBP의 받기 버튼 입력을 UIModel의 Claim 의도 이벤트로 전달한다. */
void URewardUIWidgetBase::Claim()
{
	HandleCloseClicked();
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 알림이 들어오지 않게 한다. */
void URewardUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);
		mUIModel->OnRewardClaimConfirmed.RemoveDynamic(this, &URewardUIWidgetBase::HandleRewardClaimConfirmed);
	}
	mUIModel = nullptr;
}

void URewardUIWidgetBase::HandleUIChanged()
{
	RefreshRows();
}

void URewardUIWidgetBase::HandleChoicesChanged()
{
	RefreshRows();
}

void URewardUIWidgetBase::ApplyTransparentCloseButtonStyle() const
{
	if (mCloseButton == nullptr)
	{
		return;
	}

	FSlateBrush TransparentBrush;
	TransparentBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);

	FButtonStyle TransparentStyle;
	TransparentStyle.SetNormal(TransparentBrush);
	TransparentStyle.SetHovered(TransparentBrush);
	TransparentStyle.SetPressed(TransparentBrush);
	TransparentStyle.SetDisabled(TransparentBrush);
	TransparentStyle.SetNormalPadding(FMargin(0.0f));
	TransparentStyle.SetPressedPadding(FMargin(0.0f));
	mCloseButton->SetStyle(TransparentStyle);
}

void URewardUIWidgetBase::UpdateCloseButtonVisibility() const
{
	if (mCloseButton == nullptr)
	{
		return;
	}

	// 보상이 0개인 방도 진행을 막지 않는다. 보상이 있으면 게임플레이가 전부 지급 성공을 확정한 뒤에만 닫는다.
	const bool bCanClose = mUIModel != nullptr && AreAllRewardRowsClaimed();
	mCloseButton->SetVisibility(bCanClose ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

bool URewardUIWidgetBase::AreAllRewardRowsClaimed() const
{
	for (const FRewardClaimRow& ClaimRow : mRewardClaimRows)
	{
		if (ClaimRow.mClaimed == false)
		{
			return false;
		}
	}

	return true;
}

int32 URewardUIWidgetBase::GetClaimedRewardRowCount() const
{
	int32 ClaimedCount = 0;
	for (const FRewardClaimRow& ClaimRow : mRewardClaimRows)
	{
		ClaimedCount += ClaimRow.mClaimed ? 1 : 0;
	}
	return ClaimedCount;
}

void URewardUIWidgetBase::RefreshSummary()
{
	RefreshRows();
}

UTexture2D* URewardUIWidgetBase::GetRewardIcon(ERewardChoiceKind Kind) const
{
	switch (Kind)
	{
	case ERewardChoiceKind::Equipment:
		return mEquipmentIcon != nullptr ? mEquipmentIcon.Get() : mRewardGoldIconTexture.Get();
	case ERewardChoiceKind::Skill:
		return mSkillIcon != nullptr ? mSkillIcon.Get() : mRewardGoldIconTexture.Get();
	case ERewardChoiceKind::Gold:
		return mGoldIcon != nullptr ? mGoldIcon.Get() : mRewardGoldIconTexture.Get();
	default:
		return mRewardGoldIconTexture.Get();
	}
}

void URewardUIWidgetBase::RefreshRows()
{
	mRewardClaimRows.Reset();
	mRewardRowWidgets.Reset();

	if (mUIModel == nullptr || mRewardRowsBox == nullptr)
	{
		UpdateCloseButtonVisibility();
		return;
	}

	mRewardRowsBox->ClearChildren();

	auto AddRow = [this](ERewardClaimKind ClaimKind, int32 ChoiceIndex, const FText& MainText, const FText& SubText, UTexture2D* IconTexture)
	{
		if (mRewardRowWidgetClass == nullptr)
		{
			mRewardRowWidgetClass = LoadClass<URewardRowWidgetBase>(nullptr, TEXT("/Game/UI/WBP_RewardRow.WBP_RewardRow_C"));
		}
		if (mRewardRowWidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("WBP_RewardRow 클래스를 찾지 못해 보상 행을 생성하지 못했습니다."));
			return;
		}

		URewardRowWidgetBase* RowWidget = CreateWidget<URewardRowWidgetBase>(GetWorld(), mRewardRowWidgetClass);
		if (RowWidget == nullptr)
		{
			return;
		}

		const int32 RewardRowIndex = mRewardClaimRows.Add(FRewardClaimRow{ ClaimKind, ChoiceIndex, false });

		RowWidget->SetRewardRow(MainText, SubText, IconTexture);
		RowWidget->SetRewardIndex(RewardRowIndex);
		RowWidget->OnRewardRowClicked.AddUniqueDynamic(this, &URewardUIWidgetBase::HandleRewardRowClicked);
		mRewardRowWidgets.Add(RowWidget);

		if (UVerticalBoxSlot* RowSlot = mRewardRowsBox->AddChildToVerticalBox(RowWidget))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			RowSlot->SetVerticalAlignment(VAlign_Center);
			RowSlot->SetPadding(mRewardRowPadding);
		}
	};

	const FRewardUI& Reward = mUIModel->GetReward();
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(Reward.mTitle);
	}

	UTexture2D* SummaryIcon = mGoldIcon != nullptr ? mGoldIcon.Get() : mRewardGoldIconTexture.Get();

	if (Reward.mGoldGained != 0)
	{
		AddRow(
			ERewardClaimKind::Gold,
			INDEX_NONE,
			FText::Format(LOCTEXT("GoldRewardValue", "골드 +{0}"), FText::AsNumber(Reward.mGoldGained)),
			FText::Format(LOCTEXT("GoldRewardBalance", "보유 골드 {0}"), FText::AsNumber(Reward.mGoldBalance)),
			SummaryIcon);
	}
	if (Reward.mExpGained != 0)
	{
		// 경험치 행은 전용 아이콘(없으면 골드 아이콘 폴백).
		UTexture2D* ExpIcon = mRewardExpIconTexture != nullptr ? mRewardExpIconTexture.Get() : SummaryIcon;

		FText ExpProgressText = FText::GetEmpty();
		if (Reward.mMercenaryExp.IsEmpty() == false)
		{
			TArray<FString> MercenaryProgress;
			MercenaryProgress.Reserve(Reward.mMercenaryExp.Num());
			for (const FRewardMercenaryExpUI& Mercenary : Reward.mMercenaryExp)
			{
				const FString Progress = Mercenary.mMaxExp > 0.f
					? FString::Printf(TEXT("%d→%d/%d"),
						FMath::RoundToInt(Mercenary.mExpBefore),
						FMath::RoundToInt(Mercenary.mExpAfter),
						FMath::RoundToInt(Mercenary.mMaxExp))
					: FString::Printf(TEXT("%d→%d"),
						FMath::RoundToInt(Mercenary.mExpBefore),
						FMath::RoundToInt(Mercenary.mExpAfter));
				MercenaryProgress.Add(FText::Format(
					LOCTEXT("MercenaryExpProgress", "{0} Lv.{1} · {2}"),
					Mercenary.mName,
					FText::AsNumber(Mercenary.mLevel),
					FText::FromString(Progress)).ToString());
			}
			ExpProgressText = FText::FromString(
				FString::Join(MercenaryProgress, TEXT("   |   ")));
		}
		else if (Reward.mMaxExp > 0.0f)
		{
			const int32 CurrentExp = FMath::RoundToInt(Reward.mExpAfter);
			const int32 MaxExp = FMath::RoundToInt(Reward.mMaxExp);
			if (Reward.mLevelAfter > 0 && Reward.mLevelAfter != Reward.mLevelBefore)
			{
				ExpProgressText = FText::Format(
					LOCTEXT("ExpRewardLevelUpProgress", "Lv.{0} → Lv.{1} · {2}/{3}"),
					FText::AsNumber(Reward.mLevelBefore),
					FText::AsNumber(Reward.mLevelAfter),
					FText::AsNumber(CurrentExp),
					FText::AsNumber(MaxExp));
			}
			else if (Reward.mLevelAfter > 0)
			{
				ExpProgressText = FText::Format(
					LOCTEXT("ExpRewardProgressWithLevel", "Lv.{0} · {1}/{2}"),
					FText::AsNumber(Reward.mLevelAfter),
					FText::AsNumber(CurrentExp),
					FText::AsNumber(MaxExp));
			}
			else
			{
				ExpProgressText = FText::Format(
					LOCTEXT("ExpRewardProgress", "{0}/{1}"),
					FText::AsNumber(CurrentExp),
					FText::AsNumber(MaxExp));
			}
		}

		AddRow(
			ERewardClaimKind::Exp,
			INDEX_NONE,
			FText::Format(
				Reward.mMercenaryExp.Num() > 1
					? LOCTEXT("PartyExpRewardValue", "모든 용병 경험치 +{0}")
					: LOCTEXT("ExpRewardValue", "경험치 +{0}"),
				FText::AsNumber(Reward.mExpGained)),
			ExpProgressText,
			ExpIcon);
	}

	const TArray<FRewardChoiceUI>& Items = mUIModel->GetRewardChoices();
	for (const FRewardChoiceUI& Item : Items)
	{
		UTexture2D* IconTexture = Item.mIcon != nullptr ? Item.mIcon.Get() : GetRewardIcon(Item.mKind);
		AddRow(ERewardClaimKind::Choice, Item.mChoiceIndex, MakeRewardChoiceText(Item), Item.mDescription, IconTexture);
	}

	UpdateCloseButtonVisibility();
}

void URewardUIWidgetBase::HandleRewardRowClicked(int32 RewardRowIndex)
{
	if (mRewardClaimRows.IsValidIndex(RewardRowIndex) == false || mRewardClaimRows[RewardRowIndex].mClaimed)
	{
		return;
	}

	// 지급 의도만 게임플레이로 보낸다. 실제 지급 성공 여부와 행 제거는 게임플레이 확정(NotifyRewardClaimed)이 결정한다.
	// RequestClaimReward는 동기적으로 게임플레이를 거쳐 NotifyRewardClaimed까지 재진입할 수 있으므로,
	// 값을 복사해 호출하고 이후 이 행 인덱스를 다시 건드리지 않는다(재진입 중 배열 상태 변경 대비).
	const FRewardClaimRow ClaimRow = mRewardClaimRows[RewardRowIndex];
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaimReward(ClaimRow.mKind, ClaimRow.mChoiceIndex);
	}
}

void URewardUIWidgetBase::HandleRewardClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	NotifyRewardClaimed(ClaimKind, ChoiceIndex);
}

/** @brief 게임플레이가 지급을 확정한 보상 행을 목록에 남긴 채 수령 완료 상태로 바꾼다. */
void URewardUIWidgetBase::NotifyRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	for (int32 RowIndex = 0; RowIndex < mRewardClaimRows.Num(); ++RowIndex)
	{
		FRewardClaimRow& ClaimRow = mRewardClaimRows[RowIndex];
		if (ClaimRow.mClaimed || ClaimRow.mKind != ClaimKind || ClaimRow.mChoiceIndex != ChoiceIndex)
		{
			continue;
		}

		// 결과를 사라지게 하지 않는다. 모든 보상을 한눈에 확인할 수 있고, 재클릭만 막는다.
		ClaimRow.mClaimed = true;
		if (mRewardRowWidgets.IsValidIndex(RowIndex) && mRewardRowWidgets[RowIndex] != nullptr)
		{
			mRewardRowWidgets[RowIndex]->SetClaimed(true);
		}
		break;
	}

	UpdateCloseButtonVisibility();
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void URewardUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
