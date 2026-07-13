#include "Setting/RDWorldSettings.h"

FName ARDWorldSettings::GetRandomRoomSpawnSettingName(const FRandomStream& Stream) const
{
	TArray<FName> Keys;
	mSpawnSettings.GenerateKeyArray(Keys);

	if (Keys.Num() > 0)
	{
		const FName& RandomKey = Keys[Stream.RandRange(0, Keys.Num() - 1)];
		return RandomKey;
	}
	return NAME_None;
}

AActor* ARDWorldSettings::GetMainCameraPoint(const FName& Name) const
{
	const FRoomSpawnSettings* FoundSettings = mSpawnSettings.Find(Name);
	if (FoundSettings != nullptr)
	{
		return FoundSettings->mMainCameraPoint;
	}
	return nullptr;
}

AActor* ARDWorldSettings::GetRoomStartPoint(const FName& Name) const
{
	const FRoomSpawnSettings* FoundSettings = mSpawnSettings.Find(Name);
	if (FoundSettings != nullptr)
	{
		return FoundSettings->mRoomStartPoint;
	}
	return nullptr;
}

