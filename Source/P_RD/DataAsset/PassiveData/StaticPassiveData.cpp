/*****************************************************************//**
 * @file   StaticPassiveData.cpp
 * @brief  패시브 정적 데이터 구현
 * @author 김준형, 이문환
 * @date   2026-06-18
 *********************************************************************/

#include "DataAsset/PassiveData/StaticPassiveData.h"

#include "TAS/Passive/TacticalPassive_Generic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "TAS/Effect/TacticalEffect.h"
#endif

UStaticPassiveData::UStaticPassiveData()
{
	// 새 DA는 별도 지정 없이 제네릭 패시브로 동작
	mPassiveClass = UTacticalPassive_Generic::StaticClass();
}

#if WITH_EDITOR

namespace
{
	/**
	 * @brief 타이밍 태그가 패시브 타이밍 계열인지 검사
	 */
	void ValidateTimingTag(const FGameplayTag& Tag, const FString& Label, FDataValidationContext& Context)
	{
		// 비어있는 태그는 선택 항목이므로 통과
		if (Tag.IsValid() == false)
		{
			return;
		}

		// 패시브 타이밍 계열이 아니면 에러
		static const FGameplayTag PassiveTimingParent = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayAbility.Passive")));
		if (Tag.MatchesTag(PassiveTimingParent) == false)
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("%s: GameplayAbility.Passive 계열 태그가 아님 (%s)"), *Label, *Tag.ToString())));
		}
	}

	/**
	 * @brief 피연산자의 종류별 필수 필드 검사
	 */
	void ValidateOperand(const FPassiveOperand& Operand, const FString& Label, const UStaticPassiveData& Data, FDataValidationContext& Context)
	{
		switch (Operand.mKind)
		{
		// 속성값: 읽을 속성 필수
		case EPassiveOperandKind::Attribute:
			if (Operand.mAttribute.IsValid() == false)
			{
				Context.AddError(FText::FromString(Label + TEXT(": 속성 미지정")));
			}
			break;

		// 태그 개수: 집계할 태그 필수
		case EPassiveOperandKind::TagCount:
			if (Operand.mTag.IsValid() == false)
			{
				Context.AddError(FText::FromString(Label + TEXT(": 태그 미지정")));
			}
			break;

		// 캡처값: 키 필수, 캡처 시점과 캡처 목록에 짝이 있어야 함
		case EPassiveOperandKind::Captured:
			if (Operand.mCaptureKey.IsNone())
			{
				Context.AddError(FText::FromString(Label + TEXT(": 캡처 키 미지정")));
			}
			else
			{
				if (Data.mCaptureTimingTag.IsValid() == false)
				{
					Context.AddError(FText::FromString(Label + TEXT(": 캡처값을 쓰는데 캡처 시점이 비어있음")));
				}
				const bool bKeyDefined = Data.mCaptureOperands.ContainsByPredicate(
					[&Operand](const FPassiveCaptureEntry& Entry) { return Entry.mKey == Operand.mCaptureKey; });
				if (bKeyDefined == false)
				{
					Context.AddError(FText::FromString(FString::Printf(TEXT("%s: 캡처 키 '%s'가 캡처 목록에 없음"), *Label, *Operand.mCaptureKey.ToString())));
				}
			}
			break;

		// 이동 거리: 캡처 시점의 타일을 읽으므로 캡처 시점 필수
		case EPassiveOperandKind::MovedDistance:
			if (Data.mCaptureTimingTag.IsValid() == false)
			{
				Context.AddError(FText::FromString(Label + TEXT(": 이동 거리를 쓰는데 캡처 시점이 비어있음")));
			}
			break;

		// 카운터: 리셋 시점이 없으면 전투를 넘어 계속 누적되므로 경고
		case EPassiveOperandKind::Counter:
			if (Data.mCounterResetTimingTag.IsValid() == false)
			{
				Context.AddWarning(FText::FromString(Label + TEXT(": 카운터를 쓰는데 리셋 시점이 비어있음")));
			}
			break;

		// 커스텀: 계산 클래스 필수
		case EPassiveOperandKind::Custom:
			if (Operand.mCustomClass == nullptr)
			{
				Context.AddError(FText::FromString(Label + TEXT(": 커스텀 클래스 미지정")));
			}
			break;

		default:
			break;
		}
	}
}

EDataValidationResult UStaticPassiveData::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);

	// 발동 시점은 필수
	if (mActivateTimingTag.IsValid() == false)
	{
		Context.AddError(FText::FromString(TEXT("발동 시점이 비어있음")));
	}

	// 타이밍 태그 4종의 계열 검사
	ValidateTimingTag(mActivateTimingTag, TEXT("발동 시점"), Context);
	ValidateTimingTag(mDeactivateTimingTag, TEXT("해제 시점"), Context);
	ValidateTimingTag(mCounterResetTimingTag, TEXT("리셋 시점"), Context);
	ValidateTimingTag(mCaptureTimingTag, TEXT("캡처 시점"), Context);

	// 캡처 목록이 있으면 캡처 시점도 필수
	if (mCaptureOperands.Num() > 0 && mCaptureTimingTag.IsValid() == false)
	{
		Context.AddError(FText::FromString(TEXT("캡처 목록이 있는데 캡처 시점이 비어있음")));
	}

	// 캡처 엔트리 검사
	for (int32 Index = 0; Index < mCaptureOperands.Num(); ++Index)
	{
		const FPassiveCaptureEntry& Entry = mCaptureOperands[Index];
		const FString Prefix = FString::Printf(TEXT("캡처[%d]"), Index);

		// 저장 키 필수
		if (Entry.mKey.IsNone())
		{
			Context.AddError(FText::FromString(Prefix + TEXT(": 키 미지정")));
		}

		// 캡처는 속성값과 태그 개수만 허용
		if (Entry.mOperand.mKind != EPassiveOperandKind::Attribute
			&& Entry.mOperand.mKind != EPassiveOperandKind::TagCount)
		{
			Context.AddError(FText::FromString(Prefix + TEXT(": 캡처는 속성값과 태그 개수만 허용")));
		}

		ValidateOperand(Entry.mOperand, Prefix + TEXT(" 피연산자"), *this, Context);
	}

	// 조건 검사
	for (int32 Index = 0; Index < mConditions.Num(); ++Index)
	{
		const FPassiveCondition& Condition = mConditions[Index];
		const FString Prefix = FString::Printf(TEXT("조건[%d]"), Index);

		ValidateOperand(Condition.mLhs, Prefix + TEXT(" 좌변"), *this, Context);
		ValidateOperand(Condition.mRhs, Prefix + TEXT(" 우변"), *this, Context);

		// 배수 연산의 경우에만 우변이 0 이상인 지 체크
		if (Condition.mOp == EPassiveCompareOp::ModuloZero
			&& Condition.mRhs.mKind == EPassiveOperandKind::Const
			&& Condition.mRhs.mConst <= 0.f)
		{
			Context.AddError(FText::FromString(Prefix + TEXT(": 배수 연산의 우변이 0 이하")));
		}
	}

	// 효과가 없으면 아무 일도 하지 않는 패시브이므로 경고
	if (mEffects.Num() == 0)
	{
		Context.AddWarning(FText::FromString(TEXT("효과 목록이 비어있음")));
	}

	// 효과 검사
	for (int32 Index = 0; Index < mEffects.Num(); ++Index)
	{
		const FPassiveEffectEntry& Entry = mEffects[Index];
		const FString Prefix = FString::Printf(TEXT("효과[%d]"), Index);

		// 이펙트 클래스 필수
		if (Entry.mEffectClass.IsNull())
		{
			Context.AddError(FText::FromString(Prefix + TEXT(": 이펙트 클래스 미지정")));
		}
		else
		{
			// 실시간 계산형 이펙트는 시전자 스냅샷을 요구하는데 패시브에는 아직 없으므로 막아 둠
			// TODO: NotifyPassive에 스냅샷 첨부하면 이 검사는 제거
			const UClass* EffectClass = Entry.mEffectClass.LoadSynchronous();
			const UTacticalEffect* EffectCDO = (EffectClass != nullptr) ? EffectClass->GetDefaultObject<UTacticalEffect>() : nullptr;
			if (EffectCDO != nullptr && EffectCDO->mExecutions.Num() > 0)
			{
				Context.AddError(FText::FromString(Prefix + TEXT(": 실시간 계산형 이펙트는 패시브에서 사용 불가")));
			}
		}

		ValidateOperand(Entry.mMagnitude, Prefix + TEXT(" 수치"), *this, Context);
	}

	// 패시브 클래스 필수
	if (mPassiveClass.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("패시브 클래스 미지정")));
	}

	// 에러가 있으면 검증 실패
	return (Context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : SuperResult;
}

#endif
