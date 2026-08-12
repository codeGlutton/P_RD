#include "UI/FrontendMapGraphWidgets.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"



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

void UFrontendMapLineWidget::SetLineStyle(bool bIsOpenPath, bool bIsTraversed)
{
	// 지나온 길은 가장 밝게, 지금 갈 수 있는 길은 금색, 잠긴 길은 어두운 황동색으로 읽힌다.
	const FLinearColor Tint = bIsOpenPath
		? (bIsTraversed ? mTraversedTint : mOpenTint)
		: mLockedTint;
	const FLinearColor GlowTint = bIsOpenPath ? mOpenGlowTint : mLockedGlowTint;
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
		if (LineGlowImage != nullptr)
		{
			LineGlowImage->SetBrush(LineBrush);
			LineGlowImage->SetColorAndOpacity(GlowTint);
			LineGlowImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
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
	if (LineGlowImage != nullptr)
	{
		LineGlowImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetLineColor(Tint);
}

float UFrontendMapLineWidget::GetLineThickness() const
{
	return FMath::Max(1.f, mLineThickness);
}

UFrontendMapNodeWidget::UFrontendMapNodeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> TreasureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_MapNode_Treasure.T_MapNode_Treasure"));
	if (TreasureFinder.Succeeded()) mFallbackIconTreasure = TreasureFinder.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> ShopFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_MapNode_Shop.T_MapNode_Shop"));
	if (ShopFinder.Succeeded()) mFallbackIconShop = ShopFinder.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> MonsterFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_MapNode_Monster.T_MapNode_Monster"));
	if (MonsterFinder.Succeeded()) mFallbackIconMonster = MonsterFinder.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> EliteFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_MapNode_Elite.T_MapNode_Elite"));
	if (EliteFinder.Succeeded()) mFallbackIconElite = EliteFinder.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> BossFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Run/WorldMap/Nodes/T_MapNode_Boss.T_MapNode_Boss"));
	if (BossFinder.Succeeded()) mFallbackIconBoss = BossFinder.Object;
}

UTexture2D* UFrontendMapNodeWidget::GetTypeIconTexture(ERoomType RoomType) const
{
	// 시안 빌더가 주입한 클래스 디폴트가 정본, 하드코딩 경로는 빌더 미실행 시 폴백(생성자에서 로드한 텍스처로 대체).
	UTexture2D* Icon = nullptr;
	switch (RoomType)
	{
	case ERoomType::Treasure:     Icon = mIconTreasureTexture ? mIconTreasureTexture.Get() : mFallbackIconTreasure.Get(); break;
	case ERoomType::Shop:         Icon = mIconShopTexture ? mIconShopTexture.Get() : mFallbackIconShop.Get(); break;
	case ERoomType::Monster:      Icon = mIconMonsterTexture ? mIconMonsterTexture.Get() : mFallbackIconMonster.Get(); break;
	case ERoomType::EliteMonster: Icon = mIconEliteTexture ? mIconEliteTexture.Get() : mFallbackIconElite.Get(); break;
	case ERoomType::BossMonster:  Icon = mIconBossTexture ? mIconBossTexture.Get() : mFallbackIconBoss.Get(); break;
	default: break;
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

float UFrontendMapNodeWidget::GetVisualScale(
	ERoomType RoomType, EMapRoomState RoomState, bool bIsCurrentRoom) const
{
	return 1.f;
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

	FWidgetTransform VisualTransform = GetRenderTransform();
	const float VisualScale = GetVisualScale(RoomType, RoomState, bIsCurrentRoom);
	VisualTransform.Scale = FVector2D(VisualScale, VisualScale);
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SetRenderTransform(VisualTransform);

	// 아이콘 위주 노드: 타입 아이콘이 주인공. 상태는 링 텍스처(시안)가 1순위, 없으면 상태색 프레임 폴백.
	bool bHasIcon = false;
	if (NodePanel != nullptr)
	{
		UTexture2D* Icon = GetTypeIconTexture(RoomType);
		if (Icon != nullptr)
		{
			NodePanel->SetBrushFromTexture(Icon);
			// 잠김은 자물쇠 링이 이미 표현하므로 아이콘은 살짝만 누른다. 값은 시안(nodeStyle.lockedIconTint)이 정본.
			NodePanel->SetBrushColor(RoomState == EMapRoomState::Locked && !bIsCurrentRoom
				? mLockedIconTint
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
		/*
		 * SetIsEnabled(false)는 Slate가 버튼 자식(아이콘/링)까지 자동으로 흐리게 렌더한다.
		 * 잠김/선택불가 표현은 링 텍스처와 틴트가 담당하므로, 여기서는 입력만 차단하고 시각은 원색을 유지한다.
		 */
		NodeButton->SetIsEnabled(true);
		NodeButton->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	}
}

void UFrontendMapNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NodeButton != nullptr)
	{
		// 타입 아이콘과 상태 링 자체가 노드의 전체 외형이다. UButton 기본
		// 브러시를 남기면 원형 노드 뒤에 회색 사각형이 비치므로 클릭 영역만
		// 유지하고 상태별 버튼 배경은 그리지 않는다.
		FSlateBrush TransparentBrush;
		TransparentBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);

		FButtonStyle TransparentButtonStyle = NodeButton->GetStyle();
		TransparentButtonStyle.SetNormal(TransparentBrush);
		TransparentButtonStyle.SetHovered(TransparentBrush);
		TransparentButtonStyle.SetPressed(TransparentBrush);
		TransparentButtonStyle.SetDisabled(TransparentBrush);
		NodeButton->SetStyle(TransparentButtonStyle);

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
