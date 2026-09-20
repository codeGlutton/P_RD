#include "UI/Combat/SimulationPreviewUIModel.h"

/** @brief 새 미리보기를 연다. 세대를 올려 낡은 결과 유입을 막고, 이전 payload를 비운다. */
int32 USimulationPreviewUIModel::BeginPreview()
{
	++mGeneration;
	mIsActive = true;
	ResetPreviewPayloads();
	return mGeneration;
}

/** @brief 미리보기 플로팅 로그 배치를 교체하고 Events 도메인 갱신을 알린다. */
void USimulationPreviewUIModel::SetPreviewEventBatch(int32 Generation, const TArray<FCombatFloatingLogRequest>& Requests)
{
	// 낡은 세대의 결과다 — 이미 다른 미리보기가 열렸거나 통째로 버려졌으니 무시한다.
	if (mIsActive == false || Generation != mGeneration)
	{
		return;
	}

	mPreviewEventBatch.mSource = ECombatEventDataSourceUI::SimulationPreview;
	++mPreviewEventBatch.mRevision;
	mPreviewEventBatch.mFloatingLogs = Requests;

	OnPreviewEventBatch.Broadcast(mPreviewEventBatch);
	OnPreviewChanged.Broadcast(ESimulationPreviewDomainUI::Events);
}

/** @brief 유닛별 예측 요약을 교체하고 Units 도메인 갱신을 알린다. */
void USimulationPreviewUIModel::SetPredictedUnits(int32 Generation, const TArray<FUnitPredictionUI>& Predictions)
{
	if (mIsActive == false || Generation != mGeneration)
	{
		return;
	}

	mPredictedUnits = Predictions;
	OnPreviewChanged.Broadcast(ESimulationPreviewDomainUI::Units);
}

/** @brief 미리보기 행동의 AP 예정 소모를 교체하고 PendingAction 도메인 갱신을 알린다. */
void USimulationPreviewUIModel::SetPreviewPendingAction(int32 Generation, const FCombatPendingActionUI& PendingAction)
{
	if (mIsActive == false || Generation != mGeneration)
	{
		return;
	}

	mPreviewPendingAction = PendingAction;
	OnPreviewChanged.Broadcast(ESimulationPreviewDomainUI::PendingAction);
}

/**
 * @brief 미리보기를 통째로 버린다. USimulationSubsystem이 시뮬 컨텍스트만 비우는 것
 *        (mSimulationRoomContext.mRoomInstance = nullptr)과 같은 모양이다.
 */
void USimulationPreviewUIModel::ClearPreview()
{
	// 열려 있지 않으면 알림도 내지 않는다 — 취소 경로가 겹쳐도 Cleared는 한 번만 나간다.
	if (mIsActive == false)
	{
		return;
	}

	mIsActive = false;
	++mGeneration;   // 비행 중인 낡은 Set*() 결과가 빈 자리를 다시 채우지 못하게 한다.
	ResetPreviewPayloads();

	OnPreviewCleared.Broadcast();
	OnPreviewChanged.Broadcast(ESimulationPreviewDomainUI::All);
}

/** @brief payload만 초기값으로 되돌린다. mRevision은 변경 횟수라 유지한다. */
void USimulationPreviewUIModel::ResetPreviewPayloads()
{
	mPreviewEventBatch.mSource = ECombatEventDataSourceUI::None;
	mPreviewEventBatch.mFloatingLogs.Reset();
	mPredictedUnits.Reset();
	mPreviewPendingAction = FCombatPendingActionUI();
}
