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
	// 압축된 PNG 바이트를 먼저 디스크에서 읽는다. 파일이 없거나 못 읽으면 크래시 대신
	// 경고만 남기고 nullptr를 반환한다 (UI 한 장이 빠져도 게임은 계속 떠야 하므로).
	const FString ImagePath = ResolveContentFilePath(RelativeContentPath);
	TArray<uint8> CompressedData;
	if (FPaths::FileExists(ImagePath) == false || FFileHelper::LoadFileToArray(CompressedData, *ImagePath) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: PNG file missing: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	// ImageWrapper 모듈로 PNG를 디코딩한다. 모듈/래퍼 생성이나 압축 해제에 실패해도 동일하게 안전 종료.
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid() || ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: failed to decode PNG: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	// 픽셀을 엔진 텍스처와 같은 BGRA8 포맷으로 뽑아낸다 (아래 CreateTransient의 PF_B8G8R8A8와 맞추기 위함).
	TArray64<uint8> RawData;
	if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData) == false)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: failed to extract PNG pixels: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	// 디스크 PNG는 임포트된 에셋이 아니므로 트랜션트 텍스처로 만든다 (저장되지 않고 실행 중에만 존재).
	UTexture2D* LoadedTexture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
	if (LoadedTexture == nullptr || LoadedTexture->GetPlatformData() == nullptr || LoadedTexture->GetPlatformData()->Mips.Num() == 0)
	{
		UE_LOG(LogRD, Warning, TEXT("%s: failed to create texture: %s"), LogOwner, *ImagePath);
		return nullptr;
	}

	// 디코딩한 픽셀을 0번 Mip 버퍼에 그대로 복사하고 GPU 리소스를 갱신한다.
	// UI 일러스트는 색 보정이 적용돼야 하므로 SRGB로 표시한다.
	LoadedTexture->SRGB = true;
	FTexture2DMipMap& Mip = LoadedTexture->GetPlatformData()->Mips[0];
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	Mip.BulkData.Unlock();
	LoadedTexture->UpdateResource();

	return LoadedTexture;
}
