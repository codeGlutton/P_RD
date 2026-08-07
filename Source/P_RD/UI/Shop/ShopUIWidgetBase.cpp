#include "UI/Shop/ShopUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/ViewportZOrderType.h"

#define LOCTEXT_NAMESPACE "ShopUIWidgetBase"

namespace
{
	/** @brief 상점 항목 종류별 기본 아이콘 텍스처 경로(SVN 임포트). */
	// 옛 InSideAsset 경로 3종은 프로젝트에서 지워져 아이콘이 빈칸으로
	// 나왔다(0807 감사). 실재하는 SVN 에셋으로 갈아 끼운다.
	const TCHAR* ShopKindIconPath(EShopItemKind Kind)
	{
		switch (Kind)
		{
		case EShopItemKind::Skill:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/T_SkillIcon_Whirlwind.T_SkillIcon_Whirlwind");
		case EShopItemKind::Equipment:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common");
		case EShopItemKind::Heal:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Concept02/T_skill_meditation_heal_icon.T_skill_meditation_heal_icon");
		default:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Concept02/T_gold_icon.T_gold_icon");
		}
	}

	UTexture2D* ResolveShopIcon(const FShopItemUI& Item)
	{
		if (Item.mIcon != nullptr)
		{
			return Item.mIcon;
		}
		return LoadObject<UTexture2D>(nullptr, ShopKindIconPath(Item.mKind));
	}
}

UShopUIWidgetBase::UShopUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/** @brief 나가기 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그린다. */
void UShopUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &UShopUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(LOCTEXT("Leave", "Leave"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(LOCTEXT("Shop", "Shop"));
	}

	RefreshView();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 상점 스냅샷도 즉시 한 번 그린다. */
void UShopUIWidgetBase::BindUIModel(UShopUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
		// 상점 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그린다.
		RefreshView();
		OnShopRefreshed();
	}
}

void UShopUIWidgetBase::HandleCloseClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
	RemoveFromParent();
}

/** @brief WBP의 슬롯 구매 입력을 UIModel의 구매 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::BuyItem(int32 SlotIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestBuy(SlotIndex);
	}
}

/** @brief WBP의 나가기 입력을 UIModel의 나가기 의도 이벤트로 전달한다. */
void UShopUIWidgetBase::Leave()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void UShopUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UShopUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림 → 뷰 갱신 후 WBP 구현 이벤트로도 전달. */
void UShopUIWidgetBase::HandleUIChanged()
{
	RefreshView();
	OnShopRefreshed();
}

/** @brief 현재 모델의 골드/판매 슬롯을 BindWidget 위젯(골드 라벨 + 항목 박스)에 반영한다. */
void UShopUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FShopUI& Shop = mUIModel->GetShop();

	if (mGoldText != nullptr)
	{
		mGoldText->SetText(FText::Format(
			LOCTEXT("Gold {0}", "Gold {0}"),
			FText::AsNumber(Shop.mGold)));
	}

	if (mItemBox == nullptr)
	{
		return;
	}
	mItemBox->ClearChildren();

	for (const FShopItemUI& Item : Shop.mItems)
	{
		// 슬롯 카드 = 세로 박스(아이콘 + 이름 + 가격). 종류별 아이콘 자동 매핑.
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (Card == nullptr)
		{
			continue;
		}

		if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			if (UTexture2D* Tex = ResolveShopIcon(Item))
			{
				Icon->SetBrushFromTexture(Tex, false);
			}
			Icon->SetDesiredSizeOverride(FVector2D(112.f, 112.f));
			// 품절/구매불가는 흐리게.
			Icon->SetColorAndOpacity((Item.mIsSoldOut || !Item.mIsAffordable)
				? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White);
			Card->AddChildToVerticalBox(Icon);
		}

		if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Name->SetText(Item.mName);
			Name->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
			{
				NameSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
				NameSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UTextBlock* Price = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			const FText PriceText = Item.mIsSoldOut
				? LOCTEXT("Sold Out", "Sold Out")
				: FText::Format(LOCTEXT("{0} G", "{0} G"), FText::AsNumber(Item.mPrice));
			Price->SetText(PriceText);
			Price->SetJustification(ETextJustify::Center);
			Price->SetColorAndOpacity(FSlateColor(Item.mIsAffordable && !Item.mIsSoldOut
				? FLinearColor(0.95f, 0.85f, 0.45f, 1.f) : FLinearColor(0.8f, 0.4f, 0.4f, 1.f)));
			if (UVerticalBoxSlot* PriceSlot = Card->AddChildToVerticalBox(Price))
			{
				PriceSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
				PriceSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UHorizontalBoxSlot* BoxSlot = mItemBox->AddChildToHorizontalBox(Card))
		{
			BoxSlot->SetPadding(FMargin(14.f, 0.f));
		}
	}
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UShopUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
