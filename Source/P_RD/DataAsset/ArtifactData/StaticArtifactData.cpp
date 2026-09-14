/*****************************************************************//**
 * @file   StaticArtifactData.cpp
 * @brief  아티펙트 정적 Primary Data Asset 구현
 * @author 이문환
 * @date   2026-07-23
 *********************************************************************/

#include "DataAsset/ArtifactData/StaticArtifactData.h"

#include "Engine/AssetManager.h"

FText UStaticArtifactData::GetDisplayDescription() const
{
	if (!mDescription.IsEmpty())
	{
		return mDescription;
	}
	// Legacy assets without an authored description retain their passive text.
	TArray<FText> Lines;
	for (const auto& PassiveRef : mStaticPassiveData)
	{
		if (const UStaticPassiveData* Passive = PassiveRef.LoadSynchronous();
			Passive && !Passive->mDescription.IsEmpty())
		{
			Lines.Add(Passive->mDescription);
		}
	}
	return FText::Join(FText::FromString(TEXT("\n\n")), Lines);
}

UStaticArtifactData* UStaticArtifactData::LoadByAssetId(const FPrimaryAssetId& ArtifactId)
{
	if (ArtifactId.IsValid() == false)
	{
		return nullptr;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager == nullptr)
	{
		return nullptr;
	}

	// 이미 로드된 경우 그대로 반환
	if (UStaticArtifactData* Loaded = AssetManager->GetPrimaryAssetObject<UStaticArtifactData>(ArtifactId))
	{
		return Loaded;
	}

	const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(ArtifactId);
	return Cast<UStaticArtifactData>(AssetPath.TryLoad());
}
