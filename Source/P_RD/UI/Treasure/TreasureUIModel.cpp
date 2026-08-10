/*****************************************************************//**
 * @file   TreasureUIModel.cpp
 * @brief  보물방 화면 뷰모델 구현
 * @author 이문환
 * @date   2026-08-04
 *********************************************************************/

#include "UI/Treasure/TreasureUIModel.h"

/** @details 개봉 의도만 구독자(게임플레이)에게 중계. 보상 지급은 게임플레이가 처리 */
void UTreasureUIModel::RequestOpen()
{
	OnOpenRequested.Broadcast();
}

/** @details 나가기 의도만 전달. 실제 화면 전환은 게임플레이가 결정 */
void UTreasureUIModel::RequestLeave()
{
	OnLeaveRequested.Broadcast();
}

/** @details 보물방 스냅샷을 저장하고 보상 도메인 변경 알림 발신. 개봉 후 게임플레이가 재호출해 상태 갱신 */
void UTreasureUIModel::SetTreasure(const FTreasureUI& Treasure)
{
	mTreasure = Treasure;
	OnUIChanged.Broadcast(ETreasureUIDomain::Reward);
}
