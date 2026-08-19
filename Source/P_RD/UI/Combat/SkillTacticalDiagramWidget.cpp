#include "UI/Combat/SkillTacticalDiagramWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "SkillTacticalDiagram"

namespace
{
	// 이 WBP는 상세 칠판 오른쪽 실제 정보면(872x563)을 그대로 차지한다.
	// 600 정사각판을 중앙에서 45도 회전하고 Y를 64% 압축하면 약
	// 849x543의 다이아몬드가 되어 네 변에 10~15px 정도만 남긴다.
	const FVector2D BoardOrigin(136.f, -18.5f);
	constexpr float BoardCenter = 300.f;
	constexpr float BoardTiltY = .64f;
	constexpr float InvSqrtTwo = .70710678118f;

	constexpr TCHAR NeutralTilePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Disabled.T_KitA_Cell_Disabled");
	constexpr TCHAR SelectTilePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Disabled.T_KitA_Cell_Disabled");
	constexpr TCHAR EffectTilePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Selected.T_KitA_Cell_Selected");
}

void USkillTacticalDiagramWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheWidgetParts();
	EnsureVisualAssets();
	if (mSelectButton != nullptr)
	{
		mSelectButton->OnClicked.AddUniqueDynamic(
			this, &USkillTacticalDiagramWidget::HandleSelectRangeClicked);
	}
	if (mEffectButton != nullptr)
	{
		mEffectButton->OnClicked.AddUniqueDynamic(
			this, &USkillTacticalDiagramWidget::HandleEffectRangeClicked);
	}
	RefreshDiagram();
}

void USkillTacticalDiagramWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	CacheWidgetParts();
	EnsureVisualAssets();
	if (mHasDetail == false)
	{
		mDetail.mSkillIndex = 0;
		mDetail.mTargeting.mSelectShape = ECombatSkillSelectShapeUI::Cross;
		mDetail.mTargeting.mSelectRange = 2.f;
		mDetail.mTargeting.mHitShape = ECombatSkillHitShapeUI::Circle;
		mDetail.mTargeting.mHitRange = 2.f;
	}
	RefreshDiagram();
}

void USkillTacticalDiagramWidget::ApplySkillDetail(const FSkillDetailUI& Detail)
{
	mDetail = Detail;
	mHasDetail = true;
	mPreviewMode = EPreviewMode::Hidden;
	CacheWidgetParts();
	EnsureVisualAssets();
	RefreshDiagram();
	mPreviewVisibilityChanged.Broadcast(false);
}

void USkillTacticalDiagramWidget::ShowSelectRangePreview()
{
	SetPreviewMode(mPreviewMode == EPreviewMode::SelectRange
		? EPreviewMode::Hidden : EPreviewMode::SelectRange);
}

void USkillTacticalDiagramWidget::ShowEffectRangePreview()
{
	SetPreviewMode(mPreviewMode == EPreviewMode::EffectRange
		? EPreviewMode::Hidden : EPreviewMode::EffectRange);
}

void USkillTacticalDiagramWidget::HideRangePreview()
{
	SetPreviewMode(EPreviewMode::Hidden);
}

void USkillTacticalDiagramWidget::SetPreviewMode(const EPreviewMode NewMode)
{
	const bool bWasVisible = mPreviewMode != EPreviewMode::Hidden;
	mPreviewMode = NewMode;
	RefreshDiagram();
	const bool bIsVisible = mPreviewMode != EPreviewMode::Hidden;
	if (bWasVisible != bIsVisible)
	{
		mPreviewVisibilityChanged.Broadcast(bIsVisible);
	}
}

void USkillTacticalDiagramWidget::HandleSelectRangeClicked()
{
	ShowSelectRangePreview();
}

void USkillTacticalDiagramWidget::HandleEffectRangeClicked()
{
	ShowEffectRangePreview();
}

