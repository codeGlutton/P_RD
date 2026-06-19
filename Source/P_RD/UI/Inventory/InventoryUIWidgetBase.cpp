#include "UI/Inventory/InventoryUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Inventory/InventoryUIModel.h"
#include "UI/ViewportZOrderType.h"

namespace
{
	/** @brief 인벤토리 항목 종류별 기본 아이콘 텍스처 경로(SVN 임포트). */
	const TCHAR* InventoryKindIconPath(EInventoryItemKind Kind)
	{
		switch (Kind)
		{
		case EInventoryItemKind::Skill:     return TEXT("/Game/SVN/InSideAsset/UI/Tex/Icons/T_Reward_Magic.T_Reward_Magic");
		case EInventoryItemKind::Equipment: return TEXT("/Game/SVN/InSideAsset/UI/Tex/Icons/T_Reward_Equipment.T_Reward_Equipment");
		case EInventoryItemKind::Dice:
		default:                            return TEXT("/Game/SVN/InSideAsset/UI/Tex/Icons/T_Dice_Common.T_Dice_Common");
		}
	}

	UTexture2D* ResolveInventoryIcon(const FInventoryItemUI& Item)
	{
		if (Item.mIcon != nullptr)
		{
			return Item.mIcon;
		}
		return LoadObject<UTexture2D>(nullptr, InventoryKindIconPath(Item.mKind));
	}
}

UInventoryUIWidgetBase::UInventoryUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/** @brief 닫기 버튼 연결 + 고정 라벨 세팅, BindUIModel이 먼저 됐다면 즉시 그린다. */
void UInventoryUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &UInventoryUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(NSLOCTEXT("InventoryUI", "Close", "닫기"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(NSLOCTEXT("InventoryUI", "Title", "인벤토리"));
	}
	if (mDiceLabel != nullptr)
	{
		mDiceLabel->SetText(NSLOCTEXT("InventoryUI", "Dice", "주사위"));
	}
	if (mSkillLabel != nullptr)
	{
		mSkillLabel->SetText(NSLOCTEXT("InventoryUI", "Skill", "스킬"));
	}
	if (mEquipLabel != nullptr)
	{
		mEquipLabel->SetText(NSLOCTEXT("InventoryUI", "Equip", "장비"));
	}

	RefreshView();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 인벤토리 스냅샷도 즉시 한 번 그린다. */
void UInventoryUIWidgetBase::BindUIModel(UInventoryUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UInventoryUIWidgetBase::HandleUIChanged);
		RefreshView();
		OnInventoryRefreshed();
	}
}

void UInventoryUIWidgetBase::HandleCloseClicked()
{
	RemoveFromParent();
}

/** @brief 항목 롱프레스 의도를 뷰모델로 전달한다. */
void UInventoryUIWidgetBase::LongPressItem(EInventoryItemKind Kind, int32 ItemIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestItemLongPress(Kind, ItemIndex);
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 한다. */
void UInventoryUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UInventoryUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief UIModel 변경 알림 → 뷰 갱신 후 WBP 구현 이벤트로도 전달. */
void UInventoryUIWidgetBase::HandleUIChanged()
{
	RefreshView();
	OnInventoryRefreshed();
}

/** @brief 한 섹션 박스를 항목 카드(아이콘+이름)로 채운다. */
void UInventoryUIWidgetBase::FillSection(UHorizontalBox* Box, const TArray<FInventoryItemUI>& Items)
{
	if (Box == nullptr)
	{
		return;
	}
	Box->ClearChildren();

	for (const FInventoryItemUI& Item : Items)
	{
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (Card == nullptr)
		{
			continue;
		}

		if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			if (UTexture2D* Tex = ResolveInventoryIcon(Item))
			{
				Icon->SetBrushFromTexture(Tex, false);
			}
			Icon->SetDesiredSizeOverride(FVector2D(96.f, 96.f));
			if (Item.mRarityColor != FLinearColor::White)
			{
				Icon->SetColorAndOpacity(Item.mRarityColor);
			}
			Card->AddChildToVerticalBox(Icon);
		}

		if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Name->SetText(Item.mName);
			Name->SetJustification(ETextJustify::Center);
			if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
			{
				NameSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
				NameSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Card))
		{
			BoxSlot->SetPadding(FMargin(10.f, 0.f));
		}
	}
}

/** @brief 현재 모델의 메타(골드/레벨/HP)와 보유 목록을 BindWidget 위젯에 반영한다. */
void UInventoryUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FInventoryUI& Inv = mUIModel->GetInventory();

	if (mMetaText != nullptr)
	{
		mMetaText->SetText(FText::FromString(FString::Printf(
			TEXT("골드 %d    Lv %d    HP %.0f / %.0f"), Inv.mGold, Inv.mLevel, Inv.mHP, Inv.mMaxHP)));
	}

	FillSection(mDiceBox, Inv.mDice);
	FillSection(mSkillBox, Inv.mSkills);
	FillSection(mEquipBox, Inv.mEquipment);
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따른다. */
void UInventoryUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}
