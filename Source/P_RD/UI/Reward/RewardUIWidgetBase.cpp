#include "UI/Reward/RewardUIWidgetBase.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/ViewportZOrderType.h"

namespace
{
	/** @brief 기존 BindWidget Image 의 브러시 텍스처/크기만 갈아끼운다(위젯 생성 아님 — 데이터 설정). */
	void SetImageTexture(UImage* Image, UTexture2D* Tex, float Size)
	{
		if (Image == nullptr || Tex == nullptr)
		{
			return;
		}
		FSlateBrush Brush;
		Brush.SetResourceObject(Tex);
		Brush.SetImageSize(FVector2D(Size, Size));
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Image->SetBrush(Brush);
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

	static ConstructorHelpers::FObjectFinder<UTexture2D> CloseButtonTexture(TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Start_DarkFantasy_Base.UI_Button_Start_DarkFantasy_Base"));
	if (CloseButtonTexture.Succeeded() == true)
	{
		mCloseButtonTexture = CloseButtonTexture.Object;
	}
}

/** @brief 받기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void URewardUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &URewardUIWidgetBase::HandleCloseClicked);
	}

	ApplyClaimButtonStyle();
	RefreshSummary();
	RefreshItem();
}

/** @details WBP가 배치한 '받기' 버튼 인스턴스의 스타일 브러시만 갈아끼운다(타이틀/캐릭터선택과 동일 자산). */
void URewardUIWidgetBase::ApplyClaimButtonStyle() const
{
	if (mCloseButton == nullptr)
	{
		return;
	}
	UTexture2D* Texture = mCloseButtonTexture;
	if (Texture == nullptr)
	{
		return;
	}

	FSlateBrush NormalBrush;
	NormalBrush.DrawAs = ESlateBrushDrawType::Image;
	NormalBrush.ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
	NormalBrush.SetResourceObject(Texture);

	FButtonStyle Style = mCloseButton->GetStyle();
	Style.SetNormal(NormalBrush);
	Style.SetHovered(NormalBrush);
	Style.SetPressed(NormalBrush);
	Style.SetDisabled(NormalBrush);
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	mCloseButton->SetStyle(Style);
}

UTexture2D* URewardUIWidgetBase::GetRewardIcon(ERewardChoiceKind Kind) const
{
	switch (Kind)
	{
	case ERewardChoiceKind::Equipment: return mEquipmentIcon;
	case ERewardChoiceKind::Skill:     return mSkillIcon;
	case ERewardChoiceKind::Gold:      return mGoldIcon;
	case ERewardChoiceKind::Dice:
	default:                           return mDiceIcon;
	}
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
		RefreshSummary();
		RefreshItem();
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
	RefreshSummary();
}

void URewardUIWidgetBase::HandleChoicesChanged()
{
	RefreshItem();
}

/** @brief 제목 + 골드/경험치 값을 칩 텍스트에 반영(아이콘은 WBP 고정). */
void URewardUIWidgetBase::RefreshSummary()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	const FRewardUI& Reward = mUIModel->GetReward();

	if (mTitleText != nullptr && !Reward.mTitle.IsEmpty())
	{
		mTitleText->SetText(Reward.mTitle);
	}
	if (mGoldValue != nullptr)
	{
		mGoldValue->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Reward.mGoldGained)));
	}
	if (mGoldSub != nullptr)
	{
		mGoldSub->SetText(FText::FromString(FString::Printf(TEXT("보유 %d"), Reward.mGoldBalance)));
	}
	if (mExpValue != nullptr)
	{
		mExpValue->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Reward.mExpGained)));
	}
	if (mExpSub != nullptr)
	{
		mExpSub->SetText(FText::FromString(FString::Printf(TEXT("Lv %d"), Reward.mLevelAfter)));
	}
}

/** @brief 그 방의 보상 항목(0~1개)을 항목 카드에 반영하거나 카드를 숨긴다. */
void URewardUIWidgetBase::RefreshItem()
{
	if (mUIModel == nullptr)
	{
		return;
	}
	const TArray<FRewardChoiceUI>& Items = mUIModel->GetRewardChoices();

	if (Items.Num() == 0)
	{
		// 항목 없는 방(일반 적): 카드 숨김.
		if (mItemCard != nullptr)
		{
			mItemCard->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	const FRewardChoiceUI& Item = Items[0];
	if (mItemCard != nullptr)
	{
		// 행 배경은 WBP가 정한 어두운 톤 그대로 둔다(다른 행과 통일). 희귀도는 아이콘 프레임이 표현.
		mItemCard->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (mItemIcon != nullptr)
	{
		UTexture2D* Tex = (Item.mIcon != nullptr)
			? Item.mIcon.Get() : GetRewardIcon(Item.mKind);
		SetImageTexture(mItemIcon, Tex, 78.f);   // 리스트 행 아이콘 크기
	}
	if (mItemName != nullptr)
	{
		mItemName->SetText(Item.mName);
	}
	if (mItemDetail != nullptr)
	{
		mItemDetail->SetText(Item.mDescription);
	}
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void URewardUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}
