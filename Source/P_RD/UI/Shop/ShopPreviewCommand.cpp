/*****************************************************************//**
 * @file   ShopPreviewCommand.cpp
 * @brief  실행 중인 상점방에 상점 화면을 붙여 거래를 시험하는 개발용 명령.
 *********************************************************************/

#include "RDMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "GameMode/ShopGameMode.h"
#include "UI/Shop/ShopUIModel.h"
#include "UI/Shop/ShopUIWidgetBase.h"

#if !UE_BUILD_SHIPPING

namespace ShopPreview
{
	const TCHAR* ShopWidgetPath =
		TEXT("/Game/UI/Shop/WBP_Shop.WBP_Shop_C");
	const TCHAR* FullGeneratedShopWidgetPath =
		TEXT("/Game/UI/Shop/WBP_Shop_FullGenerated.WBP_Shop_FullGenerated_C");

	TWeakObjectPtr<UShopUIWidgetBase> ShownWidget;
	TWeakObjectPtr<UShopUIWidgetBase> ShownFullGeneratedWidget;

	/**
	 * @brief 현재 월드가 상점방일 때 게임모드의 실제 뷰모델을 반환
	 * @return 상점방이 아니면 nullptr (사유는 로그로 출력)
	 */
	UShopUIModel* FindLiveModel(UWorld* World)
	{
		if (World == nullptr || World->IsGameWorld() == false)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.Shop*: 게임 월드를 찾지 못했습니다."));
			return nullptr;
		}

		AShopGameMode* GameMode = World->GetAuthGameMode<AShopGameMode>();
		if (GameMode == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.Shop*: 상점방이 아닙니다. 상점방에서 실행하세요."));
			return nullptr;
		}

