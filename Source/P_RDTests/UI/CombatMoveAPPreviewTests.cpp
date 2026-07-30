#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
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
		AddInfo(TEXT("월드가 없어 건너뜀"));
		return true;
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

	return true;
}

#endif
