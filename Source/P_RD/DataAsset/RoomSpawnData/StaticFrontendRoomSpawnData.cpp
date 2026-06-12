#include "DataAsset/RoomSpawnData/StaticFrontendRoomSpawnData.h"
#include "GameMode/FrontendGameMode.h"

#include "Setting/GamePlaySettings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UStaticFrontendRoomSpawnData::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		mGameModeBase = GetDefault<UGamePlaySettings>()->mFrontendGameMode;
	}
}

void UStaticFrontendRoomSpawnData::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	const TSoftClassPtr<AGameModeBase> UpdatedGameMode = GetDefault<UGamePlaySettings>()->mFrontendGameMode;
	if (mGameModeBase != UpdatedGameMode)
	{
		Modify();
		mGameModeBase = UpdatedGameMode;
		MarkPackageDirty();
	}
#endif
}

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
