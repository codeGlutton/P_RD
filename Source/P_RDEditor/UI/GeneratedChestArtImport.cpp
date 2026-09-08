#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/SavePackage.h"

namespace
{
	// Converts generated additive artwork to straight alpha for UMG's image brush.
	// Usage: RD.Editor.ImportChestArt <PNG> <AssetName> <light|atlas>
	void ImportChestArt(const TArray<FString>& Args)
	{
		if (Args.Num() != 3 || (Args[2] != TEXT("light") && Args[2] != TEXT("atlas"))) return;
		FImage Source, Image;
		if (!FImageUtils::LoadImage(*Args[0].TrimQuotes(), Source)) return;
		Source.CopyTo(Image, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		const bool bLight = Args[2] == TEXT("light");
		TArrayView64<FColor> Pixels = Image.AsBGRA8();
		for (int32 Y = 0; Y < Image.SizeY; ++Y)
		{
			for (int32 X = 0; X < Image.SizeX; ++X)
			{
				FColor& Pixel = Pixels[int64(Y) * Image.SizeX + X];
				if (bLight)
				{
					FLinearColor Color(Pixel);
					const float Alpha = FMath::Max3(Color.R, Color.G, Color.B);
					if (Alpha > .0005f)
					{
						Color /= Alpha;
						Color.A = Alpha;
						Pixel = Color.ToFColorSRGB();
					}
					else Pixel = FColor::Transparent;
				}
				else
				{
					// Black-key clean generated sprites; keep dark wood inside silhouettes.
					Pixel.A = uint8(FMath::RoundToInt(255.f * FMath::SmoothStep(0.f, 18.f,
						float(FMath::Max3(Pixel.R, Pixel.G, Pixel.B)))));
				}
				// A transparent guard at each cell edge prevents filtered atlas bleed.
				const float CellW = Image.SizeX / (bLight ? 1.f : 6.f);
				const float CellH = Image.SizeY / (bLight ? 1.f : 6.f);
				const float LocalX = FMath::Fmod(X + .5f, CellW);
				const float LocalY = FMath::Fmod(Y + .5f, CellH);
				const float Edge = FMath::Min(FMath::Min(LocalX, CellW - LocalX),
					FMath::Min(LocalY, CellH - LocalY));
				Pixel.A = uint8(Pixel.A * FMath::SmoothStep(1.f, bLight ? 12.f : 3.f, Edge));
			}
		}
		const FString PackageName = TEXT("/Game/UI/RewardConcept03New/Generated/") + Args[1];
		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();
		UTexture2D* Texture = FindObject<UTexture2D>(Package, *Args[1]);
		const bool bNew = Texture == nullptr;
		if (bNew) Texture = NewObject<UTexture2D>(Package, *Args[1], RF_Public | RF_Standalone);
		Texture->PreEditChange(nullptr);
		Texture->Source.Init(Image.SizeX, Image.SizeY, 1, 1, TSF_BGRA8, Image.RawData.GetData());
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->NeverStream = true;
		Texture->SRGB = true;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->PostEditChange();
		if (bNew) FAssetRegistryModule::AssetCreated(Texture);
		Package->MarkPackageDirty();
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		const FString File = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		if (!UPackage::SavePackage(Package, Texture, *File, SaveArgs)) return;
		TArray64<uint8> Png;
		TArray<FColor> ExportPixels;
		ExportPixels.Append(Pixels.GetData(), Pixels.Num());
		FImageUtils::PNGCompressImageArray(Image.SizeX, Image.SizeY, ExportPixels, Png);
		FFileHelper::SaveArrayToFile(Png, *FPaths::Combine(FPaths::ProjectSavedDir(), Args[1] + TEXT(".png")));
		UE_LOG(LogTemp, Display, TEXT("CHEST_ART_IMPORTED %s %dx%d"), *PackageName, Image.SizeX, Image.SizeY);
	}
	FAutoConsoleCommand ImportChestArtCommand(TEXT("RD.Editor.ImportChestArt"),
		TEXT("Import generated chest VFX PNG with alpha and texture guard pixels."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ImportChestArt));
}
