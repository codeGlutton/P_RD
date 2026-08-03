#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/FrontendMapWidget.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatWorldMapMenuLifecycleTest,
	"P_RD.UI.CombatHUD.WorldMapMenuLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 전투 MAP 단추가 조회용 지도를 실제로 열고 닫는지 확인한다.
 *
 * 지도 WBP의 스크롤 구조만 검사하면 버튼 델리게이트가 빠져도 통과한다.
 * 이 시험은 전투 HUD 인스턴스의 실제 버튼을 방송해 지도 표시, 방 선택 잠금,
 * HUD 입력 차단과 닫은 뒤 복원까지 한 흐름으로 확인한다.
 */
bool FCombatWorldMapMenuLifecycleTest::RunTest(const FString& Parameters)
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
		AddError(TEXT("전투 지도 메뉴를 시험할 월드가 없다."));
		return false;
	}

	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	UCombatLayoutHUDWidget* HUD = HUDClass != nullptr
		? CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass) : nullptr;
	if (!TestNotNull(TEXT("전투 HUD"), HUD))
	{
		return false;
	}
	TSharedPtr<SWidget> HUDSlate = HUD->TakeWidget();
	if (!TestTrue(TEXT("전투 HUD Slate 생명주기 유지"), HUDSlate.IsValid()))
	{
		return false;
	}

	UButton* MapButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MenuButton_0")));
	if (!TestNotNull(TEXT("MAP 메뉴 단추"), MapButton))
	{
		return false;
	}
	TestTrue(TEXT("MAP 메뉴에 열기 동작이 묶여 있다"),
		MapButton->OnClicked.IsBound());

	UWorldWidgetSubsystem* WidgetSubsystem =
		World->GetSubsystem<UWorldWidgetSubsystem>();
	if (!TestNotNull(TEXT("월드 위젯 서브시스템"), WidgetSubsystem))
	{
		return false;
	}

	// 일반 방에서는 GameMode가 먼저 만든다. 에디터 단독 시험에서는 그 초기화가
	// 없을 수 있으므로 런타임과 같은 설정 인덱스로 준비한다.
	WidgetSubsystem->InitWorldWidget(EWorldWidgetType::WorldMap);
	UFrontendMapWidget* Map = WidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
		EWorldWidgetType::WorldMap);

	HUD->OpenUI();
	MapButton->OnClicked.Broadcast();

	// 헤드리스 월드에서는 서브시스템 초기화가 OwningPlayer 부재로 임시
	// 인스턴스를 둘 만들 수 있다. HUD가 닫기 델리게이트를 실제로 묶은
	// 보이는 인스턴스를 정본으로 다시 찾는다.
	UFrontendMapWidget* OpenedMap = nullptr;
	for (TObjectIterator<UFrontendMapWidget> It; It; ++It)
	{
		if (It->GetWorld() == World
			&& It->GetVisibility() == ESlateVisibility::Visible
			&& It->OnCloseRequested.IsBound())
		{
			OpenedMap = *It;
			break;
		}
	}
	if (OpenedMap != nullptr)
	{
		Map = OpenedMap;
	}
	if (!TestNotNull(TEXT("MAP 메뉴가 연 월드맵"), Map))
	{
		return false;
	}
	TSharedPtr<SWidget> MapSlate = Map->TakeWidget();
	if (!TestTrue(TEXT("월드맵 Slate 생명주기 유지"), MapSlate.IsValid()))
	{
		return false;
	}
	TestEqual(TEXT("MAP을 누르면 지도가 보인다"),
		Map->GetVisibility(), ESlateVisibility::Visible);
	TestFalse(TEXT("전투 중 지도는 방 선택 불가"),
		Map->IsRoomSelectionEnabled());
	TestEqual(TEXT("지도 뒤 전투 HUD는 입력을 받지 않는다"),
		HUD->GetVisibility(), ESlateVisibility::Collapsed);

	UButton* CloseButton = Cast<UButton>(
		Map->WidgetTree->FindWidget(TEXT("CloseButton")));
	if (CloseButton == nullptr)
	{
		CloseButton = Cast<UButton>(
			Map->WidgetTree->FindWidget(TEXT("RuntimeCloseButton")));
	}
	if (CloseButton == nullptr)
	{
		TArray<UWidget*> Widgets;
		Map->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UButton* Candidate = Cast<UButton>(Widget);
				Candidate != nullptr
					&& Candidate->GetName().Contains(TEXT("CloseButton")))
			{
				CloseButton = Candidate;
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("지도 닫기 단추"), CloseButton))
	{
		return false;
	}
	TestTrue(TEXT("지도 닫기 단추에 동작이 묶여 있다"),
		CloseButton->OnClicked.IsBound());
	CloseButton->OnClicked.Broadcast();
	TestEqual(TEXT("닫기 요청 뒤 지도는 접힌다"),
		Map->GetVisibility(), ESlateVisibility::Collapsed);
	// HUD 는 화면 전체로 눌림을 받아 판 탭(카드 여닫기·적 살펴보기)을
	// 처리하므로 Visible 로 돌아와야 한다(OpenUI 주석 참고).
	// SelfHitTestInvisible 로 돌리면 지도를 닫은 뒤 빈 땅 탭이 전부 죽는다.
	TestEqual(TEXT("닫기 뒤 전투 HUD 입력이 복원된다"),
		HUD->GetVisibility(), ESlateVisibility::Visible);
	return true;
}

#endif
