#include "Misc/AutomationTest.h"
#include "Component/SkillAnimationComponent/SkillAnimationComponent.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkillAnimationCameraShakeOptionTest,
	"P_RD.Component.SkillAnimation.CameraShakeOption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSkillAnimationCameraShakeOptionTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("옵션 데이터가 없는 프리뷰는 기존 흔들림 허용"),
		RDSkillAnimation::ShouldStartCameraShake(nullptr));

	TStrongObjectPtr<UOptionPersistData> Options(NewObject<UOptionPersistData>());
	if (!TestNotNull(TEXT("옵션 영구 데이터"), Options.Get()))
	{
		return false;
	}

	Options->SetCameraShakeEnabled(true);
	TestTrue(TEXT("카메라 흔들림 ON이면 연출 허용"),
		RDSkillAnimation::ShouldStartCameraShake(Options.Get()));

	Options->SetCameraShakeEnabled(false);
	TestFalse(TEXT("카메라 흔들림 OFF면 연출 차단"),
		RDSkillAnimation::ShouldStartCameraShake(Options.Get()));
	return true;
}

#endif
