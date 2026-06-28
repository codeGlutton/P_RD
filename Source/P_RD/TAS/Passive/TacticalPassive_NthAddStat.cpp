/*****************************************************************//**
 * @file   TacticalPassive_NthAddStat.cpp
 * @brief  데이터 구동 N번째 발동마다 스탯 가산 패시브 구현
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_NthAddStat.h"
#include "TAS/Passive/DynamicPassiveData_NthCounter.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"

void UTacticalPassive_NthAddStat::EvaluatePassive(
	const FPassiveActivateContext& Ctx,
	FBoardCombatTargetSnapshotData& TargetDelta,
	TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 데이터/이펙트가 없으면 기여 없음
	if (mStaticData == nullptr || mEffectClass == nullptr)
	{
		return;
	}
	const UTacticalEffect* EffectCDO = mEffectClass.GetDefaultObject();
	if (EffectCDO == nullptr || EffectCDO->mModifiers.Num() == 0)
	{
		return;
	}

	// 러닝 상태를 커밋된 상태로 시드 (커밋된 게 없으면 새로 생성)
	if (!PassiveState.IsValid())
	{
		if (mState.IsValid())
		{
			PassiveState = mState;
		}
		else
		{
			PassiveState.InitializeAs<FDynamicPassiveData_NthCounter>();
		}
	}

	// 러닝본(NextState 버퍼). mState가 아니라 드라이버가 든 작업 복사본
	FDynamicPassiveData_NthCounter& Running = PassiveState.GetMutable<FDynamicPassiveData_NthCounter>();

	// 완료 횟수(과거 기록)는 읽기만. 이번 차수는 +1 한 로컬 값으로 판단
	const int32 CompletedCount = Running.mCount;
	const int32 Ordinal = CompletedCount + 1;

	// 이번 차수가 임계면 이펙트 속성에 보너스 누적
	const bool bTriggered = (Ordinal >= mStaticData->mThreshold);
	if (bTriggered)
	{
		TargetDelta.mAttributes.FindOrAdd(EffectCDO->mModifiers[0].mAttribute) += mStaticData->mMagnitude;
	}

	// 다음 상태(NextState) 기록: 발동이면 사이클 리셋(0), 아니면 완료 횟수 전진
	Running.mCount = bTriggered ? 0 : Ordinal;
}

void UTacticalPassive_NthAddStat::CommitPassive(
	const TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 러닝본을 그대로 커밋 (리셋은 Evaluate가 NextState에 이미 반영)
	mState = PassiveState;
}
