#include "UI/CombatTileMapHUDWidget.h"

#include "Components/TextBlock.h"

void UCombatTileMapHUDWidget::HandleCombatQueueNodeResolved(FCombatQueueNode Node)
{
	// 행동 큐 한 단위가 해소됨(데미지/힐/상태이상 등). 연속 공격은 노드가 하나씩 들어와 한 칸씩 재생된다.
	// 머리 위 위치 표시는 유닛 월드 좌표가 필요해 게임플레이/월드의 follow-up이고,
	// 여기서는 화면 피드에 라벨(또는 수치)을 잠깐 보여주는 것까지만 한다.
	if (mCombatFeedText == nullptr)
	{
		return;
	}

	FText FeedText = Node.mLabel;
	if (FeedText.IsEmpty() && Node.mAmount != 0)
	{
		FeedText = FText::AsNumber(Node.mAmount);
	}

	mCombatFeedText->SetText(FeedText);
	mCombatFeedText->SetVisibility(FeedText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
