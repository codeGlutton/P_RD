#include "UI/UITextureLoader.h"

#include "HAL/FileManager.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	bool HashMediaFile(const FString& Path, FSHAHash& OutHash)
	{
		TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*Path, FILEREAD_Silent));
		if (!Reader || Reader->TotalSize() <= 0) return false;
		FSHA1 Hash;
		uint8 Buffer[64 * 1024];
		while (!Reader->AtEnd() && !Reader->IsError())
		{
			const int64 Count = FMath::Min<int64>(sizeof(Buffer), Reader->TotalSize() - Reader->Tell());
			Reader->Serialize(Buffer, Count);
			if (Reader->IsError()) return false;
			Hash.Update(Buffer, Count);
		}
		if (!Reader->Close()) return false;
		OutHash = Hash.Finalize();
		return true;
	}

	uint32 ReadBE32(const uint8* Data)
	{
		return (uint32(Data[0]) << 24) | (uint32(Data[1]) << 16)
			| (uint32(Data[2]) << 8) | uint32(Data[3]);
	}

	FVector2D ReadTrackDimensions(FArchive& Reader, int64 Begin, int64 End, int32 Depth, int32& RemainingBoxes)
	{
		if (Depth > 8) return FVector2D::ZeroVector;
		for (int64 Offset = Begin; End - Offset >= 8 && RemainingBoxes-- > 0;)
		{
			uint8 Header[16] = {};
			Reader.Seek(Offset);
			Reader.Serialize(Header, 8);
			if (Reader.IsError()) return FVector2D::ZeroVector;
			uint64 Size = ReadBE32(Header);
			int64 HeaderSize = 8;
			if (Size == 1)
			{
				if (End - Offset < 16) return FVector2D::ZeroVector;
				Reader.Serialize(Header + 8, 8);
				if (Reader.IsError()) return FVector2D::ZeroVector;
				Size = (uint64(ReadBE32(Header + 8)) << 32) | ReadBE32(Header + 12);
				HeaderSize = 16;
			}
			else if (Size == 0) Size = End - Offset;
			if (Size < uint64(HeaderSize) || Size > uint64(End - Offset)) return FVector2D::ZeroVector;
			const int64 BoxEnd = Offset + int64(Size);
			const uint32 Type = ReadBE32(Header + 4);
			if (Type == 0x746b6864 /* tkhd */ && Size >= uint64(HeaderSize + 84))
			{
				uint8 Dimensions[8];
				Reader.Seek(BoxEnd - 8);
				Reader.Serialize(Dimensions, sizeof(Dimensions));
				if (Reader.IsError()) return FVector2D::ZeroVector;
				const FVector2D Result(double(ReadBE32(Dimensions)) / 65536.0,
					double(ReadBE32(Dimensions + 4)) / 65536.0);
				if (Result.X > 0 && Result.Y > 0) return Result;
			}
			else if (Type == 0x6d6f6f76 /* moov */ || Type == 0x7472616b /* trak */)
			{
				const FVector2D Result = ReadTrackDimensions(Reader, Offset + HeaderSize, BoxEnd, Depth + 1, RemainingBoxes);
				if (Result.X > 0 && Result.Y > 0) return Result;
			}
			Offset = BoxEnd;
		}
		return FVector2D::ZeroVector;
	}
}

bool RDUITexture::EnsureCachedMediaFile(const FString& SourcePath, const FString& CachePath)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RDMediaCacheResolve);
	FSHAHash SourceHash;
	if (!HashMediaFile(SourcePath, SourceHash)) return false;
	IFileManager& Files = IFileManager::Get();
	FSHAHash CacheHash;
	if (Files.FileSize(*SourcePath) == Files.FileSize(*CachePath)
		&& HashMediaFile(CachePath, CacheHash) && SourceHash == CacheHash) return true;

	if (!Files.MakeDirectory(*FPaths::GetPath(CachePath), true)) return false;
	const FString TemporaryPath = CachePath + TEXT(".tmp-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	ON_SCOPE_EXIT { Files.Delete(*TemporaryPath, false, true, true); };
	if (Files.Copy(*TemporaryPath, *SourcePath, true, false) != COPY_OK) return false;
	FSHAHash CopiedHash;
	if (!HashMediaFile(TemporaryPath, CopiedHash) || CopiedHash != SourceHash) return false;
	return Files.Move(*CachePath, *TemporaryPath, true, false, false, true);
}

FVector2D RDUITexture::ReadMediaFileDimensions(const FString& FilePath)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RDMediaReadDimensions);
	TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*FilePath, FILEREAD_Silent));
	if (!Reader) return FVector2D::ZeroVector;
	int32 RemainingBoxes = 10000;
	return ReadTrackDimensions(*Reader, 0, Reader->TotalSize(), 0, RemainingBoxes);
}

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
	if (EnsureCachedMediaFile(ContentFilePath, CachedFilePath))
	{
		return FileManager.ConvertToAbsolutePathForExternalAppForRead(*CachedFilePath);
	}

	UE_LOG(LogRD, Warning, TEXT("Failed to cache Android media file for playback: %s"), *ContentFilePath);
#endif

	return ContentFilePath;
}
