#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/CombatResultOverlayWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr TCHAR DefeatWidgetPath[] =
		TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C");

	UWorld* FindAutomationWorld()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() != nullptr)
			{
				return Context.World();
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDefeatWBPStructureTest,
	"P_RD.UI.CombatDefeat.Structure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDefeatWBPStructureTest::RunTest(const FString& Parameters)
{
	UClass* WidgetClass = LoadClass<UCombatResultOverlayWidget>(nullptr, DefeatWidgetPath);
	if (!TestNotNull(TEXT("패배 결과 WBP 클래스"), WidgetClass))
	{
		return false;
	}
	TestTrue(TEXT("패배 WBP는 결과 오버레이 C++ 클래스를 상속한다"),
		WidgetClass->IsChildOf(UCombatResultOverlayWidget::StaticClass()));

	UWidgetBlueprintGeneratedClass* Generated = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	UWidgetTree* Tree = Generated != nullptr ? Generated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("패배 WBP 위젯 트리"), Tree))
	{
		return false;
	}

	TestNotNull(TEXT("모바일 해상도 대응 ScaleBox"),
		Cast<UScaleBox>(Tree->FindWidget(TEXT("DefeatResponsiveScale"))));
	TestNotNull(TEXT("타이틀 이동 버튼"),
		Cast<UButton>(Tree->FindWidget(TEXT("mTitleButton"))));
	TestNotNull(TEXT("재도전 버튼"),
		Cast<UButton>(Tree->FindWidget(TEXT("mRetryButton"))));
	for (const TCHAR* Name : {
		TEXT("mLocationText"), TEXT("mRoundText"), TEXT("mEnemyText"),
		TEXT("mGoldText"), TEXT("mExpText") })
	{
		TestNotNull(Name, Cast<UTextBlock>(Tree->FindWidget(FName(Name))));
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FString Name = FString::Printf(TEXT("mPartyPortrait%d"), Index);
		TestNotNull(*Name, Cast<UImage>(Tree->FindWidget(FName(*Name))));
	}

	for (const TCHAR* AssetName : {
		TEXT("T_MB_Defeat_OuterFrame"), TEXT("T_MB_Defeat_TitleBanner"),
		TEXT("T_MB_Defeat_MercenaryCard"),
		TEXT("T_MB_Defeat_ButtonSecondary"), TEXT("T_MB_Defeat_ButtonPrimary") })
	{
		const FString Path = FString::Printf(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Defeat/%s.%s"), AssetName, AssetName);
		TestNotNull(*Path, LoadObject<UTexture2D>(nullptr, *Path));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDefeatWBPInteractionTest,
	"P_RD.UI.CombatDefeat.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDefeatWBPInteractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = FindAutomationWorld();
	if (World == nullptr)
	{
		AddInfo(TEXT("위젯 생성 월드가 없어 런타임 상호작용 검사를 건너뜀"));
		return true;
	}

	UClass* WidgetClass = LoadClass<UCombatResultOverlayWidget>(nullptr, DefeatWidgetPath);
	UCombatResultOverlayWidget* Widget = WidgetClass != nullptr
		? CreateWidget<UCombatResultOverlayWidget>(World, WidgetClass) : nullptr;
	if (!TestNotNull(TEXT("패배 결과 WBP 인스턴스"), Widget))
	{
		return false;
	}

	int32 TitleClicks = 0;
	int32 RetryClicks = 0;
	FCombatResultUI Result;
	Result.mLocationName = FText::FromString(TEXT("잊힌 성채"));
	Result.mRound = 7;
	Result.mDefeatedMonsterCount = 12;
	Widget->ShowDefeatResult(Result,
		FSimpleDelegate::CreateLambda([&TitleClicks]() { ++TitleClicks; }),
		FSimpleDelegate::CreateLambda([&RetryClicks]() { ++RetryClicks; }));
	Widget->OpenUI();
	Widget->TakeWidget();
	// NullRHI 에디터 월드에서는 TakeWidget 뒤에 디자이너 트리가 확정된다.
	// 실제 HUD의 OpenRequested와 같은 시점으로 데이터를 한 번 더 밀어 넣는다.
	Widget->ShowDefeatResult(Result,
		FSimpleDelegate::CreateLambda([&TitleClicks]() { ++TitleClicks; }),
		FSimpleDelegate::CreateLambda([&RetryClicks]() { ++RetryClicks; }));

	UButton* TitleButton = Cast<UButton>(Widget->WidgetTree->FindWidget(TEXT("mTitleButton")));
	UButton* RetryButton = Cast<UButton>(Widget->WidgetTree->FindWidget(TEXT("mRetryButton")));
	if (!TestNotNull(TEXT("타이틀 버튼 인스턴스"), TitleButton)
		|| !TestNotNull(TEXT("재도전 버튼 인스턴스"), RetryButton))
	{
		return false;
	}
	TestTrue(TEXT("타이틀 버튼 클릭 동작 연결"), TitleButton->OnClicked.IsBound());
	TestTrue(TEXT("재도전 버튼 클릭 동작 연결"), RetryButton->OnClicked.IsBound());
	TitleButton->OnClicked.Broadcast();
	TestEqual(TEXT("타이틀 콜백은 한 번 실행"), TitleClicks, 1);
	TestEqual(TEXT("타이틀 선택 시 재도전 콜백은 실행되지 않음"), RetryClicks, 0);
	Widget->ShowDefeatResult(Result,
		FSimpleDelegate::CreateLambda([&TitleClicks]() { ++TitleClicks; }),
		FSimpleDelegate::CreateLambda([&RetryClicks]() { ++RetryClicks; }));
	RetryButton->OnClicked.Broadcast();
	TestEqual(TEXT("재도전 콜백은 한 번 실행"), RetryClicks, 1);
	TestEqual(TEXT("재도전 선택 시 타이틀 콜백은 추가 실행되지 않음"), TitleClicks, 1);

	UTextBlock* RoundText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("mRoundText")));
	UTextBlock* EnemyText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("mEnemyText")));
	if (TestNotNull(TEXT("라운드 텍스트 인스턴스"), RoundText))
	{
		TestTrue(TEXT("7라운드 값 반영"), RoundText->GetText().ToString().Contains(TEXT("7")));
	}
	if (TestNotNull(TEXT("몬스터 텍스트 인스턴스"), EnemyText))
	{
		TestTrue(TEXT("처치 수 12 반영"), EnemyText->GetText().ToString().Contains(TEXT("12")));
	}
	Widget->CloseUI();
	return true;
}

#endif
