#include "Setting/RDWorldSettings.h"

FName ARDWorldSettings::GetRandomRoomSpawnSettingName(const FRandomStream& Stream) const
{
	// 지정 전용 세팅은 추첨에서 제외 (방 DA가 이름으로 지정할 때만 사용)
	TArray<FName> Keys;
	for (const TPair<FName, FRoomSpawnSettings>& SettingPair : mSpawnSettings)
	{
		if (SettingPair.Value.mIsDedicated == false)
		{
			Keys.Add(SettingPair.Key);
		}
	}

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

