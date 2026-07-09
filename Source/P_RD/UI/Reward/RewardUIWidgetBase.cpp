#include "UI/Reward/RewardUIWidgetBase.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardRowWidgetBase.h"
#include "UI/ViewportZOrderType.h"

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
		case ERewardChoiceKind::Dice:
		default:
			return LOCTEXT("RewardChoiceDice", "주사위");
		}
	}
}

URewardUIWidgetBase::URewardUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

	// 아래 에셋들은 SVN 미연동 환경 등에서 파일이 없을 수 있으므로 LoadObject로 안전하게 로드합니다.
	mEquipmentIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/InSideAsset/UI/Tex/Items/T_Equip_SwordCommon.T_Equip_SwordCommon"));
	mSkillIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/InSideAsset/UI/Tex/Icons/T_Reward_Magic.T_Reward_Magic"));
	mGoldIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/InSideAsset/UI/Tex/Icons/T_Stat_Gold.T_Stat_Gold"));
	mDiceIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/InSideAsset/UI/Tex/Icons/T_Dice_Common.T_Dice_Common"));
	mRewardGoldIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/RewardV4_11/Tex/T_reward_v4_gold_icon.T_reward_v4_gold_icon"));
	mRewardRowWidgetClass = LoadClass<URewardRowWidgetBase>(nullptr, TEXT("/Game/BP/UI/WBP_RewardRow.WBP_RewardRow_C"));
}

/** @brief 받기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void URewardUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &URewardUIWidgetBase::HandleCloseClicked);
		ApplyTransparentCloseButtonStyle();
	}

	RefreshRows();
	UpdateCloseButtonVisibility();
}

void URewardUIWidgetBase::HandleCloseClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaim();
	}
	OnClosed.Broadcast();
	RemoveFromParent();
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
		RefreshRows();
	}
}

/** @brief WBP의 받기 버튼 입력을 UIModel의 Claim 의도 이벤트로 전달한다. */
void URewardUIWidgetBase::Claim()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestClaim();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 알림이 들어오지 않게 한다. */
void URewardUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleUIChanged);
		mUIModel->OnChoicesChanged.RemoveDynamic(this, &URewardUIWidgetBase::HandleChoicesChanged);
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

	// 보상 행이 하나도 없으면(=아직 데이터가 오기 전) 닫기를 숨긴다. 행이 있고 전부 받았을 때만 닫기를 노출한다.
	const bool bCanClose = mUIModel != nullptr && mRewardClaimRows.Num() > 0 && AreAllRewardRowsClaimed();
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
	case ERewardChoiceKind::Dice:
	default:
		return mDiceIcon != nullptr ? mDiceIcon.Get() : mRewardGoldIconTexture.Get();
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
			mRewardRowWidgetClass = LoadClass<URewardRowWidgetBase>(nullptr, TEXT("/Game/BP/UI/WBP_RewardRow.WBP_RewardRow_C"));
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
	UTexture2D* SummaryIcon = mGoldIcon != nullptr ? mGoldIcon.Get() : mRewardGoldIconTexture.Get();

	if (Reward.mGoldGained != 0)
	{
		AddRow(
			ERewardClaimKind::Gold,
			INDEX_NONE,
			FText::Format(LOCTEXT("GoldRewardValue", "골드 +{0}"), FText::AsNumber(Reward.mGoldGained)),
			FText::GetEmpty(),
			SummaryIcon);
	}
	if (Reward.mExpGained != 0)
	{
		AddRow(
			ERewardClaimKind::Exp,
			INDEX_NONE,
			FText::Format(LOCTEXT("ExpRewardValue", "경험치 +{0}"), FText::AsNumber(Reward.mExpGained)),
			FText::GetEmpty(),
			SummaryIcon);
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

/** @brief 게임플레이가 지급을 확정한 보상 행을 목록에서 제거한다. VerticalBox에서 빠지면 아래 행이 자동으로 위로 올라온다. */
void URewardUIWidgetBase::NotifyRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	for (int32 RowIndex = 0; RowIndex < mRewardClaimRows.Num(); ++RowIndex)
	{
		FRewardClaimRow& ClaimRow = mRewardClaimRows[RowIndex];
		if (ClaimRow.mClaimed || ClaimRow.mKind != ClaimKind || ClaimRow.mChoiceIndex != ChoiceIndex)
		{
			continue;
		}

		// 행 인덱스(=위젯 클릭 시 넘어오는 값) 정렬을 유지하려고 배열은 줄이지 않고, 표시 위젯만 박스에서 제거한다.
		ClaimRow.mClaimed = true;
		if (mRewardRowWidgets.IsValidIndex(RowIndex) && mRewardRowWidgets[RowIndex] != nullptr)
		{
			mRewardRowWidgets[RowIndex]->RemoveFromParent();
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
