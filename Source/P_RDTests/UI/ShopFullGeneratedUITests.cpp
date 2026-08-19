/*****************************************************************//**
 * @file   ShopFullGeneratedUITests.cpp
 * @brief  전용 생성 아트 상점의 텍스처 계약과 기존 상점 WBP 보존을 검증한다.
 * @date   2026-08-12
 *********************************************************************/

#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace ShopFullGeneratedUITests
{
	constexpr TCHAR TextureRoot[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ShopFullGenerated");
	constexpr TCHAR WidgetPackage[] =
		TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated");
	constexpr TCHAR RailScrimTexturePackage[] =
		TEXT("/Game/UI/Shop/T_ShopFG_RailScrimGradient");
	constexpr TCHAR ShopGameModePackage[] =
		TEXT("/Game/BP/GameMode/BP_ShopGameMode");
	const TCHAR* ReusedChromeTexturePackages[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_MB_RoundBadge_Frame"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Button_Wide_Normal"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/HUD04/T_Combat_Button_Wood_SkillConfirm_20260811_v3"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/T_SkillCard_Frame_Combat"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/T_KitA_Cell_Normal"),
		TEXT("/Game/UI/Shop/T_ShopFG_RestUnitPanel_Clean"),
		TEXT("/Game/UI/Shop/T_ShopFG_SkillSlot07_Square"),
		TEXT("/Game/UI/Shop/T_ShopFG_SkillSlot07_Square_Selected"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Hire/T_MB_HireNamePlate"),
	};

	struct FExpectedTexture
	{
		const TCHAR* Folder;
		const TCHAR* Name;
		int32 Width;
		int32 Height;
		bool bRequiresAlpha;
	};

	const FExpectedTexture ExpectedTextures[] = {
		{ TEXT("Backgrounds"), TEXT("T_ShopFG_Artifact_Background"), 1600, 1000, false },
		{ TEXT("Backgrounds"), TEXT("T_ShopFG_Skill_Background"), 1600, 1000, false },
		{ TEXT("Backgrounds"), TEXT("T_ShopFG_Rest_Background"), 1600, 1000, false },
		{ TEXT("Chrome"), TEXT("T_ShopFG_TitlePlate"), 768, 192, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_Tab_Normal"), 512, 160, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_Tab_Selected"), 512, 160, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_GoldPlate"), 640, 160, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_RailCard_Normal"), 512, 640, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_RailCard_Selected"), 512, 640, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_UnitSlot_Normal"), 256, 256, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_UnitSlot_Selected"), 256, 256, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_SkillSlot_Normal"), 320, 224, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_SkillSlot_Selected"), 320, 224, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_RestUnitPanel"), 1280, 192, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_RestCostPlate"), 512, 144, true },
		{ TEXT("Chrome"), TEXT("T_ShopFG_SelectionPointer"), 96, 144, true },
		{ TEXT("Controls"), TEXT("T_ShopFG_Button_Back"), 512, 192, true },
		{ TEXT("Controls"), TEXT("T_ShopFG_Button_Primary"), 512, 192, true },
		{ TEXT("Controls"), TEXT("T_ShopFG_Arrow_Left"), 128, 192, true },
		{ TEXT("Controls"), TEXT("T_ShopFG_Arrow_Right"), 128, 192, true },
		{ TEXT("Meters"), TEXT("T_ShopFG_MeterTrack"), 512, 48, true },
		{ TEXT("Meters"), TEXT("T_ShopFG_MeterFill"), 512, 48, true },
	};

	FString TextureObjectPath(const FExpectedTexture& Expected)
	{
		return FString::Printf(TEXT("%s/%s/%s.%s"), TextureRoot,
			Expected.Folder, Expected.Name, Expected.Name);
	}

	FString TexturePackagePath(const FExpectedTexture& Expected)
	{
		return FString::Printf(TEXT("%s/%s/%s"), TextureRoot,
			Expected.Folder, Expected.Name);
	}

	bool ContainsBytes(const TArray<uint8>& Haystack, const TArray<uint8>& Needle)
	{
		if (Needle.IsEmpty() || Haystack.Num() < Needle.Num())
		{
			return false;
		}

		for (int32 Offset = 0; Offset <= Haystack.Num() - Needle.Num(); ++Offset)
		{
			if (FMemory::Memcmp(Haystack.GetData() + Offset,
				Needle.GetData(), Needle.Num()) == 0)
			{
				return true;
			}
		}
		return false;
	}

	/** UAsset 이름 테이블은 버전에 따라 ANSI 또는 UTF-16LE로 기록될 수 있다. */
	bool PackageContainsString(const TArray<uint8>& PackageBytes, const FString& Value)
	{
		FTCHARToUTF8 Utf8(*Value);
		TArray<uint8> AnsiNeedle;
		AnsiNeedle.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		if (ContainsBytes(PackageBytes, AnsiNeedle))
		{
			return true;
		}

		TArray<uint8> Utf16Needle;
		Utf16Needle.Reserve(Value.Len() * 2);
		for (const TCHAR Character : Value)
		{
			Utf16Needle.Add(static_cast<uint8>(Character & 0xff));
			Utf16Needle.Add(static_cast<uint8>((Character >> 8) & 0xff));
		}
		return ContainsBytes(PackageBytes, Utf16Needle);
	}

	FString PackageFilename(const TCHAR* LongPackageName)
	{
		return FPackageName::LongPackageNameToFilename(
			LongPackageName, FPackageName::GetAssetPackageExtension());
	}

	bool SourceContainsTransparentPixel(UTexture2D& Texture)
	{
		FImage SourceImage;
		if (!Texture.Source.GetMipImage(SourceImage, 0, 0, 0))
		{
			return false;
		}

		for (int32 Y = 0; Y < SourceImage.SizeY; ++Y)
		{
			for (int32 X = 0; X < SourceImage.SizeX; ++X)
			{
				if (SourceImage.GetOnePixelLinear(X, Y).A < 0.999f)
				{
					return true;
				}
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopFullGeneratedTextureContractTest,
	"P_RD.UI.ShopFullGenerated.TextureContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopFullGeneratedTextureContractTest::RunTest(const FString& Parameters)
{
	using namespace ShopFullGeneratedUITests;
	bool bAllValid = true;
	TestEqual(TEXT("전용 생성 상점 텍스처 수"),
		static_cast<int32>(UE_ARRAY_COUNT(ExpectedTextures)), 22);

	for (const FExpectedTexture& Expected : ExpectedTextures)
	{
		const FString ObjectPath = TextureObjectPath(Expected);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("전용 생성 텍스처: %s"),
			*ObjectPath), Texture))
		{
			bAllValid = false;
			continue;
		}

		const FIntPoint ImportedSize = Texture->GetImportedSize();
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s 폭"), Expected.Name),
			ImportedSize.X, Expected.Width);
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s 높이"), Expected.Name),
			ImportedSize.Y, Expected.Height);
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s UI LOD"), Expected.Name),
			Texture->LODGroup, TEnumAsByte<TextureGroup>(TEXTUREGROUP_UI));
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s NoMipmaps"), Expected.Name),
			Texture->MipGenSettings,
			TEnumAsByte<TextureMipGenSettings>(TMGS_NoMipmaps));
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s UI 압축"), Expected.Name),
			Texture->CompressionSettings,
			TEnumAsByte<TextureCompressionSettings>(TC_EditorIcon));
		bAllValid &= TestTrue(*FString::Printf(TEXT("%s sRGB"), Expected.Name),
			Texture->SRGB);
		bAllValid &= TestTrue(*FString::Printf(TEXT("%s NeverStream"), Expected.Name),
			Texture->NeverStream);
		bAllValid &= TestFalse(*FString::Printf(TEXT("%s 가상 텍스처 비활성"), Expected.Name),
			Texture->VirtualTextureStreaming);
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s X Clamp"), Expected.Name),
			Texture->AddressX, TEnumAsByte<TextureAddress>(TA_Clamp));
		bAllValid &= TestEqual(*FString::Printf(TEXT("%s Y Clamp"), Expected.Name),
			Texture->AddressY, TEnumAsByte<TextureAddress>(TA_Clamp));
		if (Expected.bRequiresAlpha)
		{
			// HasAlphaChannel()은 플랫폼 GPU 포맷의 알파 지원 여부만 반환한다.
			// UI 압축/NullRHI에서도 실제 소스 투명 픽셀이 보존됐는지 직접 확인한다.
			bAllValid &= TestTrue(*FString::Printf(TEXT("%s 소스 투명 픽셀"),
				Expected.Name), SourceContainsTransparentPixel(*Texture));
		}
	}
	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopFullGeneratedDependencyContractTest,
	"P_RD.UI.ShopFullGenerated.StaticDependencyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopFullGeneratedDependencyContractTest::RunTest(const FString& Parameters)
{
	using namespace ShopFullGeneratedUITests;
	const FString Filename = PackageFilename(WidgetPackage);
	TArray<uint8> PackageBytes;
	if (!TestTrue(TEXT("전용 생성 상점 WBP 패키지를 읽을 수 있음"),
		FFileHelper::LoadFileToArray(PackageBytes, *Filename)))
	{
		return false;
	}

	bool bAllValid = true;
	// The first seven generated textures and the two former footer buttons remain
	// validated as source assets, but this WBP deliberately reuses combat HUD chrome.
	for (int32 Index = 7; Index < UE_ARRAY_COUNT(ExpectedTextures); ++Index)
	{
		if (Index == 7 || Index == 8 || Index == 9 || Index == 10
			|| Index == 11 || Index == 12 || Index == 13 || Index == 14
			|| Index == 15 || Index == 16 || Index == 17)
		{
			continue;
		}
		const FExpectedTexture& Expected = ExpectedTextures[Index];
		const FString PackagePath = TexturePackagePath(Expected);
		bAllValid &= TestTrue(*FString::Printf(TEXT("새 WBP가 전용 파츠를 참조: %s"),
			*PackagePath), PackageContainsString(PackageBytes, PackagePath));
	}
	for (const TCHAR* PackagePath : ReusedChromeTexturePackages)
	{
		bAllValid &= TestTrue(*FString::Printf(TEXT("새 WBP가 전투 HUD 헤더 파츠를 참조: %s"),
			PackagePath), PackageContainsString(PackageBytes, PackagePath));
	}
	bAllValid &= TestFalse(TEXT("새 WBP가 제거된 검은 레일 그라데이션을 참조하지 않음"),
		PackageContainsString(PackageBytes, RailScrimTexturePackage));

	// 아이콘은 런타임 모델이 공급한다. 새 WBP에 예시 아이콘이나 이전 UI 크롬을
	// 시드하면 새 디자인이 다시 기존 에셋에 의존하므로 패키지 단계에서 막는다.
	const TCHAR* ForbiddenStaticDependencies[] = {
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Defeat/"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"),
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/"),
	};
	for (const TCHAR* ForbiddenPrefix : ForbiddenStaticDependencies)
	{
		bAllValid &= TestFalse(*FString::Printf(TEXT("새 WBP 기존 정적 의존 금지: %s"),
			ForbiddenPrefix), PackageContainsString(PackageBytes, ForbiddenPrefix));
	}

	TArray<uint8> GameModePackageBytes;
	bAllValid &= TestTrue(TEXT("BP_ShopGameMode 패키지를 읽을 수 있음"),
		FFileHelper::LoadFileToArray(GameModePackageBytes,
			*PackageFilename(ShopGameModePackage)));
	bAllValid &= TestTrue(TEXT("인게임 상점 HUD가 새 WBP를 참조"),
		PackageContainsString(GameModePackageBytes, WidgetPackage));
	return bAllValid;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