		UShopUIModel* Model = GameMode->GetShopUIModel();
		if (Model == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.Shop*: 상점 뷰모델이 아직 없습니다."));
		}
		return Model;
	}

	/**
	 * @brief 상점 화면(WBP_Shop)을 열고 실제 뷰모델에 바인딩
	 */
	void Show(UWorld* World)
	{
		// 이미 떠 있으면 닫고 새로 연다
		if (ShownWidget.IsValid())
		{
			ShownWidget->RemoveFromParent();
			ShownWidget.Reset();
		}

		UShopUIModel* Model = FindLiveModel(World);
		if (Model == nullptr)
		{
			return;
		}

		UClass* WidgetClass = LoadClass<UShopUIWidgetBase>(nullptr, ShopWidgetPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.ShopPreview: %s 클래스를 찾지 못했습니다."), ShopWidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		UShopUIWidgetBase* Widget = Controller != nullptr
			? CreateWidget<UShopUIWidgetBase>(Controller, WidgetClass)
			: CreateWidget<UShopUIWidgetBase>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.ShopPreview: 위젯 생성에 실패했습니다."));
			return;
		}

		Widget->BindUIModel(Model);
		Widget->OpenUI();
		ShownWidget = Widget;

		UE_LOG(LogRD, Display, TEXT("RD.ShopPreview: 상점 화면을 열었습니다."));
	}

	FAutoConsoleCommandWithWorld ShowCommand(
		TEXT("RD.ShopPreview"),
		TEXT("현재 상점방의 실제 뷰모델에 상점 화면(WBP_Shop)을 붙여 연다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Show));

	/** @brief 실제 HUD를 교체하지 않고 생성 이미지 전용 상점 WBP를 시험한다. */
	void ShowFullGenerated(UWorld* World)
	{
		if (ShownFullGeneratedWidget.IsValid())
		{
			ShownFullGeneratedWidget->RemoveFromParent();
			ShownFullGeneratedWidget.Reset();
		}

		UShopUIModel* Model = FindLiveModel(World);
		if (Model == nullptr)
		{
			return;
		}

		UClass* WidgetClass = LoadClass<UShopUIWidgetBase>(
			nullptr, FullGeneratedShopWidgetPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.ShopPreviewFullGenerated: %s 클래스를 찾지 못했습니다."),
				FullGeneratedShopWidgetPath);
			return;
		}

		APlayerController* Controller = World->GetFirstPlayerController();
		UShopUIWidgetBase* Widget = Controller != nullptr
			? CreateWidget<UShopUIWidgetBase>(Controller, WidgetClass)
			: CreateWidget<UShopUIWidgetBase>(World, WidgetClass);
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Warning,
				TEXT("RD.ShopPreviewFullGenerated: 위젯 생성에 실패했습니다."));
			return;
		}

		Widget->BindUIModel(Model);
		Widget->OpenUI();
		ShownFullGeneratedWidget = Widget;
		UE_LOG(LogRD, Display,
			TEXT("RD.ShopPreviewFullGenerated: 생성 이미지 전용 상점을 열었습니다."));
	}

	FAutoConsoleCommandWithWorld ShowFullGeneratedCommand(
		TEXT("RD.ShopPreviewFullGenerated"),
		TEXT("현재 상점방 모델로 WBP_Shop_FullGenerated를 프리뷰한다."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ShowFullGenerated));

	/**
	 * @brief 콘솔 인자에서 정수 하나를 읽음
	 * @return 해당 위치에 인자가 없으면 false
	 */
	bool GetIntArg(const TArray<FString>& Args, int32 ArgIndex, OUT int32& OutValue)
	{
		if (Args.IsValidIndex(ArgIndex) == false)
		{
			return false;
		}
		OutValue = FCString::Atoi(*Args[ArgIndex]);
		return true;
	}

	/**
	 * @brief 판매 슬롯 구매 (아티펙트 전용 의도) -- 클릭 대신 콘솔로 RequestBuy 호출
	 */
	void Buy(const TArray<FString>& Args, UWorld* World)
	{
		int32 SlotIndex = 0;
		if (GetIntArg(Args, 0, SlotIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.ShopBuy <슬롯번호>"));
			return;
		}
		if (UShopUIModel* Model = FindLiveModel(World))
		{
			Model->RequestBuy(SlotIndex);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs BuyCommand(
		TEXT("RD.ShopBuy"),
		TEXT("판매 슬롯을 산다(아티펙트). 사용법: RD.ShopBuy <슬롯번호>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Buy));

	/**
	 * @brief 스킬 구매 -- 지급 대상 유닛+스킬 슬롯까지 지정해 RequestBuySkill 호출
	 */
	void BuySkill(const TArray<FString>& Args, UWorld* World)
	{
		int32 SlotIndex = 0;
		int32 UnitIndex = 0;
		int32 SkillSlotIndex = 0;
		if (GetIntArg(Args, 0, SlotIndex) == false || GetIntArg(Args, 1, UnitIndex) == false
			|| GetIntArg(Args, 2, SkillSlotIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.ShopBuySkill <슬롯번호> <유닛번호> <스킬슬롯번호>"));
			return;
		}
		if (UShopUIModel* Model = FindLiveModel(World))
		{
			Model->RequestBuySkill(SlotIndex, UnitIndex, SkillSlotIndex);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs BuySkillCommand(
		TEXT("RD.ShopBuySkill"),
		TEXT("판매 슬롯의 스킬을 지정 유닛의 지정 슬롯에 사준다(찬 슬롯이면 교체). 사용법: RD.ShopBuySkill <슬롯번호> <유닛번호> <스킬슬롯번호>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BuySkill));

	/**
	 * @brief 소지 아티펙트 버리기 -- RequestDiscardArtifact 호출
	 */
	void DiscardArtifact(const TArray<FString>& Args, UWorld* World)
	{
		int32 ArtifactIndex = 0;
		if (GetIntArg(Args, 0, ArtifactIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.ShopDiscardArtifact <아티펙트번호>"));
			return;
		}
		if (UShopUIModel* Model = FindLiveModel(World))
		{
			Model->RequestDiscardArtifact(ArtifactIndex);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs DiscardArtifactCommand(
		TEXT("RD.ShopDiscardArtifact"),
		TEXT("파티 소지 아티펙트를 버린다. 사용법: RD.ShopDiscardArtifact <아티펙트번호>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DiscardArtifact));

	/**
	 * @brief 유닛 스킬 슬롯 버리기 -- RequestDiscardSkill 호출
	 */
	void DiscardSkill(const TArray<FString>& Args, UWorld* World)
	{
		int32 UnitIndex = 0;
		int32 SlotIndex = 0;
		if (GetIntArg(Args, 0, UnitIndex) == false || GetIntArg(Args, 1, SlotIndex) == false)
		{
			UE_LOG(LogRD, Warning, TEXT("사용법: RD.ShopDiscardSkill <유닛번호> <스킬슬롯번호>"));
			return;
		}
		if (UShopUIModel* Model = FindLiveModel(World))
		{
			Model->RequestDiscardSkill(UnitIndex, SlotIndex);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs DiscardSkillCommand(
		TEXT("RD.ShopDiscardSkill"),
		TEXT("유닛의 스킬 슬롯을 비운다. 사용법: RD.ShopDiscardSkill <유닛번호> <스킬슬롯번호>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DiscardSkill));

	/**
	 * @brief 골드 치트 -- 구매 테스트용 파티 골드 지급
	 */
	void AddGold(const TArray<FString>& Args, UWorld* World)
	{
		// 액수 생략 시 10000골드
		int32 Amount = 10000;
		GetIntArg(Args, 0, Amount);

		AShopGameMode* GameMode = (World != nullptr && World->IsGameWorld() == true)
			? World->GetAuthGameMode<AShopGameMode>() : nullptr;
		if (GameMode == nullptr)
		{
			UE_LOG(LogRD, Warning, TEXT("RD.ShopAddGold: 상점방이 아닙니다. 상점방에서 실행하세요."));
			return;
		}
		GameMode->AddPartyGoldDev(Amount);
	}

	FAutoConsoleCommandWithWorldAndArgs AddGoldCommand(
		TEXT("RD.ShopAddGold"),
		TEXT("파티 골드를 지급한다(치트). 사용법: RD.ShopAddGold [액수, 생략 시 10000]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AddGold));
}

#endif
