/*****************************************************************//**
 * @file   TreasureUIWidgetBase.cpp
 * @brief  보물방 화면 WBP 베이스 구현
 * @author 이문환
 * @date   2026-08-05
 *********************************************************************/

#include "UI/Treasure/TreasureUIWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Treasure/TreasureUIModel.h"
#include "UI/ViewportZOrderType.h"

#define LOCTEXT_NAMESPACE "TreasureUIWidgetBase"

namespace
{
	/** @brief 보상 종류별 기본 아이콘 텍스처 경로 (SVN 임포트) */
	const TCHAR* TreasureKindIconPath(ETreasureItemKind Kind)
	{
		switch (Kind)
		{
		case ETreasureItemKind::Artifact: return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Equipment/T_equip_weapon_common.T_equip_weapon_common");
		case ETreasureItemKind::Gold:
		default:                          return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Icons/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1");
		}
	}

	/** @brief 카드 아이콘 결정. 데이터 아이콘 우선, 없으면 종류별 기본 아이콘 */
	UTexture2D* ResolveTreasureIcon(const FTreasureItemUI& Item)
	{
		if (Item.mIcon != nullptr)
		{
			return Item.mIcon;
		}
		return LoadObject<UTexture2D>(nullptr, TreasureKindIconPath(Item.mKind));
	}
}

UTreasureUIWidgetBase::UTreasureUIWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/** @brief 버튼 클릭을 연결하고, BindUIModel이 먼저 됐다면 들어온 값을 즉시 그림 */
void UTreasureUIWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (mOpenButton != nullptr)
	{
		mOpenButton->OnClicked.AddUniqueDynamic(this, &UTreasureUIWidgetBase::HandleOpenClicked);
	}
	if (mOpenButtonText != nullptr)
	{
		mOpenButtonText->SetText(LOCTEXT("Open", "Open"));
	}
	if (mCloseButton != nullptr)
	{
		mCloseButton->OnClicked.AddUniqueDynamic(this, &UTreasureUIWidgetBase::HandleCloseClicked);
	}
	if (mCloseButtonText != nullptr)
	{
		mCloseButtonText->SetText(LOCTEXT("Leave", "Leave"));
	}
	if (mTitleText != nullptr)
	{
		mTitleText->SetText(LOCTEXT("Treasure", "Treasure"));
	}

	RefreshView();
}

/** @brief 새 UIModel을 구독하고 이미 들어온 스냅샷도 즉시 한 번 그림 */
void UTreasureUIWidgetBase::BindUIModel(UTreasureUIModel* InUIModel)
{
	if (mUIModel == InUIModel)
	{
		return;
	}
	UnbindUIModel();
	mUIModel = InUIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.AddDynamic(this, &UTreasureUIWidgetBase::HandleUIChanged);
		// 데이터가 BindUIModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그림
		RefreshView();
		OnTreasureRefreshed();
	}
}

/** @brief 현재 UIModel 구독을 해제해 화면 파괴 후 OnUIChanged가 들어오지 않게 함 */
void UTreasureUIWidgetBase::UnbindUIModel()
{
	if (mUIModel != nullptr)
	{
		mUIModel->OnUIChanged.RemoveDynamic(this, &UTreasureUIWidgetBase::HandleUIChanged);
	}
	mUIModel = nullptr;
}

/** @brief WBP의 개봉 입력을 UIModel의 개봉 의도 이벤트로 전달 */
void UTreasureUIWidgetBase::OpenBox()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestOpen();
	}
}

/** @brief WBP의 나가기 입력을 UIModel의 나가기 의도 이벤트로 전달 */
void UTreasureUIWidgetBase::Leave()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
}

void UTreasureUIWidgetBase::HandleOpenClicked()
{
	OpenBox();
}

void UTreasureUIWidgetBase::HandleCloseClicked()
{
	if (mUIModel != nullptr)
	{
		mUIModel->RequestLeave();
	}
	RemoveFromParent();
}

/** @brief UIModel 변경 알림 → 보상 도메인이면 뷰 갱신 후 WBP 구현 이벤트로도 전달 */
void UTreasureUIWidgetBase::HandleUIChanged(ETreasureUIDomain Domain)
{
	if (Domain != ETreasureUIDomain::Reward)
	{
		return;
	}

	RefreshView();
	OnTreasureRefreshed();
}

/** @brief 현재 모델의 상자 상태/보상 카드를 BindWidget 위젯에 반영 */
void UTreasureUIWidgetBase::RefreshView()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	const FTreasureUI& Treasure = mUIModel->GetTreasure();

	// 개봉 버튼은 개봉 전에만 노출
	if (mOpenButton != nullptr)
	{
		mOpenButton->SetVisibility(Treasure.mIsOpened
			? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (mItemBox == nullptr)
	{
		return;
	}
	mItemBox->ClearChildren();

	for (const FTreasureItemUI& Item : Treasure.mItems)
	{
		// 보상 카드 = 세로 박스(아이콘 + 이름 + 수량). 종류별 아이콘 자동 매핑
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (Card == nullptr)
		{
			continue;
		}

		if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
		{
			if (UTexture2D* Tex = ResolveTreasureIcon(Item))
			{
				Icon->SetBrushFromTexture(Tex, false);
			}
			Icon->SetDesiredSizeOverride(FVector2D(112.f, 112.f));
			Card->AddChildToVerticalBox(Icon);
		}

		if (UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			Name->SetText(Item.mName);
			Name->SetJustification(ETextJustify::Center);
			// 등급은 이름 색으로 표시 (골드 카드는 기본 흰색)
			Name->SetColorAndOpacity(FSlateColor(Item.mRarityColor));
			if (UVerticalBoxSlot* NameSlot = Card->AddChildToVerticalBox(Name))
			{
				NameSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
				NameSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		// 수량 라벨 (골드처럼 수량이 있는 카드만)
		if (Item.mAmount > 0)
		{
			if (UTextBlock* Amount = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				Amount->SetText(FText::Format(LOCTEXT("{0} G", "{0} G"), FText::AsNumber(Item.mAmount)));
				Amount->SetJustification(ETextJustify::Center);
				Amount->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.45f, 1.f)));
				if (UVerticalBoxSlot* AmountSlot = Card->AddChildToVerticalBox(Amount))
				{
					AmountSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
					AmountSlot->SetHorizontalAlignment(HAlign_Center);
				}
			}
		}

		if (UHorizontalBoxSlot* BoxSlot = mItemBox->AddChildToHorizontalBox(Card))
		{
			BoxSlot->SetPadding(FMargin(14.f, 0.f));
		}
	}
}

/** @brief 위젯 생명주기 종료 시 UIModel 델리게이트를 먼저 끊고 부모 정리를 따름 */
void UTreasureUIWidgetBase::NativeDestruct()
{
	UnbindUIModel();
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
