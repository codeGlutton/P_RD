#include "UI/RewardTextureQuality.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr TCHAR AtlasPath[] = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RewardConcept03New/T_RCN_ChestTripleBurst_Atlas.T_RCN_ChestTripleBurst_Atlas");
	FDelegateHandle LoadedHandle;

	void RestoreChestResolution(UObject* Asset)
	{
		UTexture2D* Texture = Cast<UTexture2D>(Asset);
		if (!Texture || Texture->GetPathName() != AtlasPath) return;
		const FName Android(TEXT("Android"));
		if (const float* Scale = Texture->Downscale.PerPlatform.Find(Android))
			if (*Scale == 1.f) return;
		// 6x6 atlas: a 768px texture leaves only 128px per animation frame.
		// Apply on every editor/cooker load; the shared SVN asset stays untouched.
		const bool WasDirty = Texture->GetOutermost()->IsDirty();
		Texture->Downscale.PerPlatform.Add(Android, 1.f);
		FPropertyChangedEvent Event(FindFProperty<FProperty>(UTexture::StaticClass(),
			GET_MEMBER_NAME_CHECKED(UTexture, Downscale)));
		Texture->PostEditChangeProperty(Event);
		if (!WasDirty) Texture->GetOutermost()->SetDirtyFlag(false);
		UE_LOG(LogTemp, Display, TEXT("Reward chest Android atlas restored to source resolution: %s"), *Texture->GetPathName());
	}
}

void RegisterRewardTextureQuality()
{
	LoadedHandle = FCoreUObjectDelegates::OnAssetLoaded.AddStatic(&RestoreChestResolution);
	RestoreChestResolution(FindObject<UTexture2D>(nullptr, AtlasPath));
}

void UnregisterRewardTextureQuality()
{
	FCoreUObjectDelegates::OnAssetLoaded.Remove(LoadedHandle);
	LoadedHandle.Reset();
}
