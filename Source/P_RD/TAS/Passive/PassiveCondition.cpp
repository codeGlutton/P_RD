/*****************************************************************//**
 * @file   PassiveCondition.cpp
 * @brief  패시브 조건 평가기 구현
 * @author 이문환
 * @date   2026-08-30
 *********************************************************************/

#include "TAS/Passive/PassiveCondition.h"
#include "TAS/Passive/PassiveActivateContext.h"
#include "TAS/Passive/DynamicPassiveData_Generic.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

namespace PassiveConditionUtils
{
	bool EvaluateAll(const TArray<FPassiveCondition>& Conditions, const FPassiveActivateContext& Ctx, int32 TargetIndex, const FDynamicPassiveData_Generic& State)
	{
		// 배열의 모든 원소에 대해서 AND 판정 (배열이 비어있으면 루프를 안 도니까 true)
		for (const FPassiveCondition& Condition : Conditions)
		{
			if (Condition.Evaluate(Ctx, TargetIndex, State) == false)
			{
				return false;
			}
		}
		return true;
	}

	int32 TileDistance(const FTileIndex& A, const FTileIndex& B)
	{
		// 맨해튼 방식: 가로 차이 + 세로 차이
		return FMath::Abs(A.mX - B.mX) + FMath::Abs(A.mY - B.mY);
	}

	const UBoardCombatTargetSnapshotData* GetSnapshot(const FPassiveActivateContext& Ctx, EPassiveOperandSource Source, int32 TargetIndex)
	{
		// Self는 소유자 스냅샷, Target은 인덱스가 유효할 때만 타겟 스냅샷
		switch (Source)
		{
		case EPassiveOperandSource::Self:
			return Ctx.mOwnerSnapshot;
		case EPassiveOperandSource::Target:
			return Ctx.mTargetSnapshots.IsValidIndex(TargetIndex) ? Ctx.mTargetSnapshots[TargetIndex] : nullptr;
		default:
			return nullptr;
		}
	}
}

bool FPassiveCondition::Compare(float Lhs, EPassiveCompareOp Op, float Rhs)
{
	switch (Op)
	{
	case EPassiveCompareOp::Less:         return Lhs < Rhs;
	case EPassiveCompareOp::LessEqual:    return Lhs <= Rhs;
	case EPassiveCompareOp::Equal:        return FMath::IsNearlyEqual(Lhs, Rhs);
	case EPassiveCompareOp::NotEqual:     return FMath::IsNearlyEqual(Lhs, Rhs) == false;
	case EPassiveCompareOp::GreaterEqual: return Lhs >= Rhs;
	case EPassiveCompareOp::Greater:      return Lhs > Rhs;
	case EPassiveCompareOp::ModuloZero:
		{
			// 피연산자는 float지만 %는 정수 전용이므로 정수로 바꿔서 계산
			const int32 Divisor = FMath::RoundToInt(Rhs);
			if (Divisor <= 0)
			{
				return false;
			}
			return FMath::RoundToInt(Lhs) % Divisor == 0;
		}
	default:
		return false;
	}
}

bool FPassiveCondition::Evaluate(const FPassiveActivateContext& Ctx, int32 TargetIndex, const FDynamicPassiveData_Generic& State) const
{
	// 좌변 계산
	float LhsValue = 0.f;
	if (mLhs.Resolve(Ctx, TargetIndex, State, LhsValue) == false)
	{
		return false;
	}

	// 우변 계산
	float RhsValue = 0.f;
	if (mRhs.Resolve(Ctx, TargetIndex, State, RhsValue) == false)
	{
		return false;
	}

	return Compare(LhsValue, mOp, RhsValue);
}

