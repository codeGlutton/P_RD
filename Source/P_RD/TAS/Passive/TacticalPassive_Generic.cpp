/*****************************************************************//**
 * @file   TacticalPassive_Generic.cpp
 * @brief  데이터 기반 제네릭 패시브 구현
 * @author 이문환
 * @date   2026-09-01
 *********************************************************************/

#include "TAS/Passive/TacticalPassive_Generic.h"

#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData_Generic.h"
#include "TAS/Passive/PassiveCondition.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Actor/BoardActor/BoardActorModel.h"
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

void UTacticalPassive_Generic::OnActivate(const FPassiveActivateContext& InCtx, TInstancedStruct<FDynamicPassiveData>& PassiveState)
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

	// 캡처한 타겟에게 발동하는 패시브면, 모아 둔 타겟을 대상으로 삼고 목록은 비움
	FPassiveActivateContext CapturedCtx;
	if (mStaticData->mActivateOnCapturedTargets)
	{
		CapturedCtx = InCtx;
		CapturedCtx.mTargets = State->mCapturedTargets;
		CapturedCtx.mTargetSnapshots.Reset();

		// 캡처값과 비교할 수 있도록 현재값 저장. 사라진 타겟은 nullptr
		for (const TWeakObjectPtr<UBoardActorModel>& Target : CapturedCtx.mTargets)
		{
			const IBoardCombatTarget* CombatTarget = Cast<IBoardCombatTarget>(Target.Get());
			CapturedCtx.mTargetSnapshots.Add(CombatTarget != nullptr ? CombatTarget->MakeSnapshotData() : nullptr);
		}
		State->mCapturedTargets.Reset();
	}

	// 캡처한 타겟을 대상으로 하면 CapturedCtx를, 아니면 기본값인 InCtx를 사용
	const FPassiveActivateContext& Ctx = mStaticData->mActivateOnCapturedTargets ? CapturedCtx : InCtx;

	// 카운터는 트리거 횟수이므로 조건 통과 여부와 무관하게 증가
	State->mCounter += 1;

	UE_LOG(LogPassive, Verbose, TEXT("발동 시점 진입: %s (소유자 %s, 카운터 %d)"), *GetNameSafe(mStaticData), *GetNameSafe(Ctx.mOwner.Get()), State->mCounter);

	// 수량 조건 게이트
	if (PassesTargetQuantifier(Ctx, PassiveState) == false)
	{
		UE_LOG(LogPassive, Verbose, TEXT("수량 조건 탈락: %s"), *GetNameSafe(mStaticData));
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
			UE_LOG(LogPassive, Warning, TEXT("패시브 효과 수치 계산 실패: %s"), *GetNameSafe(mStaticData));
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
		UE_LOG(LogPassive, Verbose, TEXT("적용할 효과 없음: %s"), *GetNameSafe(mStaticData));
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

	// 캡처한 타겟에게 발동하는 패시브는, 페이즈마다 캡처되는 타겟을 발동 전까지 모음
	const bool bAccumulate = mStaticData->mActivateOnCapturedTargets;
	const bool bFirstCapture = (bAccumulate == false) || State->mCapturedTargets.Num() == 0;

	// 첫 캡처: 키마다 Self 값을 새로 저장. 타겟 값은 아래에서 뒤에 붙임
	// Self와 타겟 값을 모두 저장해서, 읽는 쪽에서 선택해서 사용하게 함
	if (bFirstCapture)
	{
		for (const FPassiveCaptureEntry& Entry : mStaticData->mCaptureOperands)
		{
			// 출처를 Self로 바꿔 평가하기 위한 복사본
			FPassiveOperand Operand = Entry.mOperand;
			Operand.mSource = EPassiveOperandSource::Self;

			// 실패하면 키 자체를 지움 -> 발동 시 Captured 조회 실패: 조건 실패
			FPassiveCaptureSlot Slot;
			if (Operand.Resolve(Ctx, INDEX_NONE, *State, Slot.mSelf))
			{
				State->mCaptures.Add(Entry.mKey, Slot);
			}
			else
			{
				State->mCaptures.Remove(Entry.mKey);
				UE_LOG(LogPassive, Warning, TEXT("패시브 캡처 실패: %s (키 %s)"), *GetNameSafe(mStaticData), *Entry.mKey.ToString());
			}
		}

		// 타일 위치는 항상 저장 (이동 거리 판정용). 스냅샷 없으면 Invalid
		State->mCapturedSelfTile = (Ctx.mOwnerSnapshot != nullptr) ? Ctx.mOwnerSnapshot->mTileTransform.mIndex : FTileIndex::Invalid;
		State->mCapturedTargetTiles.Reset();
		State->mCapturedTargets.Reset();
	}

	// 타겟마다 캡처값과 타일을 뒤에 붙임. 이미 수집된 타겟은 처음 캡처값 유지
	for (int32 Index = 0; Index < Ctx.mTargets.Num(); ++Index)
	{
		const TWeakObjectPtr<UBoardActorModel>& Target = Ctx.mTargets[Index];
		if (bAccumulate && State->mCapturedTargets.Contains(Target))
		{
			continue;
		}

		// 키마다 타겟 값 추가. 오류로 계산에 실패하면 키 자체를 지워서 조건 실패하게 함
		for (const FPassiveCaptureEntry& Entry : mStaticData->mCaptureOperands)
		{
			FPassiveCaptureSlot* Slot = State->mCaptures.Find(Entry.mKey);
			if (Slot == nullptr)
			{
				continue;
			}

			FPassiveOperand Operand = Entry.mOperand;
			Operand.mSource = EPassiveOperandSource::Target;

			float Value = 0.f;
			if (Operand.Resolve(Ctx, Index, *State, Value))
			{
				Slot->mTargets.Add(Value);
			}
			else
			{
				State->mCaptures.Remove(Entry.mKey);
				UE_LOG(LogPassive, Warning, TEXT("패시브 캡처 실패: %s (키 %s)"), *GetNameSafe(mStaticData), *Entry.mKey.ToString());
			}
		}

		// 타일 위치는 항상 저장 (이동 거리 판정용). 스냅샷 없으면 Invalid
		const UBoardCombatTargetSnapshotData* Snapshot = Ctx.mTargetSnapshots.IsValidIndex(Index) ? Ctx.mTargetSnapshots[Index] : nullptr;
		State->mCapturedTargetTiles.Add(Snapshot != nullptr ? Snapshot->mTileTransform.mIndex : FTileIndex::Invalid);
		State->mCapturedTargets.Add(Target);
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
