/*****************************************************************//**
 * @file   CombatLayoutPreviewCommands.cpp
 * @brief  배치안 10개를 게임 안에서 바로 바꿔 보는 콘솔 명령.
 * @details
 * 배치안은 열 개를 나란히 놓고 고르는 것이라, 하나 보려고 게임을 껐다 켜면
 * 비교가 안 된다. 명령 한 줄로 즉시 갈아 끼운다.
 *
 * 전투 밖에서도 동작한다. UCombatLayoutHUDWidget은 전투 게임모드가 있으면
 * 실제 뷰모델에 붙고, 없으면 스스로 가짜 전투 장면을 세운다. 그래서 타이틀
 * 화면에서도 열 개를 다 넘겨 볼 수 있고, 전투까지 진행하지 않아도 된다.
 *
 * 배포 빌드에는 들어가지 않는다.
 * @author 박용수
 * @date   2026-07-26
 *********************************************************************/

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"

#if !UE_BUILD_SHIPPING

namespace CombatLayoutPreview
{
	/** @brief 넘겨 볼 배치안. 순서가 곧 명령의 번호다. */
	const TCHAR* LayoutPaths[] = {
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_01_ClassicCRPG.WBP_CombatLayout_01_ClassicCRPG_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_02_LeftParty.WBP_CombatLayout_02_LeftParty_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_03_ActiveUnit.WBP_CombatLayout_03_ActiveUnit_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_04_Radial.WBP_CombatLayout_04_Radial_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_05_BottomBar.WBP_CombatLayout_05_BottomBar_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_06_Mirrored.WBP_CombatLayout_06_Mirrored_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_07_CardHand.WBP_CombatLayout_07_CardHand_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_08_Minimal.WBP_CombatLayout_08_Minimal_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_09_SplitBands.WBP_CombatLayout_09_SplitBands_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_10_Targeting.WBP_CombatLayout_10_Targeting_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_11_RightGrid.WBP_CombatLayout_11_RightGrid_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_12_TurnQueue.WBP_CombatLayout_12_TurnQueue_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_13_RightList.WBP_CombatLayout_13_RightList_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_14_FloatingBar.WBP_CombatLayout_14_FloatingBar_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_15_UnifiedDock.WBP_CombatLayout_15_UnifiedDock_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_16_FullFrame.WBP_CombatLayout_16_FullFrame_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_17_RightDock.WBP_CombatLayout_17_RightDock_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_18_RightFan.WBP_CombatLayout_18_RightFan_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_19_TopRail.WBP_CombatLayout_19_TopRail_C"),
		TEXT("/Game/UI/CombatLayouts/WBP_CombatLayout_20_CommandMode.WBP_CombatLayout_20_CommandMode_C"),
	};

	/** @brief 이름표. 화면 어느 안인지 헷갈리지 않게 로그로 남긴다. */
	const TCHAR* LayoutNames[] = {
		TEXT("1안 클래식 CRPG"), TEXT("2안 좌측 세로 파티"),
		TEXT("3안 활성 유닛 집중"), TEXT("4안 방사형"),
		TEXT("5안 하단 통합 바"), TEXT("6안 좌우 대칭"),
		TEXT("7안 카드 핸드"), TEXT("8안 미니멀"),
		TEXT("9안 정보·조작 분리"), TEXT("10안 상황 전환형"),
		TEXT("11안 우측 스킬 격자"), TEXT("12안 좌측 턴 큐"),
		TEXT("13안 우측 세로 목록"), TEXT("14안 유닛 위 스킬 바"),
		TEXT("15안 하단 통합 독"), TEXT("16안 전체 액자"),
		TEXT("17안 우측 통합 독"), TEXT("18안 우측 부채"),
		TEXT("19안 상단 스킬 레일"), TEXT("20안 지휘 모드"),
	};

	static_assert(UE_ARRAY_COUNT(LayoutPaths) == UE_ARRAY_COUNT(LayoutNames),
		"layout paths and labels must stay in step");

	/** @brief 지금 띄워 둔 미리보기. 다음 안으로 넘어갈 때 걷어낸다. */
	TWeakObjectPtr<UCombatLayoutHUDWidget> Shown;

	/** @brief 마지막으로 띄운 번호. Next가 이어서 넘긴다. */
	int32 ShownIndex = INDEX_NONE;

	/** @brief 미리보기 때문에 감춘 위젯과, 감추기 전의 표시 상태. */
	struct FHiddenWidget
	{
		TWeakObjectPtr<UUserWidget> Widget;
		ESlateVisibility Restore = ESlateVisibility::Visible;
	};
	TArray<FHiddenWidget> Hidden;

