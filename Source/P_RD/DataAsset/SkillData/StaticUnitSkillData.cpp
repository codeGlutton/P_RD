#include "DataAsset/SkillData/StaticUnitSkillData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "StaticSkillData"

FText UStaticUnitSkillData::MakeDescription() const
{
	FText Description = Super::MakeDescription();
	if (mIgnoreCondition == true)
	{
		FText PreviewHeader = LOCTEXT("PreviewHeaderFormat", "※ 컨디션 무관 랜덤 적용");
		Description = FText::Format(
			LOCTEXT("DescriptionFormat", "{0}\n{1}"),
			PreviewHeader,
			Description
		);
	}
	return Description;
}

#undef LOCTEXT_NAMESPACE

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