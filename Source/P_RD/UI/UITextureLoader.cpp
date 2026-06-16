#include "UI/UITextureLoader.h"

#include "Engine/Texture2D.h"

FString RDUITexture::ResolveContentFilePath(const FString& RelativeContentPath)
{
	if (FPaths::IsRelative(RelativeContentPath) == false)
	{
		return FPaths::ConvertRelativePathToFull(RelativeContentPath);
	}

	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), RelativeContentPath);
}

UTexture2D* RDUITexture::LoadUITexture(const FString& AssetPath, const TCHAR* LogOwner)
{
	if (AssetPath.IsEmpty())
	{
		return nullptr;
	}

	// 무거운 텍스처는 SVN에 임포트해 둔 uasset을 그대로 로드한다(쿡 시 ASTC 압축·밉맵 적용).
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath);
	if (Texture == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: UI texture asset not found: %s"), LogOwner, *AssetPath);
	}

	return Texture;
}
