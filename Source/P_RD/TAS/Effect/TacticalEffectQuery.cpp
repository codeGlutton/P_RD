#include "TAS/Effect/TacticalEffectQuery.h"
#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/ActiveTacticalEffect.h"

FTacticalEffectQuery::FTacticalEffectQuery(FActiveTacticalEffectQueryCustomMatch InCustomMatchDelegate) : mCustomMatchDelegate(InCustomMatchDelegate)
{
}

bool FTacticalEffectQuery::operator==(const FTacticalEffectQuery& Other) const
{
	// TDelegate에는 값 비교 연산자가 없다. 쿼리의 직렬화 가능한 조건은 값으로
	// 비교하고, 런타임 콜백은 동일성을 정의할 수 없으므로 바인딩 여부만 본다.
	return mCustomMatchDelegate.IsBound() == Other.mCustomMatchDelegate.IsBound()
		&& mOwningTagQuery == Other.mOwningTagQuery
		&& mEffectTagQuery == Other.mEffectTagQuery
		&& mModifyingAttribute == Other.mModifyingAttribute
		&& mEffectSource == Other.mEffectSource
		&& mEffectDefinition == Other.mEffectDefinition
		&& mIgnoreHandles == Other.mIgnoreHandles;
}

bool FTacticalEffectQuery::operator!=(const FTacticalEffectQuery& Other) const
{
	return !(*this == Other);
}

bool FTacticalEffectQuery::Matches(const FActiveTacticalEffect& Effect) const
{
	/* 무시할 핸들 포함 검사 */

	if (mIgnoreHandles.Contains(Effect.mHandle) == true)
	{
		return false;
	}

	/* 커스텀 체크 함수 검사 */

	if (mCustomMatchDelegate.IsBound() == true)
	{
		if (mCustomMatchDelegate.Execute(Effect) == false)
		{
			return false;
		}
	}

	return Matches(Effect.mSpec);
}

bool FTacticalEffectQuery::Matches(const FTacticalEffectSpec& Spec) const
{
	if (Spec.mEffectClass == nullptr)
	{
		return false;
	}

	/* ASC 태그 검사 */

	if (mOwningTagQuery.IsEmpty() == false)
	{
		check(IsInGameThread() == true);

		static FGameplayTagContainer TargetTags;
		TargetTags.Reset();

		TargetTags.AppendTags(Spec.mEffectClass->GetAssetTags());
		TargetTags.AppendTags(Spec.mEffectClass->GetGrantedTags());

		if (mOwningTagQuery.Matches(TargetTags) == false)
		{
			return false;
		}
	}

	/* 활성 Effect 태그 검사 */

	if (mEffectTagQuery.IsEmpty() == false)
	{
		check(IsInGameThread());

		static FGameplayTagContainer TETags;
		TETags.Reset();

		TETags.AppendTags(Spec.mEffectClass->GetAssetTags());

		if (mEffectTagQuery.Matches(TETags) == false)
		{
			return false;
		}
	}

	/* 수정 속성 검사 */

	if (mModifyingAttribute.IsValid() == true)
	{
		bool IsEffectModifiesThisAttribute = false;

		for (int32 ModIdx = 0; ModIdx < Spec.mModifiers.Num(); ++ModIdx)
		{
			const FTacticalModifierInfo& ModDef = Spec.mEffectClass->mModifiers[ModIdx];
			if (ModDef.mAttribute == mModifyingAttribute)
			{
				IsEffectModifiesThisAttribute = true;
				break;
			}
		}
		if (IsEffectModifiesThisAttribute == false)
		{
			return false;
		}
	}

	/* 소스 오브젝트 검사 */

	if (mEffectSource != nullptr)
	{
		if (Spec.GetContext()->GetSourceObject() != mEffectSource)
		{
			return false;
		}
	}

	/* 활성 이펙트 클래스 검사 */

	if (mEffectDefinition != nullptr)
	{
		if (Spec.mEffectClass != mEffectDefinition.GetDefaultObject())
		{
			return false;
		}
	}

	return true;
}

bool FTacticalEffectQuery::IsEmpty() const
{
	return (
		mCustomMatchDelegate.IsBound() == false &&
		mOwningTagQuery.IsEmpty() == true &&
		mEffectTagQuery.IsEmpty() == true &&
		mModifyingAttribute.IsValid() == false &&
		mEffectSource == nullptr &&
		mEffectDefinition == nullptr
		);
}

FTacticalEffectQuery FTacticalEffectQuery::MakeQuery_MatchAnyOwningTags(const FGameplayTagContainer& InTags)
{
	FTacticalEffectQuery OutQuery;
	OutQuery.mOwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(InTags);
	return OutQuery;
}

FTacticalEffectQuery FTacticalEffectQuery::MakeQuery_MatchAllOwningTags(const FGameplayTagContainer& InTags)
{
	FTacticalEffectQuery OutQuery;
	OutQuery.mOwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAllTags(InTags);
	return OutQuery;
}

FTacticalEffectQuery FTacticalEffectQuery::MakeQuery_MatchNoOwningTags(const FGameplayTagContainer& InTags)
{
	FTacticalEffectQuery OutQuery;
	OutQuery.mOwningTagQuery = FGameplayTagQuery::MakeQuery_MatchNoTags(InTags);
	return OutQuery;
}

FTacticalEffectQuery FTacticalEffectQuery::MakeQuery_MatchAnyEffectTags(const FGameplayTagContainer& InTags)
{
	FTacticalEffectQuery OutQuery;
	OutQuery.mEffectTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(InTags);
	return OutQuery;
}

FTacticalEffectQuery FTacticalEffectQuery::MakeQuery_MatchAllEffectTags(const FGameplayTagContainer& InTags)
{
	FTacticalEffectQuery OutQuery;
	OutQuery.mEffectTagQuery = FGameplayTagQuery::MakeQuery_MatchAllTags(InTags);
	return OutQuery;
}

FTacticalEffectQuery FTacticalEffectQuery::MakeQuery_MatchNoEffectTags(const FGameplayTagContainer& InTags)
{
	FTacticalEffectQuery OutQuery;
	OutQuery.mEffectTagQuery = FGameplayTagQuery::MakeQuery_MatchNoTags(InTags);
	return OutQuery;
}
