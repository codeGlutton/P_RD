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
			*FString::Printf(TEXT("%d칸 도착 후 중앙 AP 숫자"), CompletedSteps),
			TurnAPText->GetText().ToString(),
			ExpectedText);
		TestEqual(
			*FString::Printf(TEXT("%d칸 도착 후 파티 AP 숫자"), CompletedSteps),
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
				*FString::Printf(TEXT("%d칸 도착 후 활성 AP 칸 %d"), CompletedSteps, PipIndex),
				IsShown(ActivePip),
				bExpectedActive);
			TestEqual(
				*FString::Printf(TEXT("%d칸 도착 후 사용 AP 칸 %d"), CompletedSteps, PipIndex),
				IsShown(UsedPip),
				!bExpectedActive);

			if (bExpectedActive)
			{
				TestEqual(
					*FString::Printf(TEXT("%d칸 도착 후 중앙 AP 강조 잔류 없음 %d"), CompletedSteps, PipIndex),
					ActivePip->GetRenderOpacity(),
					1.f);
			}
		}
	};

	// 실제 Movement는 아직 5인 채로, 물리적으로 타일에 도착할 때마다 표시만
	// 4 -> 3 -> 2로 내려간다.
	for (int32 CompletedSteps = 1; CompletedSteps <= 3; ++CompletedSteps)
	{
		Model->SetMoveAPStepPresentation(PlayerUnit.mUnitId, CompletedSteps);
		const int32 ExpectedLeft = 5 - CompletedSteps;
		TestEqual(
			*FString::Printf(TEXT("%d칸 도착 후 표시 AP"), CompletedSteps),
			Model->ResolveDisplayedMovementPoint(PlayerUnit),
			ExpectedLeft);
		VerifyStepPresentation(CompletedSteps, ExpectedLeft);
	}

	// 이동 중 HP/상태 등의 이유로 실제 유닛 스냅샷이 다시 와도 AP 연출은
	// 완료 칸 수를 유지한다.
	Model->SetUnitUIs({ PlayerUnit });
	TestEqual(TEXT("이동 중 실제 스냅샷 재수신 후 표시 AP 유지"),
		Model->ResolveDisplayedMovementPoint(PlayerUnit), 2);
	VerifyStepPresentation(3, 2);

	// 중단/취소되면 실제 AP를 내지 않았으므로 즉시 5로 복구한다.
	Model->ClearMoveAPStepPresentation(INDEX_NONE);
	TestEqual(TEXT("이동 취소 후 실제 AP 복구"),
		Model->ResolveDisplayedMovementPoint(PlayerUnit), 5);
	VerifyStepPresentation(0, 5);

	// 타일 효과가 Movement를 1 줄인 경우에도 다음 도착 표시는
	// 실제 4 - 이동 완료 2칸 = 2로 다시 맞춘다.
	Model->SetMoveAPStepPresentation(PlayerUnit.mUnitId, 1);
	PlayerUnit.mMovementPoint = 4.f;
	Model->SetUnitUIs({ PlayerUnit });
	Model->SetMoveAPStepPresentation(PlayerUnit.mUnitId, 2);
	TestEqual(TEXT("타일 효과 AP 변경 후 이동 표시 재계산"),
		Model->ResolveDisplayedMovementPoint(PlayerUnit), 2);
	VerifyStepPresentation(2, 2);
	Model->ClearMoveAPStepPresentation(INDEX_NONE);
	TestEqual(TEXT("타일 효과 후 취소 시 실제 AP 유지"),
		Model->ResolveDisplayedMovementPoint(PlayerUnit), 4);

	PlayerUnit.mMovementPoint = 5.f;
	Model->SetUnitUIs({ PlayerUnit });

	// 정상 종료에서는 실제 AP 2가 먼저 들어온 뒤 override를 해제하므로 화면이
	// 2에서 튀지 않는다.
	Model->SetMoveAPStepPresentation(PlayerUnit.mUnitId, 3);
	PlayerUnit.mMovementPoint = 2.f;
	Model->SetUnitUIs({ PlayerUnit });
	VerifyStepPresentation(3, 2);
	Model->ClearMoveAPStepPresentation(INDEX_NONE);
	TestEqual(TEXT("정상 종료 후 실제 AP와 표시 AP 일치"),
		Model->ResolveDisplayedMovementPoint(PlayerUnit), 2);
	VerifyStepPresentation(3, 2);

	return true;
}

#endif