void USkillTacticalDiagramWidget::CacheWidgetParts()
{
	if (WidgetTree == nullptr)
	{
		return;
	}
	if (mCells.Num() != RowCount * ColumnCount)
	{
		mCells.Reset();
		for (int32 Row = 0; Row < RowCount; ++Row)
		{
			for (int32 Column = 0; Column < ColumnCount; ++Column)
			{
				mCells.Add(Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(
					TEXT("TacticalCell_R%dC%d"), Row, Column)))));
			}
		}
	}
	if (mRouteDots.Num() != RouteDotCount)
	{
		mRouteDots.Reset();
		for (int32 Index = 0; Index < RouteDotCount; ++Index)
		{
			mRouteDots.Add(Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(
				TEXT("TacticalRouteDot_%d"), Index)))));
		}
	}
	mEffectRing = Cast<UImage>(WidgetTree->FindWidget(TEXT("TacticalEffectRing")));
	mCasterRing = Cast<UImage>(WidgetTree->FindWidget(TEXT("TacticalCasterRing")));
	mCasterMarker = Cast<UImage>(WidgetTree->FindWidget(TEXT("TacticalCasterMarker")));
	mTargetRing = Cast<UImage>(WidgetTree->FindWidget(TEXT("TacticalTargetRing")));
	mTargetMarker = Cast<UImage>(WidgetTree->FindWidget(TEXT("TacticalTargetMarker")));
	mBackdrop = WidgetTree->FindWidget(TEXT("TacticalDiagramBackdrop"));
	mBoardTilt = WidgetTree->FindWidget(TEXT("TacticalBoardTilt"));
	mSelectButton = Cast<UButton>(WidgetTree->FindWidget(
		TEXT("TacticalSelectLegendButton")));
	mEffectButton = Cast<UButton>(WidgetTree->FindWidget(
		TEXT("TacticalEffectLegendButton")));
	mSelectLegend = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TacticalSelectLegendText")));
	mEffectLegend = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TacticalEffectLegendText")));
	// 범위 버튼은 각각의 나무 명패만으로 충분히 구분된다. 두 버튼 뒤를 가로지르는
	// 1px 장식선은 공용 상세판을 다시 위아래로 나눠 보이게 하므로 항상 접는다.
	if (UWidget* LegendRule = WidgetTree->FindWidget(TEXT("TacticalLegendRule")))
	{
		LegendRule->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USkillTacticalDiagramWidget::EnsureVisualAssets()
{
	if (mNeutralTileTexture == nullptr)
	{
		mNeutralTileTexture = LoadObject<UTexture2D>(nullptr, NeutralTilePath);
	}
	if (mSelectTileTexture == nullptr)
	{
		mSelectTileTexture = LoadObject<UTexture2D>(nullptr, SelectTilePath);
	}
	if (mEffectTileTexture == nullptr)
	{
		mEffectTileTexture = LoadObject<UTexture2D>(nullptr, EffectTilePath);
	}
}

bool USkillTacticalDiagramWidget::SelectCovers(const int32 Row,
	const int32 Column, const int32 OriginRow, const int32 OriginColumn,
	const int32 Range, const ECombatSkillSelectShapeUI Shape) const
{
	const int32 DeltaRow = FMath::Abs(Row - OriginRow);
	const int32 DeltaColumn = FMath::Abs(Column - OriginColumn);
	const int32 Manhattan = DeltaRow + DeltaColumn;
	const int32 Chebyshev = FMath::Max(DeltaRow, DeltaColumn);
	switch (Shape)
	{
	case ECombatSkillSelectShapeUI::Cross:
	case ECombatSkillSelectShapeUI::Line:
		return (DeltaRow == 0 || DeltaColumn == 0) && Manhattan <= Range;
	case ECombatSkillSelectShapeUI::Square:
		return Chebyshev <= Range;
	case ECombatSkillSelectShapeUI::Diagonal:
		return Chebyshev <= Range && (DeltaRow == 0 || DeltaColumn == 0
			|| DeltaRow == DeltaColumn);
	case ECombatSkillSelectShapeUI::Single:
	case ECombatSkillSelectShapeUI::None:
	default:
		return DeltaRow == 0 && DeltaColumn == 0;
	}
}

bool USkillTacticalDiagramWidget::HitCovers(const int32 Row, const int32 Column,
	const int32 TargetRow, const int32 TargetColumn, const int32 Range,
	const ECombatSkillHitShapeUI Shape) const
{
	const int32 DeltaRow = FMath::Abs(Row - TargetRow);
	const int32 DeltaColumn = FMath::Abs(Column - TargetColumn);
	switch (Shape)
	{
	case ECombatSkillHitShapeUI::Cross:
		return (DeltaRow == 0 || DeltaColumn == 0)
			&& DeltaRow + DeltaColumn <= Range;
	// 8방향: 직교 축 또는 정확한 대각선(체비셰프 거리 이내).
	// 게임플레이 진실(TileMapModel::GetEffectTiles의 Star)과 같은 모양이다.
	case ECombatSkillHitShapeUI::Star:
		return (DeltaRow == 0 || DeltaColumn == 0 || DeltaRow == DeltaColumn)
			&& FMath::Max(DeltaRow, DeltaColumn) <= Range;
	case ECombatSkillHitShapeUI::Circle:
		return FMath::Max(DeltaRow, DeltaColumn) <= Range;
	case ECombatSkillHitShapeUI::Single:
	case ECombatSkillHitShapeUI::None:
	default:
		return DeltaRow == 0 && DeltaColumn == 0;
	}
}
FVector2D USkillTacticalDiagramWidget::CellCenter(const int32 Row,
	const int32 Column) const
{
	const int32 Index = Row * ColumnCount + Column;
	const UImage* Cell = mCells.IsValidIndex(Index) ? mCells[Index].Get() : nullptr;
	const UCanvasPanelSlot* CanvasSlot = Cell != nullptr
		? Cast<UCanvasPanelSlot>(Cell->Slot) : nullptr;
	if (CanvasSlot == nullptr)
	{
		return FVector2D::ZeroVector;
	}

	// TacticalBoardRotate(45도) -> TacticalBoardTilt(Y 55%) 순서의
	// 렌더 변환을 그대로 계산해, 판 밖에 있는 직립 마커를 셀 중심에 맞춘다.
	const FVector2D LocalCenter = CanvasSlot->GetPosition()
		+ CanvasSlot->GetSize() * .5f;
	const FVector2D FromPivot = LocalCenter - FVector2D(BoardCenter);
	const FVector2D Rotated(
		(FromPivot.X - FromPivot.Y) * InvSqrtTwo + BoardCenter,
		(FromPivot.X + FromPivot.Y) * InvSqrtTwo + BoardCenter);
	const FVector2D Tilted(Rotated.X,
		(Rotated.Y - BoardCenter) * BoardTiltY + BoardCenter);
	return BoardOrigin + Tilted;
}

void USkillTacticalDiagramWidget::PlaceCentered(UWidget* Widget,
	const FVector2D& Center, const FVector2D& Size) const
{
	if (UCanvasPanelSlot* CanvasSlot = Widget != nullptr
		? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
	{
		CanvasSlot->SetPosition(Center - Size * .5f);
		CanvasSlot->SetSize(Size);
	}
}

void USkillTacticalDiagramWidget::RefreshDiagram()
{
	if (mCells.Num() != RowCount * ColumnCount
		|| mNeutralTileTexture == nullptr || mSelectTileTexture == nullptr
		|| mEffectTileTexture == nullptr)
	{
		return;
	}

	const int32 ActualSelectRange = FMath::Max(0,
		FMath::RoundToInt(mDetail.mTargeting.mSelectRange));
	const int32 ActualHitRange = FMath::Max(0,
		FMath::RoundToInt(mDetail.mTargeting.mHitRange));
	const bool bSelfTarget = mDetail.mTargeting.mSelectShape
		== ECombatSkillSelectShapeUI::Single
		|| mDetail.mTargeting.mSelectShape == ECombatSkillSelectShapeUI::None;
	// 범위 모식도는 실제 전장 위치를 흉내 내는 미니맵이 아니라 모양과 거리를
	// 비교하는 설명판이다. 사정 범위는 시전자, 영향 범위는 선택 지점을 각각
	// 9x9 정중앙에 놓아 비대칭 여백 때문에 범위가 왜곡돼 보이지 않게 한다.
	const int32 TargetRow = RowCount / 2;
	const int32 TargetColumn = ColumnCount / 2;
	const int32 PreviewHitRange = FMath::Clamp(ActualHitRange, 0, 2);
	const bool bBoardVisible = mPreviewMode != EPreviewMode::Hidden;
	const bool bSelectMode = mPreviewMode == EPreviewMode::SelectRange;
	const bool bEffectMode = mPreviewMode == EPreviewMode::EffectRange;
	if (mBackdrop != nullptr)
	{
		mBackdrop->SetVisibility(bBoardVisible
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mBoardTilt != nullptr)
	{
		mBoardTilt->SetVisibility(bBoardVisible
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	for (int32 Row = 0; Row < RowCount; ++Row)
	{
		for (int32 Column = 0; Column < ColumnCount; ++Column)
		{
			const int32 Index = Row * ColumnCount + Column;
			UImage* Cell = mCells[Index];
			if (Cell == nullptr)
			{
				continue;
			}
			const bool bEffect = bEffectMode && HitCovers(Row, Column, TargetRow,
				TargetColumn, PreviewHitRange, mDetail.mTargeting.mHitShape);
			const bool bSelect = bSelectMode && SelectCovers(Row, Column,
				CasterRow, CasterColumn, ActualSelectRange,
				mDetail.mTargeting.mSelectShape);
			UTexture2D* Texture = bEffect ? mEffectTileTexture.Get()
				: (bSelect ? mSelectTileTexture.Get() : mNeutralTileTexture.Get());
			Cell->SetBrushFromTexture(Texture, false);
			FSlateBrush Brush = Cell->GetBrush();
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Cell->SetBrush(Brush);
			const FLinearColor CellTint = bEffect
				? FLinearColor(1.f, .72f, .52f, 1.f)
				: (bSelect ? FLinearColor(.12f, .82f, 1.f, 1.f)
					: FLinearColor(.58f, .55f, .50f, .58f));
			Cell->SetColorAndOpacity(CellTint);
		}
	}

	const FVector2D CasterCenter = CellCenter(CasterRow, CasterColumn)
		+ FVector2D(0.f, -12.f);
	const FVector2D TargetCenter = CellCenter(TargetRow, TargetColumn)
		+ FVector2D(0.f, -12.f);
	// 모바일에서 마커가 1칸보다 크게 읽히지 않도록 링을 포함한 플레이어/적
	// 표시 묶음을 기존 크기의 정확히 50%로 축소한다.
	PlaceCentered(mCasterRing, CasterCenter, FVector2D(38.f, 38.f));
	PlaceCentered(mCasterMarker, CasterCenter, FVector2D(25.f, 29.f));
	PlaceCentered(mTargetRing, TargetCenter, FVector2D(38.f, 38.f));
	PlaceCentered(mTargetMarker, TargetCenter, FVector2D(25.f, 29.f));
	if (mCasterRing != nullptr)
	{
		mCasterRing->SetVisibility(bSelectMode
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mCasterMarker != nullptr)
	{
		mCasterMarker->SetVisibility(bSelectMode
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mTargetRing != nullptr)
	{
		mTargetRing->SetVisibility(bEffectMode
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mTargetMarker != nullptr)
	{
		mTargetMarker->SetVisibility(bEffectMode
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (mEffectRing != nullptr)
	{
		mEffectRing->SetVisibility(ESlateVisibility::Collapsed);
	}

	// LineToTarget(PR #466): 영향 시점이 시전자→조준 경로 전체다. 사정 범위
	// 프리뷰에서 시전자 중심에서 조준 방향으로 사거리만큼 경로 점선을 그려
	// "지나가는 칸 전부가 맞는다" 는 것을 알린다. 조준 칸 하나짜리(Default)는
	// 점선이 오해만 부르므로 계속 숨긴다.
	const bool bLineToTarget = mDetail.mTargeting.mTargetPattern
		== ECombatSkillTargetPatternUI::LineToTarget;
	const int32 RayLength = FMath::Clamp(
		ActualSelectRange, 1, ColumnCount - 1 - CasterColumn);
	const FVector2D RayEnd =
		CellCenter(CasterRow, CasterColumn + RayLength) + FVector2D(0.f, -12.f);
	for (int32 Index = 0; Index < mRouteDots.Num(); ++Index)
	{
		UImage* Dot = mRouteDots[Index];
		if (Dot == nullptr)
		{
			continue;
		}
		const bool bShow = bSelectMode && bLineToTarget;
		Dot->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
		if (bShow)
		{
			const float Alpha = float(Index + 1) / float(mRouteDots.Num() + 1);
			PlaceCentered(Dot, FMath::Lerp(CasterCenter, RayEnd, Alpha),
				FVector2D(6.f, 6.f));
		}
	}

	if (mSelectLegend != nullptr)
	{
		mSelectLegend->SetText(bSelfTarget
			? LOCTEXT("SelectLegendSelf", "자신을 중심으로 발동")
			: FText::Format(LOCTEXT("SelectLegendRange", "사정 범위  {0}칸"),
				FText::AsNumber(ActualSelectRange)));
	}
	if (mEffectLegend != nullptr)
	{
		mEffectLegend->SetText(ActualHitRange <= 0
			? LOCTEXT("EffectLegendSingle", "영향 범위  대상 1칸")
			: FText::Format(LOCTEXT("EffectLegendRange", "영향 범위  {0}칸"),
				FText::AsNumber(ActualHitRange)));
	}
	if (mSelectButton != nullptr)
	{
		mSelectButton->SetBackgroundColor(bSelectMode
			? FLinearColor(.08f, .48f, .92f, 1.f)
			: FLinearColor(.30f, .25f, .18f, .92f));
	}
	if (mEffectButton != nullptr)
	{
		mEffectButton->SetBackgroundColor(bEffectMode
			? FLinearColor(.78f, .18f, .06f, 1.f)
			: FLinearColor(.30f, .25f, .18f, .92f));
	}
}

#undef LOCTEXT_NAMESPACE
