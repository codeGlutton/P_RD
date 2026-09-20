#include "Misc/AutomationTest.h"

#include "Sound/SoundBase.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatResultAudioCueSelectionTest,
	"P_RD.UI.CombatResult.AudioCueSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatResultAudioCueSelectionTest::RunTest(const FString& Parameters)
{
	const UCombatLayoutHUDWidget* WidgetCDO = GetDefault<UCombatLayoutHUDWidget>();
	if (TestNotNull(TEXT("전투 HUD 기본 객체"), WidgetCDO) == false)
	{
		return false;
	}

	USoundBase* VictoryCue = WidgetCDO->SelectCombatResultJingleForTest(true);
	TestNotNull(TEXT("승리에는 검증된 승리 징글을 선택한다"), VictoryCue);
	if (VictoryCue != nullptr)
	{
		TestEqual(TEXT("승리 징글 경로"), VictoryCue->GetPathName(),
			FString(TEXT("/Game/SVN/OutSideAsset/Music/OpenGameArt/Jingle/"
				"BGM_Jingle_Victory_Fupi_CC0.BGM_Jingle_Victory_Fupi_CC0")));
	}

	TestNull(TEXT("패배에는 검증되지 않은 승리성 징글을 재생하지 않는다"),
		WidgetCDO->SelectCombatResultJingleForTest(false));
	return true;
}

#endif
