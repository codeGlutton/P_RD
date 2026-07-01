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

bool UTacticalPassive_NthAddStat::EvaluateActivate(
	const FPassiveActivateContext& Ctx,
	TInstancedStruct<FDynamicPassiveData>& PassiveState,
	FBoardCombatTargetSnapshotData& TargetDelta)
{
	// 데이터/이펙트가 없으면 적용 안 함
	if (mStaticData == nullptr || mEffectClass == nullptr)
	{
		return false;
	}
	const UTacticalEffect* EffectCDO = mEffectClass.GetDefaultObject();
	if (EffectCDO == nullptr || EffectCDO->mModifiers.Num() == 0)
	{
		return false;
	}

	// 내부상태 복사본이 없으면 복사하거나 새로 생성 (복사본을 리턴해야 하니까)
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

	// 내부상태 복사본을 구체화
	FDynamicPassiveData_NthCounter& Running = PassiveState.GetMutable<FDynamicPassiveData_NthCounter>();

	// 완료 카운트를 읽고, 현재 카운트는 +1 해서 사용
	const int32 CompletedCount = Running.mCount;
	const int32 Ordinal = CompletedCount + 1;

	// 이번 카운트가 임계치에 도달했으면 이펙트 발동!
	const bool bTriggered = (Ordinal >= mStaticData->mThreshold);
	if (bTriggered)
	{
		TargetDelta.mAttributes.FindOrAdd(EffectCDO->mModifiers[0].mAttribute) += mStaticData->mMagnitude;
	}

	// 다음 내부상태 기록: 발동했으면 리셋해야 하니까 0, 아니면 증가
	Running.mCount = bTriggered ? 0 : Ordinal;

	// 이번 카운트가 임계치에 도달했으면 적용, 아니면 스킵 회신
	return bTriggered;
}

void UTacticalPassive_NthAddStat::CommitPassive(
	const TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 발동이 안됐다면 갱신하지 않고 무시
	if (!PassiveState.IsValid())
	{
		return;
	}

	// 내부상태 갱신
	mState = PassiveState;
}
