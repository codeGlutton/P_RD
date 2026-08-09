#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatMoveAPPreviewTest,
	"P_RD.UI.CombatHUD.MoveAPPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatMoveAPPreviewTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	if (GEngine != nullptr)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != nullptr)
			{
				World = Context.World();
				break;
			}
		}
	}
	if (World == nullptr)
	{
		AddError(TEXT("HUD 이동 AP 테스트에 필요한 월드가 없음"));
		return false;
	}

	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD 클래스"), HUDClass))
	{
		return false;
	}

	UCombatLayoutHUDWidget* HUD = CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->mUsePreviewData = false;
	HUD->TakeWidget();

	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);

	FUnitUI PlayerUnit;
	PlayerUnit.mUnitId = 101;
	PlayerUnit.mIsPlayer = true;
	PlayerUnit.mActionPoints = 5;
	PlayerUnit.mMaxActionPoints = 5;
	PlayerUnit.mMovementPoint = 5.f;
	PlayerUnit.mMaxMovementPoint = 5.f;
	Model->SetUnitUIs({ PlayerUnit });

	FTurnUI PlayerTurn;
	PlayerTurn.mCurrentUnitId = PlayerUnit.mUnitId;
	PlayerTurn.mTurnOrderUnitIds.Add(PlayerUnit.mUnitId);
	Model->SetTurnUI(PlayerTurn);

	auto FindPip = [HUD](int32 PipIndex)
	{
		return HUD->WidgetTree->FindWidget(
			*FString::Printf(TEXT("TurnAPPip_%d"), PipIndex));
	};
	auto FindUsedPip = [HUD](int32 PipIndex)
	{
		return HUD->WidgetTree->FindWidget(
			*FString::Printf(TEXT("TurnAPPipUsed_%d"), PipIndex));
	};
	auto IsShown = [](const UWidget* Widget)
	{
		return Widget != nullptr
			&& Widget->GetVisibility() != ESlateVisibility::Collapsed
			&& Widget->GetVisibility() != ESlateVisibility::Hidden;
	};

	FCombatPendingActionUI PendingAction;
	PendingAction.mType = ECombatPendingActionType::Move;
	PendingAction.mActionPointCost = 3;
	Model->SetPendingAction(PendingAction);

	TestEqual(TEXT("이동 프리뷰 종류"), Model->GetPendingAction().mType,
		ECombatPendingActionType::Move);
	TestEqual(TEXT("이동 프리뷰 비용"), Model->GetPendingAction().mActionPointCost, 3);

	for (int32 PipIndex = 0; PipIndex < 5; ++PipIndex)
	{
		UWidget* Pip = FindPip(PipIndex);
		if (!TestNotNull(*FString::Printf(TEXT("AP 칸 %d"), PipIndex), Pip))
		{
			return false;
		}

		if (PipIndex >= 2)
		{
			TestTrue(*FString::Printf(TEXT("AP 칸 %d 예정 소모 강조"), PipIndex),
				Pip->GetRenderOpacity() < 1.f);
		}
		else
		{
			TestEqual(*FString::Printf(TEXT("AP 칸 %d 유지"), PipIndex),
				Pip->GetRenderOpacity(), 1.f);
		}
	}

	PendingAction.mActionPointCost = 1;
	Model->SetPendingAction(PendingAction);
	for (int32 PipIndex = 0; PipIndex < 5; ++PipIndex)
	{
		UWidget* Pip = FindPip(PipIndex);
		const bool bLastPip = PipIndex == 4;
		if (bLastPip)
		{
			TestTrue(TEXT("경로 축소 후 마지막 AP 칸만 강조"),
				Pip->GetRenderOpacity() < 1.f);
		}
		else
		{
			TestEqual(*FString::Printf(TEXT("경로 축소 후 AP 칸 %d 원복"), PipIndex),
				Pip->GetRenderOpacity(), 1.f);
		}
	}

	Model->SetPendingAction(FCombatPendingActionUI());
	for (int32 PipIndex = 0; PipIndex < 5; ++PipIndex)
	{
		UWidget* Pip = FindPip(PipIndex);
		TestEqual(*FString::Printf(TEXT("취소 후 AP 칸 %d 강조 해제"), PipIndex),
			Pip->GetRenderOpacity(), 1.f);
	}

	UTextBlock* TurnAPText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnAPText")));
	if (!TestNotNull(TEXT("중앙 AP 숫자"), TurnAPText))
	{
		return false;
	}
	UTextBlock* PartyAPText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("PartyAPText_0")));
	if (!TestNotNull(TEXT("파티 AP 숫자"), PartyAPText))
	{
		return false;
	}

	auto VerifyStepPresentation = [
		this,
		&FindPip,
		&FindUsedPip,
		&IsShown,
		TurnAPText,
		PartyAPText](const int32 CompletedSteps, const int32 ExpectedLeft)
	{
		const FString ExpectedText = FString::Printf(TEXT("%d/5"), ExpectedLeft);
		TestEqual(
			*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 중앙 AP 숫자"), CompletedSteps),
			TurnAPText->GetText().ToString(),
			ExpectedText);
		TestEqual(
			*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 파티 AP 숫자"), CompletedSteps),
			PartyAPText->GetText().ToString(),
			ExpectedText);

		for (int32 PipIndex = 0; PipIndex < 5; ++PipIndex)
		{
			UWidget* ActivePip = FindPip(PipIndex);
			UWidget* UsedPip = FindUsedPip(PipIndex);
			TestNotNull(
				*FString::Printf(TEXT("활성 AP 칸 %d"), PipIndex),
				ActivePip);
			TestNotNull(
				*FString::Printf(TEXT("사용 AP 칸 %d"), PipIndex),
				UsedPip);

			const bool bExpectedActive = PipIndex < ExpectedLeft;
			TestEqual(
				*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 활성 AP 칸 %d"), CompletedSteps, PipIndex),
				IsShown(ActivePip),
				bExpectedActive);
			TestEqual(
				*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 사용 AP 칸 %d"), CompletedSteps, PipIndex),
				IsShown(UsedPip),
				!bExpectedActive);

			if (bExpectedActive)
			{
				TestEqual(
					*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 중앙 AP 강조 잔류 없음 %d"), CompletedSteps, PipIndex),
					ActivePip->GetRenderOpacity(),
					1.f);
			}
		}
	};

	// 이동 액션이 스텝을 시작할 때 실제 AP를 1씩 차감한다. 새 유닛 스냅샷이
	// 들어올 때마다 중앙·파티 숫자와 보석도 4 -> 3 -> 2로 함께 갱신되어야 한다.
	for (int32 StepIndex = 1; StepIndex <= 3; ++StepIndex)
	{
		const int32 ExpectedLeft = 5 - StepIndex;
		PlayerUnit.mActionPoints = ExpectedLeft;
		PlayerUnit.mMovementPoint = static_cast<float>(ExpectedLeft);
		Model->SetUnitUIs({ PlayerUnit });
		TestEqual(
			*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 표시 AP"), StepIndex),
			Model->GetDisplayedMovementPoint(PlayerUnit),
			ExpectedLeft);
		VerifyStepPresentation(StepIndex, ExpectedLeft);
	}

	return true;
}

#endif
