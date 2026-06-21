#include "UI/FrontendMapGraphWidgets.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	/** @brief 방 타입별 맵 노드 토큰 텍스처 경로(P_RD_codex_img에서 SVN으로 임포트). None이면 nullptr. */
	const TCHAR* MapNodeIconPath(ERoomType RoomType)
	{
		switch (RoomType)
		{
		case ERoomType::Treasure:     return TEXT("/Game/SVN/InSideAsset/UI/Tex/Nodes/T_Node_Treasure.T_Node_Treasure");
		case ERoomType::Shop:         return TEXT("/Game/SVN/InSideAsset/UI/Tex/Nodes/T_Node_Shop.T_Node_Shop");
		case ERoomType::Monster:      return TEXT("/Game/SVN/InSideAsset/UI/Tex/Nodes/T_Node_Monster.T_Node_Monster");
		case ERoomType::EliteMonster: return TEXT("/Game/SVN/InSideAsset/UI/Tex/Nodes/T_Node_Elite.T_Node_Elite");
		case ERoomType::BossMonster:  return TEXT("/Game/SVN/InSideAsset/UI/Tex/Nodes/T_Node_Boss.T_Node_Boss");
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

	if (NodePanel != nullptr)
	{
		// 방 타입 토큰 텍스처가 있으면 노드 배경을 그 아이콘으로(흰 틴트=원색). 없으면 기존 상태 색을 유지.
		// 상태(잠금/선택/진입가능)는 NodeTypeStripe 색·라벨 색·버튼 활성으로 계속 표현된다.
		const TCHAR* IconPath = MapNodeIconPath(RoomType);
		UTexture2D* Icon = (IconPath != nullptr) ? LoadObject<UTexture2D>(nullptr, IconPath) : nullptr;
		if (Icon != nullptr)
		{
			NodePanel->SetBrushFromTexture(Icon);
			NodePanel->SetBrushColor(FLinearColor::White);
		}
		else
		{
			NodePanel->SetBrushColor(PanelColor);
		}
	}
	if (NodeTypeStripe != nullptr)
	{
		NodeTypeStripe->SetBrushColor(TypeStripeColor);
	}
	if (NodeLabelText != nullptr)
	{
		NodeLabelText->SetText(Label);
		NodeLabelText->SetColorAndOpacity(LabelColor);
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