	/**
	 * @brief 뷰포트에 올라와 있는 기존 UI를 전부 감춘다.
	 *
	 * @details
	 * 비주얼을 보려는 것이므로 예전 HUD가 뒤에 겹쳐 보이면 판단이 안 된다.
	 * 지우지 않고 표시만 끈다 -- 게임모드가 만든 HUD를 지우면 다시 만들 수
	 * 없어서 미리보기를 꺼도 원래 화면으로 못 돌아온다.
	 */
	void HideExistingUI(UWorld* World, UUserWidget* Keep)
	{
		TArray<UUserWidget*> Found;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
			World, Found, UUserWidget::StaticClass(), false);
		for (UUserWidget* Candidate : Found)
		{
			if (Candidate == nullptr || Candidate == Keep
				|| !Candidate->IsInViewport())
			{
				continue;
			}
			Hidden.Add({ Candidate, Candidate->GetVisibility() });
			Candidate->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	/** @brief 감췄던 것을 원래 표시 상태로 되돌린다. */
	void RestoreExistingUI()
	{
		for (const FHiddenWidget& Entry : Hidden)
		{
			if (Entry.Widget.IsValid())
			{
				Entry.Widget->SetVisibility(Entry.Restore);
			}
		}
		Hidden.Reset();
	}

	/**
	 * @brief 배치안 하나를 화면에 올린다.
	 *
	 * @details
	 * 기존 HUD는 건드리지 않고 그 위에 덮는다. 게임모드가 만든 HUD를 걷어내면
	 * 다시 만들 방법이 없어서, 미리보기를 끄면 원래 화면으로 돌아오지 못한다.
	 */
	void Show(UWorld* World, const int32 Index)
	{
		if (World == nullptr)
		{
			return;
		}
		if (!LayoutPaths[0] || Index < 0 || Index >= UE_ARRAY_COUNT(LayoutPaths))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Layout] 1~%d 사이로 넣어 주세요"),
				int32(UE_ARRAY_COUNT(LayoutPaths)));
			return;
		}

		if (Shown.IsValid())
		{
			Shown->RemoveFromParent();
			Shown.Reset();
		}

		UClass* LayoutClass =
			LoadClass<UCombatLayoutHUDWidget>(nullptr, LayoutPaths[Index]);
		if (LayoutClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Layout] 못 찾음: %s"), LayoutPaths[Index]);
			return;
		}

		// CreateWidget는 소유자 타입을 템플릿으로 고른다. UObject*로 뭉뚱그리면
		// 지원 타입이 아니라고 컴파일이 막힌다.
		APlayerController* Controller = World->GetFirstPlayerController();
		UCombatLayoutHUDWidget* Layout =
			Controller != nullptr
				? CreateWidget<UCombatLayoutHUDWidget>(Controller, LayoutClass)
				: CreateWidget<UCombatLayoutHUDWidget>(World, LayoutClass);
		if (Layout == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Layout] 위젯 생성 실패"));
			return;
		}

		// 기존 UI를 먼저 감춘다. 위에 얹기만 하면 예전 HUD가 비쳐서 비주얼
		// 판단이 안 된다. 감춘 목록은 RD.Layout 0으로 되돌린다.
		HideExistingUI(World, Layout);

		// URDUserWidget은 OpenUI()가 AddToViewport와 표시 상태를 함께 처리한다.
		Layout->OpenUI(FOnEndUIOpenAnimation());
		Shown = Layout;
		ShownIndex = Index;
		UE_LOG(LogTemp, Display, TEXT("[Layout] %s"), LayoutNames[Index]);
	}

	void Hide()
	{
		if (Shown.IsValid())
		{
			Shown->RemoveFromParent();
			Shown.Reset();
		}
		RestoreExistingUI();
		ShownIndex = INDEX_NONE;
	}
}

static FAutoConsoleCommandWithWorldAndArgs GCombatLayoutShow(
	TEXT("RD.Layout"),
	TEXT("전투 HUD 배치안을 띄운다. 예: RD.Layout 3 (1~10). 0이면 끈다."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				UE_LOG(LogTemp, Display,
					TEXT("[Layout] 지금: %s"),
					CombatLayoutPreview::ShownIndex == INDEX_NONE
						? TEXT("없음")
						: CombatLayoutPreview::LayoutNames[CombatLayoutPreview::ShownIndex]);
				return;
			}
			const int32 Number = FCString::Atoi(*Args[0]);
			if (Number == 0)
			{
				CombatLayoutPreview::Hide();
				return;
			}
			CombatLayoutPreview::Show(World, Number - 1);
		}));

static FAutoConsoleCommandWithWorld GCombatLayoutNext(
	TEXT("RD.LayoutNext"),
	TEXT("다음 배치안으로 넘긴다. 열 개를 이어서 넘겨 보기 위한 것."),
	FConsoleCommandWithWorldDelegate::CreateStatic(
		[](UWorld* World)
		{
			const int32 Count = UE_ARRAY_COUNT(CombatLayoutPreview::LayoutPaths);
			CombatLayoutPreview::Show(
				World, (CombatLayoutPreview::ShownIndex + 1) % Count);
		}));

#endif // !UE_BUILD_SHIPPING
