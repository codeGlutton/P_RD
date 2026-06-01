#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UStaticUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (mClass == nullptr || mName.IsEmpty() == true)
	{
		Context.AddError(FText::FromString(TEXT("액터 클래스 혹은 이름 미지정")));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif