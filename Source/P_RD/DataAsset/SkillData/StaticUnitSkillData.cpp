#include "DataAsset/SkillData/StaticUnitSkillData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR

EDataValidationResult UStaticUnitSkillData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mJobType == EUnitJobType::None)
	{
		Context.AddError(FText::FromString(TEXT("스킬 직업 분류 미지정")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}

#endif