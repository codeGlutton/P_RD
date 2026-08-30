#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
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
	UWidget* TurnAPScale = HUD->WidgetTree->FindWidget(TEXT("TurnAPScale"));
	const UCanvasPanelSlot* TurnAPSlot = TurnAPScale != nullptr
		? Cast<UCanvasPanelSlot>(TurnAPScale->Slot) : nullptr;
	if (!TestNotNull(TEXT("좌상단 AP 바 슬롯"), TurnAPSlot))
	{
		return false;
	}
	TestEqual(TEXT("AP 바 좌상단 좌표"), TurnAPSlot->GetPosition(),
		FVector2D(18.f, 164.f));
	TestEqual(TEXT("AP 바는 중앙 스킬 카드 전에서 끝나는 압축 크기"),
		TurnAPSlot->GetSize(), FVector2D(800.f, 97.f));
	const UTextBlock* InitialTurnAPText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnAPText")));
	if (TestNotNull(TEXT("AP 숫자"), InitialTurnAPText))
	{
		TestEqual(TEXT("압축 후에도 AP 숫자 크기를 유지"),
			InitialTurnAPText->GetFont().Size, 46.f);
	}
	const UWidget* MoveCard = HUD->WidgetTree->FindWidget(TEXT("CommandCard_0"));
	const UCanvasPanelSlot* MoveCardSlot = MoveCard != nullptr
		? Cast<UCanvasPanelSlot>(MoveCard->Slot) : nullptr;
	if (TestNotNull(TEXT("중앙 이동 카드 슬롯"), MoveCardSlot))
	{
		const float APBarRight = TurnAPSlot->GetPosition().X
			+ TurnAPSlot->GetSize().X;
		const float MoveCardLeft = 1920.f * MoveCardSlot->GetAnchors().Minimum.X
			+ MoveCardSlot->GetPosition().X
			- MoveCardSlot->GetAlignment().X * MoveCardSlot->GetSize().X;
		TestTrue(TEXT("AP 바와 중앙 이동 카드 사이에 40px 이상 여백"),
			MoveCardLeft - APBarRight >= 40.f);
	}
	for (int32 GroupIndex = 0; GroupIndex < 3; ++GroupIndex)
	{
		UWidget* GroupWell = HUD->WidgetTree->FindWidget(*FString::Printf(
			TEXT("TurnAPGroupWell_%d"), GroupIndex));
		TestNotNull(*FString::Printf(TEXT("AP 5칸 묶음 %d"), GroupIndex),
			GroupWell);
		TestTrue(*FString::Printf(TEXT("AP 5칸 묶음 %d 부모"), GroupIndex),
			GroupWell != nullptr && GroupWell->GetParent()
				== HUD->WidgetTree->FindWidget(TEXT("TurnAPPipRow")));
	}
	for (int32 SeparatorIndex = 0; SeparatorIndex < 2; ++SeparatorIndex)
	{
		UWidget* Separator = HUD->WidgetTree->FindWidget(*FString::Printf(
			TEXT("TurnAPGroupSeparator_%d"), SeparatorIndex));
		TestNotNull(*FString::Printf(TEXT("AP 묶음 구분선 %d"), SeparatorIndex),
			Separator);
		const UCanvasPanelSlot* SeparatorSlot = Separator != nullptr
			? Cast<UCanvasPanelSlot>(Separator->Slot) : nullptr;
		if (TestNotNull(*FString::Printf(TEXT("AP 묶음 구분선 슬롯 %d"),
			SeparatorIndex), SeparatorSlot))
		{
			TestEqual(*FString::Printf(TEXT("AP 묶음 구분선 위치 %d"),
				SeparatorIndex), SeparatorSlot->GetPosition(),
				FVector2D(148.f + 155.f * SeparatorIndex, 3.f));
			TestEqual(*FString::Printf(TEXT("AP 묶음 구분선 크기 %d"),
				SeparatorIndex), SeparatorSlot->GetSize(), FVector2D(3.f, 30.f));
		}
	}
	TestNull(TEXT("AP 묶음은 정확히 세 개"),
		HUD->WidgetTree->FindWidget(TEXT("TurnAPGroupWell_3")));
	TestNull(TEXT("AP 묶음 구분선은 정확히 두 개"),
		HUD->WidgetTree->FindWidget(TEXT("TurnAPGroupSeparator_2")));
	for (int32 PipIndex = 0; PipIndex < 15; ++PipIndex)
	{
		TestNotNull(*FString::Printf(TEXT("AP 활성 아이콘 %d"), PipIndex),
			HUD->WidgetTree->FindWidget(*FString::Printf(
				TEXT("TurnAPPip_%d"), PipIndex)));
		TestNotNull(*FString::Printf(TEXT("AP 소모 아이콘 %d"), PipIndex),
			HUD->WidgetTree->FindWidget(*FString::Printf(
				TEXT("TurnAPPipUsed_%d"), PipIndex)));
		TestNotNull(*FString::Printf(TEXT("AP 점등 레이어 %d"), PipIndex),
			HUD->WidgetTree->FindWidget(*FString::Printf(
				TEXT("TurnAPPipGlow_%d"), PipIndex)));
	}
	TestNull(TEXT("AP 아이콘은 15칸을 넘기지 않음"),
		HUD->WidgetTree->FindWidget(TEXT("TurnAPPip_15")));
	for (const TPair<int32, int32>& Boundary : {
		TPair<int32, int32>(4, 5), TPair<int32, int32>(9, 10) })
	{
		const UWidget* LeftPip = HUD->WidgetTree->FindWidget(*FString::Printf(
			TEXT("TurnAPPip_%d"), Boundary.Key));
		const UWidget* RightPip = HUD->WidgetTree->FindWidget(*FString::Printf(
			TEXT("TurnAPPip_%d"), Boundary.Value));
		const UCanvasPanelSlot* LeftSlot = LeftPip != nullptr
			? Cast<UCanvasPanelSlot>(LeftPip->Slot) : nullptr;
		const UCanvasPanelSlot* RightSlot = RightPip != nullptr
			? Cast<UCanvasPanelSlot>(RightPip->Slot) : nullptr;
		if (TestNotNull(TEXT("AP 묶음 왼쪽 경계 슬롯"), LeftSlot)
			&& TestNotNull(TEXT("AP 묶음 오른쪽 경계 슬롯"), RightSlot))
		{
			const float BoundaryGap = RightSlot->GetPosition().X
				- LeftSlot->GetPosition().X - LeftSlot->GetSize().X;
			TestEqual(TEXT("AP 5칸 묶음 사이 12px 여백"), BoundaryGap, 12.f);
		}
	}

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
	auto FindGlowPip = [HUD](int32 PipIndex)
	{
		return HUD->WidgetTree->FindWidget(
			*FString::Printf(TEXT("TurnAPPipGlow_%d"), PipIndex));
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
		UWidget* GlowPip = FindGlowPip(PipIndex);
		if (!TestNotNull(*FString::Printf(TEXT("AP 칸 %d"), PipIndex), Pip))
		{
			return false;
		}
		if (!TestNotNull(*FString::Printf(TEXT("AP 글로우 칸 %d"), PipIndex), GlowPip))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("원본 AP 칸 %d 선명도 유지"), PipIndex),
			Pip->GetRenderOpacity(), 1.f);
		TestFalse(*FString::Printf(TEXT("시작 전 AP 칸 %d 글로우 꺼짐"), PipIndex),
			IsShown(GlowPip));
		TestTrue(*FString::Printf(TEXT("AP 칸 %d 크기 고정"), PipIndex),
			FMath::IsNearlyEqual(Pip->GetRenderTransform().Scale.X, 1.f));
		TestTrue(*FString::Printf(TEXT("AP 칸 %d 위치 고정"), PipIndex),
			Pip->GetRenderTransform().Translation.IsNearlyZero());
		const UImage* PipImage = Cast<UImage>(Pip);
		if (PipIndex < 3)
		{
			TestTrue(*FString::Printf(TEXT("소모 예정 AP 칸 %d 어둡게 표시"), PipIndex),
				PipImage != nullptr && PipImage->GetColorAndOpacity().R < .40f);
		}
		else
		{
			TestTrue(*FString::Printf(TEXT("비소모 AP 칸 %d 보통 밝기"), PipIndex),
				PipImage != nullptr && PipImage->GetColorAndOpacity().Equals(
					FLinearColor::White));
		}
	}

	HUD->AdvancePendingAPGlowForTest(.12f);
	UWidget* FirstGlow = FindGlowPip(0);
	const UImage* FirstPipImage = Cast<UImage>(FindPip(0));
	TestTrue(TEXT("첫 AP 칸 어두움 뒤 보통 밝기"), FirstPipImage != nullptr
		&& FirstPipImage->GetColorAndOpacity().Equals(FLinearColor::White));
	TestFalse(TEXT("보통 밝기 단계에는 글로우 없음"), IsShown(FirstGlow));

	HUD->AdvancePendingAPGlowForTest(.08f);
	TestTrue(TEXT("첫 AP 칸 보통 밝기 뒤 점등"),
		FirstGlow != nullptr && IsShown(FirstGlow)
		&& FirstGlow->GetRenderOpacity() > .95f);
	const UImage* FirstGlowImage = Cast<UImage>(FirstGlow);
	TestTrue(TEXT("첫 AP 점등은 원본 보석 재질과 고정 크기 사용"),
		FirstGlowImage != nullptr
		&& FirstGlowImage->GetBrush().GetResourceObject() != nullptr
		&& FMath::IsNearlyEqual(FirstGlowImage->GetRenderTransform().Angle, 0.f)
		&& FMath::IsNearlyEqual(
			FirstGlowImage->GetRenderTransform().Scale.X, 1.f)
		&& FirstGlowImage->GetRenderTransform().Translation.IsNearlyZero());

	HUD->AdvancePendingAPGlowForTest(.20f);
	UWidget* SecondGlow = FindGlowPip(1);
	TestTrue(TEXT("두 번째 AP 칸이 다음 순서에 최고 밝기"),
		SecondGlow != nullptr && IsShown(SecondGlow)
		&& SecondGlow->GetRenderOpacity() > .95f);
	TestTrue(TEXT("두 번째 AP 점등 때 첫 칸은 감광"),
		FirstGlow != nullptr && FirstGlow->GetRenderOpacity() < .15f);

	HUD->AdvancePendingAPGlowForTest(.20f);
	UWidget* ThirdGlow = FindGlowPip(2);
	TestTrue(TEXT("세 번째 AP 칸이 왼쪽 순서 뒤에 최고 밝기"),
		ThirdGlow != nullptr && IsShown(ThirdGlow)
		&& ThirdGlow->GetRenderOpacity() > .95f);
	TestFalse(TEXT("세 번째 AP 점등 때 첫 칸 효과 종료"), IsShown(FirstGlow));

	HUD->AdvancePendingAPGlowForTest(.22f);
	for (int32 PipIndex = 0; PipIndex < 3; ++PipIndex)
	{
		TestFalse(*FString::Printf(TEXT("한 순회 뒤 AP 글로우 %d 휴지기"), PipIndex),
			IsShown(FindGlowPip(PipIndex)));
		const UImage* PipImage = Cast<UImage>(FindPip(PipIndex));
		TestTrue(*FString::Printf(TEXT("휴지기 AP 칸 %d 다시 어두움"), PipIndex),
			PipImage != nullptr && PipImage->GetColorAndOpacity().R < .40f);
	}

	HUD->AdvancePendingAPGlowForTest(.55f);
	HUD->AdvancePendingAPGlowForTest(.20f);
	TestTrue(TEXT("휴지기 뒤 첫 AP 칸부터 반복"),
		IsShown(FirstGlow) && FirstGlow->GetRenderOpacity() > .95f);

	PendingAction.mActionPointCost = 1;
	Model->SetPendingAction(PendingAction);
	for (int32 PipIndex = 0; PipIndex < 5; ++PipIndex)
	{
		UWidget* Pip = FindPip(PipIndex);
		UWidget* GlowPip = FindGlowPip(PipIndex);
		const bool bFirstPip = PipIndex == 0;
		if (bFirstPip)
		{
			const UImage* PipImage = Cast<UImage>(Pip);
			TestTrue(TEXT("경로 축소 후 첫 AP 칸만 어둡게 표시"),
				!IsShown(GlowPip) && PipImage != nullptr
				&& PipImage->GetColorAndOpacity().R < .40f);
		}
		else
		{
			TestFalse(*FString::Printf(TEXT("경로 축소 후 AP 글로우 %d 꺼짐"), PipIndex),
				IsShown(GlowPip));
		}
		TestEqual(*FString::Printf(TEXT("경로 축소 후 원본 AP 칸 %d 유지"), PipIndex),
			Pip->GetRenderOpacity(), 1.f);
	}
	HUD->AdvancePendingAPGlowForTest(.20f);
	TestTrue(TEXT("경로 축소 후 첫 AP 칸이 보통을 거쳐 점등"),
		IsShown(FindGlowPip(0)) && FindGlowPip(0)->GetRenderOpacity() > .95f);

	Model->SetPendingAction(FCombatPendingActionUI());
	for (int32 PipIndex = 0; PipIndex < 5; ++PipIndex)
	{
		UWidget* Pip = FindPip(PipIndex);
		UWidget* GlowPip = FindGlowPip(PipIndex);
		TestEqual(*FString::Printf(TEXT("취소 후 AP 원본 칸 %d 유지"), PipIndex),
			Pip->GetRenderOpacity(), 1.f);
		TestTrue(*FString::Printf(TEXT("취소 후 AP 원본 칸 %d 크기 복귀"), PipIndex),
			FMath::IsNearlyEqual(Pip->GetRenderTransform().Scale.X, 1.f));
		const UImage* PipImage = Cast<UImage>(Pip);
		TestTrue(*FString::Printf(TEXT("취소 후 AP 원본 칸 %d 색상 복귀"), PipIndex),
			PipImage != nullptr && PipImage->GetColorAndOpacity().Equals(
				FLinearColor::White));
		TestFalse(*FString::Printf(TEXT("취소 후 AP 글로우 %d 해제"), PipIndex),
			IsShown(GlowPip));
	}

	UTextBlock* TurnAPText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnAPText")));
	if (!TestNotNull(TEXT("좌상단 AP 숫자"), TurnAPText))
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
			*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 좌상단 AP 숫자"), CompletedSteps),
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
					*FString::Printf(TEXT("%d번째 이동 스텝 시작 후 좌상단 AP 강조 잔류 없음 %d"), CompletedSteps, PipIndex),
					ActivePip->GetRenderOpacity(),
					1.f);
			}
		}
	};

	// 유닛 이동 컴포넌트 모델이 스텝을 시작할 때 실제 AP를 1씩 차감한다. 새 유닛 스냅샷이
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

	// 표시 한도 15칸을 넘더라도 숫자는 실제 값을 유지하고, 보이는 첫 칸부터
	// 비용 피드백을 계속 제공한다.
	PlayerUnit.mActionPoints = 18;
	PlayerUnit.mMaxActionPoints = 18;
	PlayerUnit.mMovementPoint = 18.f;
	PlayerUnit.mMaxMovementPoint = 18.f;
	Model->SetUnitUIs({ PlayerUnit });
	PendingAction.mActionPointCost = 1;
	Model->SetPendingAction(PendingAction);
	TestEqual(TEXT("15칸 표시 한도 초과 AP 숫자"),
		TurnAPText->GetText().ToString(), FString(TEXT("18/18")));
	for (int32 PipIndex = 0; PipIndex < 15; ++PipIndex)
	{
		UWidget* GlowPip = FindGlowPip(PipIndex);
		if (PipIndex == 0)
		{
			const UImage* PipImage = Cast<UImage>(FindPip(PipIndex));
			TestTrue(TEXT("18/18 비용 1도 첫 보이는 AP가 어둡게 표시됨"),
				!IsShown(GlowPip) && PipImage != nullptr
				&& PipImage->GetColorAndOpacity().R < .40f);
		}
		else
		{
			TestFalse(*FString::Printf(
				TEXT("18/18 비용 1에서 나머지 AP 글로우 %d 꺼짐"), PipIndex),
				IsShown(GlowPip));
		}
	}
	HUD->AdvancePendingAPGlowForTest(.20f);
	TestTrue(TEXT("18/18 비용 1도 첫 보이는 AP가 순서대로 점등됨"),
		IsShown(FindGlowPip(0))
		&& FindGlowPip(0)->GetRenderOpacity() > .95f);

	return true;
}

#endif
