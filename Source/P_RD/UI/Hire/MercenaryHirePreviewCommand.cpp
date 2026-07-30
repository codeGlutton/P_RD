/*****************************************************************//**
 * @file   MercenaryHirePreviewCommand.cpp
 * @brief  고용 게시판을 아무 화면에서나 띄워 보는 콘솔 명령.
 * @details
 * 전투 밖에서도 열려야 한다. 화면을 고칠 때마다 게임을 처음부터 돌려
 * 고용 단계까지 가야 한다면 아무도 안 고친다.
 *
 * 프론트엔드에서 부르면 실제 후보를 걸어 준다. 다른 데서 부르면 넘길 것이
 * 없어 WBP 에 구워 둔 시안 글자가 그대로 보인다 -- 고르는 규칙은 어느 쪽이든
 * 똑같이 돈다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#include "RDMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/Hire/MercenaryHireWidget.h"

#if !UE_BUILD_SHIPPING

namespace MercenaryHirePreview
{
	const TCHAR* WidgetPath =
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire.WBP_MercenaryHire_C");

	TWeakObjectPtr<UMercenaryHireWidget> Shown;

	UWorld* PickWorld()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game
				|| Context.WorldType == EWorldType::PIE)
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void Toggle(const TArray<FString>& Args)
	{
		if (Shown.IsValid())
		{
			Shown->RemoveFromParent();
			Shown.Reset();
			if (Args.Num() == 0)
			{
				UE_LOG(LogTemp, Display, TEXT("[고용] 닫음"));
				return;
			}
		}

		UWorld* World = PickWorld();
		if (World == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[고용] 월드를 못 찾음"));
			return;
		}

		UClass* WidgetClass = LoadClass<UMercenaryHireWidget>(nullptr, WidgetPath);
		if (WidgetClass == nullptr)
		{
			// 부모 클래스를 아직 안 바꿨으면 여기서 걸린다. 조용히 아무 일도
			// 안 일어나면 원인을 찾는 데 한참 걸리므로 이유를 남긴다.
			UE_LOG(LogTemp, Warning,
				TEXT("[고용] %s 를 UMercenaryHireWidget 으로 못 읽음. "
					 "WBP 의 부모 클래스를 확인하라."), WidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		UMercenaryHireWidget* Widget = Controller != nullptr
			? CreateWidget<UMercenaryHireWidget>(Controller, WidgetClass)
			: CreateWidget<UMercenaryHireWidget>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[고용] 위젯 생성 실패"));
			return;
		}

		// 프론트엔드에 있으면 실제 후보를 건다. 없으면 시안 글자가 남는다.
		if (const AFrontendGameMode* Frontend =
			Cast<AFrontendGameMode>(World->GetAuthGameMode()))
		{
			TArray<FFrontendCharacterOption> Options;
			if (Frontend->GetCharacterOptions(Options))
			{
				Widget->SetCharacterOptions(Options);
				UE_LOG(LogTemp, Display, TEXT("[고용] 후보 %d명 걸음"),
					Options.Num());
			}
		}

		Widget->AddToViewport(5000);
		Shown = Widget;
		UE_LOG(LogTemp, Display, TEXT("[고용] 띄움"));
	}

	FAutoConsoleCommand ToggleCommand(
		TEXT("RD.Hire"),
		TEXT("용병 고용 게시판을 띄우거나 닫는다."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Toggle));
}

#endif
