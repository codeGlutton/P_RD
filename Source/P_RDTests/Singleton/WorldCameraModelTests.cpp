#include "Misc/AutomationTest.h"
#include "Singleton/WorldSubsystem/WorldCameraModel.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldCameraModelAcknowledgementTest,
	"P_RD.Camera.WorldCameraModel.Acknowledgement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWorldCameraModelAcknowledgementTest::RunTest(const FString& Parameters)
{
	UWorldCameraModel* Model = NewObject<UWorldCameraModel>();
	if (!TestNotNull(TEXT("카메라 모델"), Model))
	{
		return false;
	}

	int32 ReturnNotificationCount = 0;
	Model->OnMainCameraReturned.AddLambda(
		[&ReturnNotificationCount]()
		{
			++ReturnNotificationCount;
		});

	// 중계 델리게이트만 붙고 실제 Pawn/Component가 없는 월드를 흉내 낸다.
	Model->OnRequestZoomInMainCamera.AddLambda(
		[](const FVector& /*Location*/, float /*ScreenSize*/)
		{
		});
	Model->RequestZoomInMainCamera(FVector::ZeroVector, 1000.f);
	TestFalse(TEXT("요청 구독자만으로는 강조 상태가 되지 않는다"),
		Model->IsMainCameraEmphasized());

	Model->NotifyMainCameraEmphasisStarted();
	TestTrue(TEXT("실제 카메라가 수락하면 강조 상태가 된다"),
		Model->IsMainCameraEmphasized());

	// 줌아웃을 받을 뷰가 사라졌다면 즉시 복귀한 것으로 처리한다.
	Model->RequestZoomOutMainCamera();
	TestFalse(TEXT("뷰가 없으면 줌아웃 요청 즉시 강조 상태를 푼다"),
		Model->IsMainCameraEmphasized());
	TestEqual(TEXT("복귀 알림은 한 번 발생한다"),
		ReturnNotificationCount, 1);

	Model->NotifyMainCameraReturned();
	TestEqual(TEXT("중복 복귀 알림은 무시한다"),
		ReturnNotificationCount, 1);
	return true;
}

#endif
