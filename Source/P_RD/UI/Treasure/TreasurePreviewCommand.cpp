/*****************************************************************//**
 * @file   TreasurePreviewCommand.cpp
 * @brief  실행 중인 보물방에 보물방 화면을 붙여 개봉을 시험하는 개발용 명령
 * @author 이문환
 * @date   2026-08-05
 *********************************************************************/

#include "RDMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "GameMode/TreasureGameMode.h"
#include "UI/Treasure/TreasureUIModel.h"
#include "UI/Treasure/TreasureUIWidgetBase.h"

#if !UE_BUILD_SHIPPING

namespace TreasurePreview
{
	const TCHAR* TreasureWidgetPath =
		TEXT("/Game/UI/Treasure/WBP_Treasure.WBP_Treasure_C");

	TWeakObjectPtr<UTreasureUIWidgetBase> ShownWidget;

	/**
	 * @brief 현재 월드가 보물방일 때 게임모드의 실제 뷰모델을 반환
	 * @return 보물방이 아니면 nullptr (사유는 로그로 출력)
	 */
	UTreasureUIModel* FindLiveModel(UWorld* World)
	{
		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.Treasure*: 게임 월드를 찾지 못했습니다."));
			return nullptr;
		}

		ATreasureGameMode* GameMode = World->GetAuthGameMode<ATreasureGameMode>();
		if (GameMode == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.Treasure*: 보물방이 아닙니다. 보물방에서 실행하세요."));
			return nullptr;
		}

		UTreasureUIModel* Model = GameMode->GetTreasureUIModel();
		if (Model == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.Treasure*: 보물방 뷰모델이 아직 없습니다."));
		}
		return Model;
	}

	/**
	 * @brief 보물방 화면(WBP_Treasure)을 열고 실제 뷰모델에 바인딩
	 */
	void Show(UWorld* World)
	{
		// 이미 떠 있으면 닫고 새로 연다
		if (ShownWidget.IsValid())
		{
			ShownWidget->RemoveFromParent();
			ShownWidget.Reset();
		}

		UTreasureUIModel* Model = FindLiveModel(World);
		if (Model == nullptr)
		{
			return;
		}

		UClass* WidgetClass = LoadClass<UTreasureUIWidgetBase>(nullptr, TreasureWidgetPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.TreasurePreview: %s 클래스를 찾지 못했습니다."), TreasureWidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		UTreasureUIWidgetBase* Widget = Controller != nullptr
			? CreateWidget<UTreasureUIWidgetBase>(Controller, WidgetClass)
			: CreateWidget<UTreasureUIWidgetBase>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.TreasurePreview: 위젯 생성에 실패했습니다."));
			return;
		}

		Widget->BindUIModel(Model);
		Widget->OpenUI();
		ShownWidget = Widget;

		UE_LOG(LogRD, Display, TEXT("RD.TreasurePreview: 보물방 화면을 열었습니다."));
	}

	FAutoConsoleCommandWithWorld ShowCommand(
		TEXT("RD.TreasurePreview"),
		TEXT("현재 보물방의 실제 뷰모델에 보물방 화면(WBP_Treasure)을 붙여 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Show));

	/**
	 * @brief 상자 개봉 -- 클릭 대신 콘솔로 RequestOpen 호출
	 */
	void Open(UWorld* World)
	{
		if (UTreasureUIModel* Model = FindLiveModel(World))
		{
			Model->RequestOpen();
		}
	}

	FAutoConsoleCommandWithWorld OpenCommand(
		TEXT("RD.TreasureOpen"),
		TEXT("보물상자를 연다. 보상 지급 결과는 로그와 화면으로 확인한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Open));
}

#endif
