#include "Misc/AutomationTest.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOptionOverallQualityPersistenceTest,
	"P_RD.Singleton.Option.OverallQualityPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOptionOverallQualityPersistenceTest::RunTest(const FString& Parameters)
{
	UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings();
	if (!TestNotNull(TEXT("전역 게임 유저 세팅"), UserSettings))
	{
		return false;
	}

	// SetOverallQuality applies and saves global engine settings. Preserve the
	// complete mixed quality state (not merely the new object's Medium default),
	// plus the value and priority of the project-owned ScreenPercentage CVar.
	const Scalability::FQualityLevels OriginalScalability =
		UserSettings->ScalabilityQuality;
	IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(
		TEXT("r.ScreenPercentage"));
	const FString OriginalScreenPercentage = ScreenPercentage != nullptr
		? ScreenPercentage->GetString() : FString();
	const EConsoleVariableFlags OriginalScreenPercentagePriority = ScreenPercentage != nullptr
		? StaticCast<EConsoleVariableFlags>(ScreenPercentage->GetFlags() & ECVF_SetByMask)
		: ECVF_SetByConstructor;
	ON_SCOPE_EXIT
	{
		UserSettings->ScalabilityQuality = OriginalScalability;
		UserSettings->ApplyNonResolutionSettings();
		UserSettings->SaveSettings();

		if (ScreenPercentage != nullptr)
		{
			IConsoleVariable::FSetContext RestoreContext(
				OriginalScreenPercentagePriority, NAME_None);
			RestoreContext.PriorityMode =
				IConsoleVariable::FSetContext::EPriorityMode::ReplaceCurrent;
			RestoreContext.MinPriority = ECVF_SetByConstructor;
			RestoreContext.MaxPriority = ECVF_SetByConsole;
			ScreenPercentage->Set(*OriginalScreenPercentage, RestoreContext);
		}
	};

	TStrongObjectPtr<UOptionPersistData> Options(NewObject<UOptionPersistData>());
	if (!TestNotNull(TEXT("옵션 영구 데이터"), Options.Get()))
	{
		return false;
	}

	Options->SetOverallQuality(EOverallQualityType::Low);
	TestEqual(TEXT("낮음 품질을 영구 데이터에 반영"),
		Options->GetOverallQuality(), EOverallQualityType::Low);

	Options->SetOverallQuality(EOverallQualityType::High);
	TestEqual(TEXT("높음 품질을 영구 데이터에 반영"),
		Options->GetOverallQuality(), EOverallQualityType::High);

	return true;
}

#endif
