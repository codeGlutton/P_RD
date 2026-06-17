#include "UI/UITextureLoader.h"

FString RDUITexture::ResolveContentFilePath(const FString& RelativeContentPath)
{
	if (FPaths::IsRelative(RelativeContentPath) == false)
	{
		return FPaths::ConvertRelativePathToFull(RelativeContentPath);
	}

	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), RelativeContentPath);
}