bool FPassiveOperand::Resolve(const FPassiveActivateContext& Ctx, int32 TargetIndex, const FDynamicPassiveData_Generic& State, float& OutValue) const
{
	float Value = 0.f;

	// Kind별 계산값
	switch (mKind)
	{
	case EPassiveOperandKind::Const:
		// 고정값은 배수 없이 그대로 반환
		OutValue = mConst;
		return true;

	case EPassiveOperandKind::Counter:
		Value = static_cast<float>(State.mCounter);
		break;

	case EPassiveOperandKind::TargetCount:
		Value = static_cast<float>(Ctx.mTargets.Num());
		break;

	case EPassiveOperandKind::Attribute:
		{
			// 출처 스냅샷에서 속성 현재값. 스냅샷이나 속성이 없으면 실패
			const UBoardCombatTargetSnapshotData* Snapshot = PassiveConditionUtils::GetSnapshot(Ctx, mSource, TargetIndex);
			if (Snapshot == nullptr)
			{
				return false;
			}
			const float* Found = Snapshot->mAttributes.Find(mAttribute);
			if (Found == nullptr)
			{
				return false;
			}
			Value = *Found;
		}
		break;

	case EPassiveOperandKind::TagCount:
		{
			// 출처 스냅샷의 상태이상 스택 중 mTag와 일치(하위 태그 포함)하는 것의 합
			const UBoardCombatTargetSnapshotData* Snapshot = PassiveConditionUtils::GetSnapshot(Ctx, mSource, TargetIndex);
			if (Snapshot == nullptr)
			{
				return false;
			}
			int32 Count = 0;
			for (const TPair<FGameplayTag, int32>& Pair : Snapshot->mEffectCounts)
			{
				if (Pair.Key.MatchesTag(mTag))
				{
					Count += Pair.Value;
				}
			}
			Value = static_cast<float>(Count);
		}
		break;

	case EPassiveOperandKind::Distance:
		{
			// 소유자 타일과 타겟 타일 사이 거리. 둘 중 하나라도 스냅샷이 없으면 실패
			const UBoardCombatTargetSnapshotData* SelfSnapshot = PassiveConditionUtils::GetSnapshot(Ctx, EPassiveOperandSource::Self, TargetIndex);
			const UBoardCombatTargetSnapshotData* TargetSnapshot = PassiveConditionUtils::GetSnapshot(Ctx, EPassiveOperandSource::Target, TargetIndex);
			if (SelfSnapshot == nullptr || TargetSnapshot == nullptr)
			{
				return false;
			}
			Value = static_cast<float>(PassiveConditionUtils::TileDistance(SelfSnapshot->mTileTransform.mIndex, TargetSnapshot->mTileTransform.mIndex));
		}
		break;

	case EPassiveOperandKind::Captured:
		{
			// 캡처 키로 저장값 조회. 키가 없거나 타겟 인덱스가 범위 밖이면 실패
			const FPassiveCaptureSlot* Slot = State.mCaptures.Find(mCaptureKey);
			if (Slot == nullptr)
			{
				return false;
			}
			if (mSource == EPassiveOperandSource::Self)
			{
				Value = Slot->mSelf;
			}
			else
			{
				if (Slot->mTargets.IsValidIndex(TargetIndex) == false)
				{
					return false;
				}
				Value = Slot->mTargets[TargetIndex];
			}
		}
		break;

	case EPassiveOperandKind::MovedDistance:
		{
			// 캡처 시점 타일과 현재 타일 사이 거리. 캡처가 없으면(Invalid) 실패
			const UBoardCombatTargetSnapshotData* Snapshot = PassiveConditionUtils::GetSnapshot(Ctx, mSource, TargetIndex);
			if (Snapshot == nullptr)
			{
				return false;
			}
			FTileIndex CapturedTile = FTileIndex::Invalid;
			if (mSource == EPassiveOperandSource::Self)
			{
				CapturedTile = State.mCapturedSelfTile;
			}
			else if (State.mCapturedTargetTiles.IsValidIndex(TargetIndex))
			{
				CapturedTile = State.mCapturedTargetTiles[TargetIndex];
			}
			if (CapturedTile == FTileIndex::Invalid)
			{
				return false;
			}
			Value = static_cast<float>(PassiveConditionUtils::TileDistance(CapturedTile, Snapshot->mTileTransform.mIndex));
		}
		break;

	case EPassiveOperandKind::Team:
		{
			// 소유자 기준 타겟의 팀 관계. 모델 포인터가 없거나 전투 대상이 아니면 실패
			const IBoardCombatTarget* Owner = Cast<IBoardCombatTarget>(Ctx.mOwner.Get());
			const UBoardActorModel* Target = Ctx.mTargets.IsValidIndex(TargetIndex) ? Ctx.mTargets[TargetIndex].Get() : nullptr;
			if (Owner == nullptr || Target == nullptr)
			{
				return false;
			}
			switch (Owner->GetTeamAttitudeTowards(*Target))
			{
			case ETeamAttitude::Friendly: OutValue = 1.f; break;
			case ETeamAttitude::Hostile:  OutValue = 2.f; break;
			default:                      OutValue = 0.f; break;
			}
			// 범주값이라 배수 없이 반환
			return true;
		}

	case EPassiveOperandKind::Custom:
		{
			// 커스텀 클래스의 Resolve 호출 (인스턴스 생성 없이 CDO 사용)
			const UPassiveCustomOperand* Custom = mCustomClass != nullptr ? mCustomClass.GetDefaultObject() : nullptr;
			if (Custom == nullptr || Custom->Resolve(Ctx, TargetIndex, State, Value) == false)
			{
				return false;
			}
		}
		break;

	default:
		return false;
	}

	// 배수 적용
	OutValue = Value * mMultiplier;
	return true;
}
