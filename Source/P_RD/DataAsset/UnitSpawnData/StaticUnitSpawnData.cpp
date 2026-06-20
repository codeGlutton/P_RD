#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "Setting/GamePlaySettings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
void UStaticUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UStaticUnitSpawnData, mModelClass))
	{
		// [핫픽스] 매핑이 있으면 그걸로 mUnitClass를 채우되, 매핑이 비어 있으면 기존 값을 보존한다.
		// (예전엔 무조건 nullptr로 초기화해서, 매핑 미설정 상태에서 mUnitClass가 항상 null이 됐다.)
		if (mModelClass.IsNull() == false)
		{
			const FModelViewMapping* Mapping = GetDefault<UGamePlaySettings>()->mModelViewMappings.Find(mModelClass);
			if (Mapping != nullptr)
			{
				mUnitClass = Mapping->mViewClass.ToSoftObjectPath();
			}
		}
	}
}

EDataValidationResult UStaticUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mModelClass == nullptr || mUnitClass == nullptr || mDisplayName.IsEmpty() == true)
	{
		Context.AddError(FText::FromString(TEXT("모델 클래스 or 액터 클래스 or 이름 미지정")));
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

