#include "Misc/AutomationTest.h"

#include "Misc/ConfigCacheIni.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace AndroidPackagingSettingsTests
{
	constexpr const TCHAR* AndroidSettingsSection =
		TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAndroidLandscapeOnlyPackagingSettingsTest,
	"P_RD.Platform.Android.LandscapeOnlyPackagingSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAndroidLandscapeOnlyPackagingSettingsTest::RunTest(const FString& Parameters)
{
	using namespace AndroidPackagingSettingsTests;

	FString Orientation;
	const bool bHasOrientation = GConfig != nullptr
		&& GConfig->GetString(AndroidSettingsSection, TEXT("Orientation"), Orientation,
			GEngineIni);
	TestTrue(TEXT("Android orientation must be explicitly configured"), bHasOrientation);
	TestEqual(TEXT("Android manifest must use fixed landscape, not a sensor mode"),
		Orientation, FString(TEXT("Landscape")));

	bool bAllowResizing = true;
	const bool bHasAllowResizing = GConfig != nullptr
		&& GConfig->GetBool(AndroidSettingsSection, TEXT("bAllowResizing"),
			bAllowResizing, GEngineIni);
	TestTrue(TEXT("Android resizing policy must be explicit"), bHasAllowResizing);
	TestFalse(TEXT("Landscape-only UI must not opt into activity resizing"),
		bAllowResizing);

	bool bSupportSizeChanges = true;
	const bool bHasSupportSizeChanges = GConfig != nullptr
		&& GConfig->GetBool(AndroidSettingsSection, TEXT("bSupportSizeChanges"),
			bSupportSizeChanges, GEngineIni);
	TestTrue(TEXT("Fold/flip size-change policy must be explicit"),
		bHasSupportSizeChanges);
	TestFalse(TEXT("Landscape-only UI must not opt into fold/flip size changes"),
		bSupportSizeChanges);

	TArray<FString> ExtraApplicationNodeTags;
	if (GConfig != nullptr)
	{
		GConfig->GetArray(AndroidSettingsSection, TEXT("ExtraApplicationNodeTags"),
			ExtraApplicationNodeTags, GEngineIni);
	}
	const bool bDeclaresGameCategory = ExtraApplicationNodeTags.ContainsByPredicate(
		[](const FString& Tag)
		{
			return Tag.Contains(TEXT("android:appCategory"),
				ESearchCase::IgnoreCase)
				&& Tag.Contains(TEXT("game"), ESearchCase::IgnoreCase);
		});
	TestTrue(TEXT("Android 16 large-screen orientation exception requires the game app category"),
		bDeclaresGameCategory);

	return !HasAnyErrors();
}

#endif
