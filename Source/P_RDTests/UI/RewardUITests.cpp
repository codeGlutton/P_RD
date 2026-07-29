/*****************************************************************//**
 * @file   RewardUITests.cpp
 * @brief  보상 화면의 즉시 표시와 request/confirmed 상태 분리를 검증한다.
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "Components/Widget.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardUIWidgetBase.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRewardUIImmediateClaimStateTest,
	"P_RD.UI.Reward.ImmediateClaimState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardUIImmediateClaimStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드를 찾았다"), World))
	{
		return false;
	}

	UClass* RewardWidgetClass = LoadClass<URewardUIWidgetBase>(
		nullptr, TEXT("/Game/UI/WBP_Reward.WBP_Reward_C"));
	if (!TestNotNull(TEXT("새 보상 WBP 클래스를 찾았다"), RewardWidgetClass))
	{
		return false;
	}

	URewardUIModel* Model = NewObject<URewardUIModel>(GetTransientPackage());
	URewardUIWidgetBase* Widget = CreateWidget<URewardUIWidgetBase>(World, RewardWidgetClass);
	if (!TestNotNull(TEXT("보상 위젯을 만들었다"), Widget))
	{
		return false;
	}

	FRewardUI Reward;
	Reward.mTitle = FText::FromString(TEXT("VICTORY REWARD"));
	Reward.mGoldGained = 50;
	Reward.mGoldBalance = 170;
	Reward.mExpGained = 30;
	Reward.mLevelBefore = 3;
	Reward.mLevelAfter = 3;
	Reward.mExpAfter = 70.0f;
	Reward.mMaxExp = 100.0f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FRewardMercenaryExpUI& Mercenary =
			Reward.mMercenaryExp.AddDefaulted_GetRef();
		Mercenary.mName = FText::FromString(
			FString::Printf(TEXT("Mercenary %d"), Index + 1));
		Mercenary.mLevel = Index + 1;
		Mercenary.mExpBefore = 10.f * Index;
		Mercenary.mExpAfter = Mercenary.mExpBefore + Reward.mExpGained;
		Mercenary.mMaxExp = 100.f;
	}
	Model->SetReward(Reward);

	FRewardChoiceUI Choice;
	Choice.mChoiceIndex = 7;
	Choice.mKind = ERewardChoiceKind::Equipment;
	Choice.mName = FText::FromString(TEXT("Test Equipment"));
	Model->SetRewardChoices({ Choice });

	Widget->BindUIModel(Model);
	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();

	TestEqual(TEXT("EXP 지급 대상 세 명의 진행도를 따로 유지한다"),
		Model->GetReward().mMercenaryExp.Num(), 3);
	TestNotNull(TEXT("현재 톤 보상 배경을 디자인 캔버스에 생성한다"),
		Widget->GetWidgetFromName(TEXT("RewardBackgroundImage")));
	if (UWidget* LegacyPanel = Widget->GetWidgetFromName(
		TEXT("RewardElem_02_widget_00_widget")))
	{
		TestEqual(TEXT("구형 불투명 보상 판은 새 배경을 가리지 않는다"),
			LegacyPanel->GetVisibility(), ESlateVisibility::Collapsed);
	}

	// 커맨드렛의 EditorWorld에는 GameViewport가 없어 IsOpened()를 검사할 수 없다.
	// Bind 직후 별도 tick/timer 없이 행이 완성되는 것으로 정적 즉시 표시 계약을 검증한다.
	TestEqual(TEXT("골드·EXP·선택 보상이 별도 tick 없이 즉시 모두 그려진다"),
		Widget->GetRewardRowCount(), 3);
	TestEqual(TEXT("처음에는 수령 완료 행이 없다"),
		Widget->GetClaimedRewardRowCount(), 0);
	TestFalse(TEXT("모든 지급이 확정되기 전에는 완료가 아니다"),
		Widget->IsRewardClaimComplete());

	Model->ConfirmRewardClaim(ERewardClaimKind::Gold);
	TestEqual(TEXT("Gold 성공 ACK만 수령 상태로 반영한다"),
		Widget->GetClaimedRewardRowCount(), 1);
	TestEqual(TEXT("수령 후에도 결과 확인을 위해 행은 유지한다"),
		Widget->GetRewardRowCount(), 3);

	Model->ConfirmRewardClaim(ERewardClaimKind::Choice, 99);
	TestEqual(TEXT("존재하지 않는 선택지 ACK는 무시한다"),
		Widget->GetClaimedRewardRowCount(), 1);

	Model->ConfirmRewardClaim(ERewardClaimKind::Exp);
	Model->ConfirmRewardClaim(ERewardClaimKind::Choice, 7);
	TestTrue(TEXT("모든 성공 ACK 뒤 완료 상태가 된다"),
		Widget->IsRewardClaimComplete());
	TestEqual(TEXT("세 행 모두 완료 상태다"),
		Widget->GetClaimedRewardRowCount(), 3);

	return true;
}

#endif
