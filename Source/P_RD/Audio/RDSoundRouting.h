#pragma once

#include "CoreMinimal.h"
#include "Singleton/InstanceSubsystem/PersistentDataType.h"

namespace RDSoundRouting
{
	// Imported SVN waves frequently have no authored SoundClass. Keep the routing
	// rules here so newly loaded and already loaded sounds follow the same buses.
	inline EGameVolumeType Category(const FString& Path)
	{
		if (Path.Contains(TEXT("/UISFX/")) || Path.Contains(TEXT("/OpenGameArt_CC0/UI/"))
			|| Path.Contains(TEXT("/Jingle/")) || Path.Contains(TEXT("/OpenGameArt_CC0/Coin/"))
			|| Path.Contains(TEXT("/ExternalCoinCandidates/")) || Path.Contains(TEXT("/ExternalSkillSelectCandidatesV3/")))
			return EGameVolumeType::UI;
		if (Path.Contains(TEXT("/Music/")) || Path.Contains(TEXT("/TitleMusic/")))
			return EGameVolumeType::BGM;
		return EGameVolumeType::SFX;
	}
}
