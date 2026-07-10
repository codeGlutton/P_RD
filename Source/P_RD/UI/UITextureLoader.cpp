#include "UI/UITextureLoader.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

FString RDUITexture::ResolveContentFilePath(const FString& RelativeContentPath)
{
	if (FPaths::IsRelative(RelativeContentPath) == false)
	{
		return FPaths::ConvertRelativePathToFull(RelativeContentPath);
	}

	const FString ContentFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), RelativeContentPath);

#if PLATFORM_ANDROID
	// Android 패키지에서 NonUFS 미디어도 APK 내부 OBB에 묶이면 FileMediaSource가 OS 파일로 직접 열 수 없다.
	// UE 파일 시스템으로 읽은 뒤 앱 Saved 아래 실제 파일로 캐시하고, MediaPlayer에는 그 경로를 넘긴다.
	const FString CachedFilePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("MediaCache"),
		RelativeContentPath));

	IFileManager& FileManager = IFileManager::Get();
	const int64 SourceSize = FileManager.FileSize(*ContentFilePath);
	const int64 CachedSize = FileManager.FileSize(*CachedFilePath);
	if (SourceSize > 0 && SourceSize == CachedSize)
	{
		return FileManager.ConvertToAbsolutePathForExternalAppForRead(*CachedFilePath);
	}

	TArray<uint8> FileBytes;
	if (FFileHelper::LoadFileToArray(FileBytes, *ContentFilePath))
	{
		const FString CachedDirectory = FPaths::GetPath(CachedFilePath);
		FileManager.MakeDirectory(*CachedDirectory, true);
		if (FFileHelper::SaveArrayToFile(FileBytes, *CachedFilePath))
		{
			const FString ExternalFilePath = FileManager.ConvertToAbsolutePathForExternalAppForRead(*CachedFilePath);
			UE_LOG(LogRD, Display, TEXT("Cached media file for Android playback: %s -> %s"), *ContentFilePath, *ExternalFilePath);
			return ExternalFilePath;
		}
	}

	UE_LOG(LogRD, Warning, TEXT("Failed to cache Android media file for playback: %s"), *ContentFilePath);
#endif

	return ContentFilePath;
}
