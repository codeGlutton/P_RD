#include "UI/FrontendMapGraphWidgets.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
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

void UFrontendMapLineWidget::SetLineStyle(bool bIsOpenPath, const FLinearColor& Tint)
{
	UTexture2D* PathTexture = bIsOpenPath ? mSolidTexture.Get() : mDashedTexture.Get();
	if (LineImage != nullptr && PathTexture != nullptr)
	{
		// 경로 텍스처를 길이 방향으로 타일링해 이어 붙인다(늘리면 문양이 뭉개진다).
		FSlateBrush LineBrush;
		LineBrush.DrawAs = ESlateBrushDrawType::Image;
		LineBrush.Tiling = ESlateBrushTileType::Horizontal;
		LineBrush.ImageSize = FVector2D(PathTexture->GetSizeX(), PathTexture->GetSizeY());
		LineBrush.SetResourceObject(PathTexture);
		LineImage->SetBrush(LineBrush);
		LineImage->SetColorAndOpacity(Tint);
		LineImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (LinePanel != nullptr)
		{
			LinePanel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
		}
		return;
	}

	// 텍스처 미주입(빌더 미실행) 시 기존 단색 Border 폴백.
	if (LineImage != nullptr)
	{
		LineImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetLineColor(Tint);
}

float UFrontendMapLineWidget::GetLineThickness() const
{
	return FMath::Max(1.f, mLineThickness);
}

UTexture2D* UFrontendMapNodeWidget::GetTypeIconTexture(ERoomType RoomType) const
{
	// 시안 빌더가 주입한 클래스 디폴트가 정본, 하드코딩 경로는 빌더 미실행 시 폴백.
	UTexture2D* Icon = nullptr;
	switch (RoomType)
	{
	case ERoomType::Treasure:     Icon = mIconTreasureTexture; break;
	case ERoomType::Shop:         Icon = mIconShopTexture; break;
	case ERoomType::Monster:      Icon = mIconMonsterTexture; break;
	case ERoomType::EliteMonster: Icon = mIconEliteTexture; break;
	case ERoomType::BossMonster:  Icon = mIconBossTexture; break;
	default: break;
	}
	if (Icon == nullptr)
	{
		const TCHAR* IconPath = MapNodeIconPath(RoomType);
		Icon = (IconPath != nullptr) ? LoadObject<UTexture2D>(nullptr, IconPath) : nullptr;
	}
	return Icon;
}

UTexture2D* UFrontendMapNodeWidget::GetStateRingTexture(EMapRoomState RoomState, bool bIsCurrentRoom) const
{
	if (bIsCurrentRoom)
	{
		return mRingCurrentTexture;
	}
	switch (RoomState)
	{
	case EMapRoomState::Selected:
	case EMapRoomState::Ready:    return mRingNormalTexture;
	case EMapRoomState::Cleared:  return mRingClearedTexture;
	default:                      return mRingLockedTexture;
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
	ERoomType RoomType,
	EMapRoomState RoomState,
	bool bIsCurrentRoom)
{
	mRowIndex = InRowIndex;
	mColumnIndex = InColumnIndex;

	// 아이콘 위주 노드: 타입 아이콘이 주인공. 상태는 링 텍스처(시안)가 1순위, 없으면 상태색 프레임 폴백.
	bool bHasIcon = false;
	if (NodePanel != nullptr)
	{
		UTexture2D* Icon = GetTypeIconTexture(RoomType);
		if (Icon != nullptr)
		{
			NodePanel->SetBrushFromTexture(Icon);
			// 잠김은 자물쇠 링이 이미 표현하므로 아이콘은 살짝만 눌러 원색을 유지한다(과한 감쇠는 씻겨 보임).
			NodePanel->SetBrushColor(RoomState == EMapRoomState::Locked && !bIsCurrentRoom
				? FLinearColor(0.8f, 0.8f, 0.8f, 1.f)
				: FLinearColor::White);
			bHasIcon = true;
		}
		else
		{
			NodePanel->SetBrushColor(PanelColor);            // 아이콘 누락 시 기존 상태색 폴백
		}
	}

	UTexture2D* RingTexture = GetStateRingTexture(RoomState, bIsCurrentRoom);
	const bool bHasRing = NodeRingImage != nullptr && RingTexture != nullptr;
	if (NodeRingImage != nullptr)
	{
		if (bHasRing)
		{
			NodeRingImage->SetBrushFromTexture(RingTexture, false);
			NodeRingImage->SetColorAndOpacity(FLinearColor::White);
			NodeRingImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			NodeRingImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (NodeTypeStripe != nullptr)
	{
		// 링이 상태를 말하면 색 프레임은 완전히 숨긴다. 링이 없을 때만 기존 상태색 프레임 폴백.
		FLinearColor StripeColor = TypeStripeColor;
		if (bHasRing)
		{
			StripeColor.A = 0.f;
		}
		else if (bHasIcon)
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
