#include "DataAsset/RoomSpawnData/StaticFrontendRoomSpawnData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UStaticFrontendRoomSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mPlayableUnits.IsEmpty() == true)
	{
		Context.AddError(FText::FromString(TEXT("선택가능한 플레이어 없음")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif
