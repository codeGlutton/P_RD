#include "UI/FrontendMapGraphWidgets.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	/** @brief 방 타입별 맵 노드 토큰 텍스처 경로(작업용 이미지에서 SVN으로 임포트). None이면 nullptr. */
	const TCHAR* MapNodeIconPath(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Treasure:     return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Treasure.T_MapNode_Treasure");
		case ERoomType::Shop:         return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Shop.T_MapNode_Shop");
		case ERoomType::Monster:      return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Monster.T_MapNode_Monster");
		case ERoomType::EliteMonster: return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Elite.T_MapNode_Elite");
		case ERoomType::BossMonster:  return TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Boss.T_MapNode_Boss");
		default:                      return nullptr;
		}
	}
}

void UFrontendMapNodeButton::SetNodeCoordinates(int32 InRowIndex, int32 InColumnIndex)
{
	mRowIndex = InRowIndex;
	mColumnIndex = InColumnIndex;

	if (!mClickBound)
	{
		OnClicked.AddUniqueDynamic(this, &UFrontendMapNodeButton::HandleClicked);
		mClickBound = true;
	}
}

void UFrontendMapNodeButton::HandleClicked()
{
	OnMapNodeClicked.Broadcast(mRowIndex, mColumnIndex);
}

void UFrontendMapLineWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (LinePanel == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapLineWidget: LinePanel is not connected. WBP_FrontendMapLine must provide a Border named LinePanel."));
	}
}

void UFrontendMapLineWidget::SetLineColor(const FLinearColor& InColor)
{
	if (LinePanel != nullptr)
	{
		LinePanel->SetBrushColor(InColor);
	}
}

void UFrontendMapNodeWidget::SetNodeVisual(
	int32 InRowIndex,
	int32 InColumnIndex,
	const FText& Label,
	const FText& Badge,
	const FLinearColor& PanelColor,
	const FLinearColor& TypeStripeColor,
	const FSlateColor& LabelColor,
	const FSlateColor& BadgeColor,
	ERoomType RoomType)
{
	mRowIndex = InRowIndex;
	mColumnIndex = InColumnIndex;

	// 아이콘 위주 노드: 타입 아이콘이 주인공. 상태(잠금/선택/현재)는 옅은 상태색 테두리 + 라벨로만 표현.
	bool bHasIcon = false;
	if (NodePanel != nullptr)
	{
		const TCHAR* IconPath = MapNodeIconPath(RoomType);
		UTexture2D* Icon = (IconPath != nullptr) ? LoadObject<UTexture2D>(nullptr, IconPath) : nullptr;
		if (Icon != nullptr)
		{
			NodePanel->SetBrushFromTexture(Icon);
			NodePanel->SetBrushColor(FLinearColor::White);   // 원색 아이콘
			bHasIcon = true;
		}
		else
		{
			NodePanel->SetBrushColor(PanelColor);            // 아이콘 누락 시 기존 상태색 폴백
		}
	}
	if (NodeTypeStripe != nullptr)
	{
		// 아이콘이 있으면 밝은 상태색 프레임을 옅게(아이콘을 덮지 않게). 없으면 기존대로.
		FLinearColor StripeColor = TypeStripeColor;
		if (bHasIcon)
		{
			StripeColor.A *= 0.35f;
		}
		NodeTypeStripe->SetBrushColor(StripeColor);
	}
	if (NodeLabelText != nullptr)
	{
		// 아이콘이 타입을 말하므로 아이콘 노드에선 라벨을 숨겨 깔끔하게(범례가 타입을 안내).
		if (bHasIcon)
		{
			NodeLabelText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			NodeLabelText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			NodeLabelText->SetText(Label);
			NodeLabelText->SetColorAndOpacity(LabelColor);
		}
	}
	if (NodeBadgeText != nullptr)
	{
		NodeBadgeText->SetText(FText::GetEmpty());
		NodeBadgeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFrontendMapNodeWidget::SetNodeEnabled(bool bEnabled) const
{
	if (NodeButton != nullptr)
	{
		NodeButton->SetIsEnabled(bEnabled);
	}
}

void UFrontendMapNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NodeButton != nullptr)
	{
		NodeButton->OnClicked.AddUniqueDynamic(this, &UFrontendMapNodeWidget::HandleNodeButtonClicked);
	}
	else
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapNodeWidget: NodeButton is not connected. WBP_FrontendMapNode must provide a Button named NodeButton."));
	}
}

void UFrontendMapNodeWidget::NativeDestruct()
{
	if (NodeButton != nullptr)
	{
		NodeButton->OnClicked.RemoveDynamic(this, &UFrontendMapNodeWidget::HandleNodeButtonClicked);
	}

	Super::NativeDestruct();
}

void UFrontendMapNodeWidget::HandleNodeButtonClicked()
{
	OnMapNodeClicked.Broadcast(mRowIndex, mColumnIndex);
}
