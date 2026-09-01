/*****************************************************************//**
 * @file   TacticalPassive_Generic.cpp
 * @brief  데이터 기반 제네릭 패시브 구현
 * @author 이문환
 * @date   2026-09-01
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_Generic.h"

#include "P_RD.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData_Generic.h"
#include "TAS/Passive/PassiveCondition.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"

void UTacticalPassive_Generic::InitializeState(TInstancedStruct<FDynamicPassiveData>& PassiveState) const
{
	// 제네릭 상태(카운터 + 캡처값)로 초기화
	PassiveState.InitializeAs<FDynamicPassiveData_Generic>();
}

void UTacticalPassive_Generic::OnCounterReset(TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 카운터만 0으로 초기화 (캡처값은 유지)
	FDynamicPassiveData_Generic* State = PassiveState.GetMutablePtr<FDynamicPassiveData_Generic>();
	if (State != nullptr)
	{
		State->mCounter = 0;
	}
}

void UTacticalPassive_Generic::OnActivate(const FPassiveActivateContext& Ctx, TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 데이터나 상태가 없으면 발동할 수 없음
	if (mStaticData == nullptr)
	{
		return;
	}
	FDynamicPassiveData_Generic* State = PassiveState.GetMutablePtr<FDynamicPassiveData_Generic>();
	if (State == nullptr)
	{
		return;
	}

	// 카운터는 트리거 횟수이므로 조건 통과 여부와 무관하게 증가
	State->mCounter += 1;

	// 수량 조건 게이트
	if (PassesTargetQuantifier(Ctx, PassiveState) == false)
	{
		return;
	}

	// 효과 수치를 먼저 전부 계산해 실제 적용할 효과만 추림
	struct FResolvedEffect
	{
		TSubclassOf<UTacticalEffect> mEffectClass;
		float mMagnitude = 0.f;
	};
	TArray<FResolvedEffect> ResolvedEffects;
	for (const FPassiveEffectEntry& Entry : mStaticData->mEffects)
	{
		float Magnitude = 0.f;
		if (Entry.mMagnitude.Resolve(Ctx, 0, *State, Magnitude) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("패시브 효과 수치 계산 실패: %s"), *GetNameSafe(mStaticData));
			continue;
		}
		if (FMath::IsNearlyZero(Magnitude))
		{
			continue;
		}
		TSubclassOf<UTacticalEffect> EffectClass = Entry.mEffectClass.LoadSynchronous();
		if (EffectClass == nullptr)
		{
			continue;
		}
		ResolvedEffects.Add({ EffectClass, Magnitude });
	}

	// 적용할 효과가 없으면 이전 배치도 그대로 유지
	if (ResolvedEffects.Num() == 0)
	{
		return;
	}

	// 재발동은 갱신이므로 이전 배치를 먼저 제거
	DeactivatePassive();

	// 효과마다 이펙트 적용
	for (const FResolvedEffect& Resolved : ResolvedEffects)
	{
		NotifyPassive(Ctx, Resolved.mEffectClass, Resolved.mMagnitude, mStaticData->mEffectTarget, false);
	}
}

bool UTacticalPassive_Generic::IsTargetQualified(const FPassiveActivateContext& Ctx, int32 TargetIndex, const TInstancedStruct<FDynamicPassiveData>& State) const
{
	// 데이터가 없으면 조건도 없으므로 통과
	if (mStaticData == nullptr)
	{
		return true;
	}

	// 상태가 아직 없으면 빈 상태로 판정
	static const FDynamicPassiveData_Generic EmptyState;
	const FDynamicPassiveData_Generic* Generic = State.GetPtr<FDynamicPassiveData_Generic>();

	// DA에 정의된 조건으로 자격 판정
	return PassiveConditionUtils::EvaluateAll(mStaticData->mConditions, Ctx, TargetIndex, Generic != nullptr ? *Generic : EmptyState);
}

void UTacticalPassive_Generic::OnCapture(const FPassiveActivateContext& Ctx, TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 데이터나 상태가 없으면 캡처할 수 없음
	if (mStaticData == nullptr)
	{
		return;
	}
	FDynamicPassiveData_Generic* State = PassiveState.GetMutablePtr<FDynamicPassiveData_Generic>();
	if (State == nullptr)
	{
		return;
	}

	// 캡처 엔트리마다 Self 값과 타겟별 값을 모두 평가해 키로 저장
	// (읽는 쪽(Captured)의 출처(Source)가 어느 쪽을 쓸지 고르므로 양쪽 다 저장)
	for (const FPassiveCaptureEntry& Entry : mStaticData->mCaptureOperands)
	{
		// 출처를 강제로 바꿔가며 평가하기 위한 복사본
		FPassiveOperand Operand = Entry.mOperand;
		FPassiveCaptureSlot Slot;

		// Self 값
		Operand.mSource = EPassiveOperandSource::Self;
		bool bResolved = Operand.Resolve(Ctx, INDEX_NONE, *State, Slot.mSelf);

		// 타겟별 값 (mTargets 인덱스와 짝)
		Operand.mSource = EPassiveOperandSource::Target;
		for (int32 Index = 0; bResolved && Index < Ctx.mTargets.Num(); ++Index)
		{
			float Value = 0.f;
			bResolved = Operand.Resolve(Ctx, Index, *State, Value);
			Slot.mTargets.Add(Value);
		}

		// 하나라도 실패하면 키 자체를 저장하지 않음 → 발동 시 Captured 조회 실패 = 조건 불통과
		if (bResolved)
		{
			State->mCaptures.Add(Entry.mKey, Slot);
		}
		else
		{
			UE_LOG(LogRD, Warning, TEXT("패시브 캡처 실패: %s (키 %s)"), *GetNameSafe(mStaticData), *Entry.mKey.ToString());
		}
	}

	// 타일 위치는 항상 저장 (이동 거리 판정용). 스냅샷 없으면 Invalid
	State->mCapturedSelfTile = (Ctx.mOwnerSnapshot != nullptr) ? Ctx.mOwnerSnapshot->mTileTransform.mIndex : FTileIndex::Invalid;
	State->mCapturedTargetTiles.Reset();
	for (int32 Index = 0; Index < Ctx.mTargets.Num(); ++Index)
	{
		const UBoardCombatTargetSnapshotData* Snapshot = Ctx.mTargetSnapshots.IsValidIndex(Index) ? Ctx.mTargetSnapshots[Index] : nullptr;
		State->mCapturedTargetTiles.Add(Snapshot != nullptr ? Snapshot->mTileTransform.mIndex : FTileIndex::Invalid);
	}
}

void UTacticalPassive_Generic::CommitPassive(const TInstancedStruct<FDynamicPassiveData>& PassiveState)
{
	// 발동이 안됐다면 갱신하지 않고 무시
	if (!PassiveState.IsValid())
	{
		return;
	}

	// 내부상태 갱신
	mState = PassiveState;
}
