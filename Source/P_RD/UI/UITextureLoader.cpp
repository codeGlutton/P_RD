#include "UI/UITextureLoader.h"

#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

FString RDUITexture::ResolveContentFilePath(const FString& RelativeContentPath)
{
	if (FPaths::IsRelative(RelativeContentPath) == false)
	{
		return FPaths::ConvertRelativePathToFull(RelativeContentPath);
	}

	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), RelativeContentPath);
}

UTexture2D* RDUITexture::LoadTextureFromContentPng(const FString& RelativeContentPath, const TCHAR* LogOwner)
{
	const FString ImagePath = ResolveContentFilePath(RelativeContentPath);
	TArray<uint8> CompressedData;
	if (FPaths::FileExists(ImagePath) == false || FFileHelper::LoadFileToArray(CompressedData, *ImagePath) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: PNG file missing: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid() || ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: failed to decode PNG: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	TArray64<uint8> RawData;
	if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: failed to extract PNG pixels: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	UTexture2D* LoadedTexture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
	if (LoadedTexture == nullptr || LoadedTexture->GetPlatformData() == nullptr || LoadedTexture->GetPlatformData()->Mips.Num() == 0)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: failed to create texture: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	LoadedTexture->SRGB = true;
	FTexture2DMipMap& Mip = LoadedTexture->GetPlatformData()->Mips[0];
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	Mip.BulkData.Unlock();
	LoadedTexture->UpdateResource();

	return LoadedTexture;
}
