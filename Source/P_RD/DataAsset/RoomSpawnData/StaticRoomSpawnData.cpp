#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UStaticRoomSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult SuperResult = Super::IsDataValid(Context);
	EDataValidationResult ThisResult = EDataValidationResult::Valid;

	if (mBackgroundMap == nullptr)
	{
		Context.AddError(FText::FromString(TEXT("배경 맵 미지정")));
		ThisResult = EDataValidationResult::Invalid;
	}
	if (mGameModeBase == nullptr)
	{
		Context.AddError(FText::FromString(TEXT("게임 모드 미지정")));
		ThisResult = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(SuperResult, ThisResult);
}
#endif
