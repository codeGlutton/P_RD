#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

EDataValidationResult UStaticUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mClass == nullptr || mDisplayName.IsEmpty() == true)
	{
		Context.AddError(FText::FromString(TEXT("액터 클래스 혹은 이름 미지정")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif

FName UStaticUnitSpawnData::GetKeyName() const
{
	FString Key = mDisplayName.ToString();
	Key.RemoveSpacesInline();
	return *Key;
}

