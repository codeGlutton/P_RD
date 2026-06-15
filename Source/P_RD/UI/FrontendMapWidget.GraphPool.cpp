#include "UI/FrontendMapWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/FrontendMapViewPolicy.h"

using namespace RDFrontendMap;

FFrontendMapLinePoolEntry* UFrontendMapWidget::AcquireMapLineWidget(int32 LineIndex)
{
	/*
	 * 지도 연결선은 방 개수에 따라 매번 필요한 수가 달라진다.
	 * RefreshMap()마다 새 위젯을 전부 만들면 비용과 GC 부담이 커지므로, 필요한 인덱스까지 풀을 늘리고 재사용한다.
	 */
	if (MapGraphCanvas == nullptr || MapLineWidgetClass == nullptr || LineIndex < 0)
	{
		return nullptr;
	}

	while (mMapLinePool.Num() <= LineIndex)
	{
		FFrontendMapLinePoolEntry NewEntry;
		NewEntry.mLineWidget = CreateWidget<UFrontendMapLineWidget>(this, MapLineWidgetClass);
		if (NewEntry.mLineWidget != nullptr)
		{
			NewEntry.mLineWidget->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
			NewEntry.mLineWidget->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* LineSlot = MapGraphCanvas->AddChildToCanvas(NewEntry.mLineWidget))
			{
				LineSlot->SetAutoSize(false);
				LineSlot->SetZOrder(0);
			}
		}
		mMapLinePool.Add(MoveTemp(NewEntry));
	}

	FFrontendMapLinePoolEntry& Entry = mMapLinePool[LineIndex];
	if (Entry.mLineWidget != nullptr)
	{
		Entry.mLineWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return &Entry;
}

FFrontendMapNodePoolEntry* UFrontendMapWidget::AcquireMapNodeWidget(int32 NodeIndex)
{
	/*
	 * 지도 노드도 선과 동일하게 풀링한다.
	 * 새 노드를 만들 때만 OnMapNodeClicked를 연결하고, 이후 RefreshMap()에서는 데이터와 위치만 다시 적용한다.
	 */
	if (MapGraphCanvas == nullptr || MapNodeWidgetClass == nullptr || NodeIndex < 0)
	{
		return nullptr;
	}

	while (mMapNodePool.Num() <= NodeIndex)
	{
		FFrontendMapNodePoolEntry NewEntry;
		NewEntry.mNodeWidget = CreateWidget<UFrontendMapNodeWidget>(this, MapNodeWidgetClass);
		if (NewEntry.mNodeWidget != nullptr)
		{
			NewEntry.mNodeWidget->SetVisibility(ESlateVisibility::Collapsed);
			NewEntry.mNodeWidget->OnMapNodeClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);

			if (UCanvasPanelSlot* NodeSlot = MapGraphCanvas->AddChildToCanvas(NewEntry.mNodeWidget))
			{
				NodeSlot->SetAutoSize(false);
				NodeSlot->SetSize(FVector2D(MapNodeWidth, MapNodeHeight));
				NodeSlot->SetZOrder(1);
			}
		}
		mMapNodePool.Add(MoveTemp(NewEntry));
	}

	FFrontendMapNodePoolEntry& Entry = mMapNodePool[NodeIndex];
	if (Entry.mNodeWidget != nullptr)
	{
		Entry.mNodeWidget->SetVisibility(ESlateVisibility::Visible);
	}
	return &Entry;
}

void UFrontendMapWidget::HideUnusedMapGraphWidgets(int32 UsedLineCount, int32 UsedNodeCount)
{
	/*
	 * 이전 지도 갱신에서 더 많은 노드/선이 필요했다면 풀 안에 남은 위젯이 있을 수 있다.
	 * 이번 갱신에서 사용하지 않는 나머지는 제거하지 않고 숨겨 다음 갱신 때 재사용한다.
	 */
	for (int32 LineIndex = UsedLineCount; LineIndex < mMapLinePool.Num(); ++LineIndex)
	{
		if (UFrontendMapLineWidget* LineWidget = mMapLinePool[LineIndex].mLineWidget)
		{
			LineWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	for (int32 NodeIndex = UsedNodeCount; NodeIndex < mMapNodePool.Num(); ++NodeIndex)
	{
		if (UFrontendMapNodeWidget* NodeWidget = mMapNodePool[NodeIndex].mNodeWidget)
		{
			NodeWidget->SetNodeEnabled(false);
			NodeWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UFrontendMapWidget::ConfigureMapGraphLayout() const
{
	/*
	 * 지도 그래프는 ScrollBox 안의 고정 크기 Canvas로 다룬다.
	 * 노드 좌표 계산이 고정 캔버스 크기를 기준으로 하므로, WBP의 SizeBox 크기도 매번 같은 값으로 맞춘다.
	 */
	if (MapGraphSize != nullptr)
	{
		MapGraphSize->SetWidthOverride(MapGraphWidth);
		MapGraphSize->SetHeightOverride(MapGraphHeight);
	}
	if (MapScrollBox != nullptr)
	{
		MapScrollBox->SetOrientation(Orient_Vertical);
		MapScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		MapScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
	}
	if (MapGraphCanvas != nullptr)
	{
		MapGraphCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	}
}
