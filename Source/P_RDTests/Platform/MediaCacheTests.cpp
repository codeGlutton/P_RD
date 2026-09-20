#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/UITextureLoader.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	struct FMediaFixture
	{
		FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/MediaCache"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
		FMediaFixture() { IFileManager::Get().MakeDirectory(*Directory, true); }
		~FMediaFixture() { IFileManager::Get().DeleteDirectory(*Directory, false, true); }
		FString Path(const TCHAR* Name) const { return FPaths::Combine(Directory, Name); }
	};

	void AppendBE32(TArray<uint8>& Bytes, uint32 Value)
	{
		Bytes.Append({uint8(Value >> 24), uint8(Value >> 16), uint8(Value >> 8), uint8(Value)});
	}

	TArray<uint8> Box(uint32 Type, const TArray<uint8>& Payload)
	{
		TArray<uint8> Bytes;
		AppendBE32(Bytes, Payload.Num() + 8);
		AppendBE32(Bytes, Type);
		Bytes.Append(Payload);
		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMediaCacheContentTest, "P_RD.Platform.Android.MediaCache.ContentIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMediaCacheContentTest::RunTest(const FString& Parameters)
{
	FMediaFixture Fixture;
	const FString Source = Fixture.Path(TEXT("source.mp4"));
	const FString Cache = Fixture.Path(TEXT("cache/video.mp4"));
	const TArray<uint8> First = {1, 2, 3, 4};
	const TArray<uint8> Update = {5, 6, 7, 8};
	TestTrue(TEXT("Create source"), FFileHelper::SaveArrayToFile(First, *Source));
	TestTrue(TEXT("First install creates cache"), RDUITexture::EnsureCachedMediaFile(Source, Cache));
	TestTrue(TEXT("Unchanged content reuses cache"), RDUITexture::EnsureCachedMediaFile(Source, Cache));
	TestTrue(TEXT("Replace with equal-length source"), FFileHelper::SaveArrayToFile(Update, *Source));
	TestTrue(TEXT("Equal-length update refreshes cache"), RDUITexture::EnsureCachedMediaFile(Source, Cache));
	TArray<uint8> Actual;
	TestTrue(TEXT("Read updated cache"), FFileHelper::LoadFileToArray(Actual, *Cache));
	TestTrue(TEXT("Updated bytes are used"), Actual == Update);
	TestTrue(TEXT("Corrupt cache without changing length"), FFileHelper::SaveArrayToFile(First, *Cache));
	TestTrue(TEXT("Equal-length corruption is repaired"), RDUITexture::EnsureCachedMediaFile(Source, Cache));
	FFileHelper::LoadFileToArray(Actual, *Cache);
	TestTrue(TEXT("Repair restores source bytes"), Actual == Update);
	IFileManager::Get().Delete(*Source);
	TestFalse(TEXT("Missing source does not validate stale cache"), RDUITexture::EnsureCachedMediaFile(Source, Cache));
	FFileHelper::LoadFileToArray(Actual, *Cache);
	TestTrue(TEXT("Failed refresh retains last complete cache"), Actual == Update);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMediaDimensionsTest, "P_RD.Platform.Android.MediaCache.Mp4Dimensions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMediaDimensionsTest::RunTest(const FString& Parameters)
{
	FMediaFixture Fixture;
	TArray<uint8> TrackPayload;
	TrackPayload.SetNumZeroed(76);
	AppendBE32(TrackPayload, 1920u << 16);
	AppendBE32(TrackPayload, 1080u << 16);
	const TArray<uint8> Movie = Box(0x6d6f6f76, Box(0x7472616b, Box(0x746b6864, TrackPayload)));
	// A tkhd-like byte sequence inside encoded payload must not be treated as metadata.
	TArray<uint8> FalseTrack;
	FalseTrack.SetNumZeroed(76);
	AppendBE32(FalseTrack, 32u << 16);
	AppendBE32(FalseTrack, 32u << 16);
	TArray<uint8> Video = Box(0x6d646174, Box(0x746b6864, FalseTrack));
	Video.Append(Movie);
	const FString Path = Fixture.Path(TEXT("movie.mp4"));
	TestTrue(TEXT("Write media fixture"), FFileHelper::SaveArrayToFile(Video, *Path));
	TestEqual(TEXT("Seek past payload and read nested track header"), RDUITexture::ReadMediaFileDimensions(Path), FVector2D(1920, 1080));
	Video[0] = 0x7f;
	FFileHelper::SaveArrayToFile(Video, *Path);
	TestEqual(TEXT("Out-of-file box is rejected"), RDUITexture::ReadMediaFileDimensions(Path), FVector2D::ZeroVector);
	const TArray<uint8> Truncated = {0, 0, 0, 1, 'm', 'o', 'o', 'v'};
	FFileHelper::SaveArrayToFile(Truncated, *Path);
	TestEqual(TEXT("Truncated 64-bit header is rejected"), RDUITexture::ReadMediaFileDimensions(Path), FVector2D::ZeroVector);
	return !HasAnyErrors();
}
#endif
