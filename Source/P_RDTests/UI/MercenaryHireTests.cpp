/*****************************************************************//**
 * @file   MercenaryHireTests.cpp
 * @brief  용병 선택 게시판의 규칙이 실제로 도는지.
 * @details
 * 화면을 띄워 눈으로 보는 것만으로는 "세 명 채운 뒤 네 번째를 누를 때"나
 * "골랐다가 다시 눌러 무를 때" 같은 자리를 못 훑는다. 규칙만 떼어 시험한다.
 *
 * 위젯을 실제로 만들되 WBP 는 쓰지 않는다. 이름으로 찾는 위젯이 하나도 안
 * 잡히지만 값을 넣는 자리는 전부 널을 걸러내므로, 규칙은 그대로 돈다.
 *
 * 앞선 판은 규칙을 시험 안에 그대로 다시 적어 놓고 그것을 검사했다. 코드가
 * 틀려도 통과하는 시험이었다. 여기서는 위젯이 진짜로 세는 것을 본다.
 * @author 박용수
 * @date   2026-07-27
 *********************************************************************/

#include "Misc/AutomationTest.h"

#include "Frontend/CharacterSelectTypes.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Setting/GamePlaySettings.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Hire/MercenaryHireWidget.h"
#include "UI/MercenaryHireTestsHelper.h"

void UMercenaryDetailTestResponder::Bind(UCombatUIModel* UIModel)
{
	mUIModel = UIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnCombatCommand.AddUniqueDynamic(
			this, &UMercenaryDetailTestResponder::HandleCombatCommand);
	}
}

void UMercenaryDetailTestResponder::HandleCombatCommand(
	const ECombatInputType Type, const int32 IntPayload)
{
	mLastType = Type;
	mLastPayload = IntPayload;
	if (Type != ECombatInputType::InspectUnit || mUIModel == nullptr)
	{
		return;
	}
	++mInspectRequestCount;

	FUnitDetailUI Detail;
	Detail.mUnitId = IntPayload;
	Detail.mLevel = 7;
	for (const FUnitUI& Unit : mUIModel->GetUnitUIs())
	{
		if (Unit.mUnitId == IntPayload)
		{
			Detail.mName = Unit.mName;
			Detail.mPortrait = Unit.mPortrait;
			break;
		}
	}
	Detail.mPassiveDescriptions.Add(FText::FromString(TEXT("용병 상세 왕복 확인")));
	mUIModel->SetUnitDetail(Detail);
}

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** @brief 고를 수 있는 후보 한 명. */
	FFrontendCharacterOption MakeOption(const int32 Index, const TCHAR* Name,
		const bool bSelectable)
	{
		FFrontendCharacterOption Option;
		Option.mIndex = Index;
		Option.mDisplayName = FText::FromString(Name);
		Option.mMaxHP = 100;
		Option.mSelectable = bSelectable;
		Option.mPlayerUnitId = FPrimaryAssetId(TEXT("PlayerUnit"), FName(Name));
		return Option;
	}

	/** @brief 여섯 명 걸린 게시판. WBP 없이 규칙만 돈다. */
	UMercenaryHireWidget* MakeBoard(const int32 LockedIndex = INDEX_NONE)
	{
		UMercenaryHireWidget* Board =
			NewObject<UMercenaryHireWidget>(GetTransientPackage());
		TArray<FFrontendCharacterOption> Options;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Options.Add(MakeOption(Index,
				*FString::Printf(TEXT("후보%d"), Index),
				Index != LockedIndex));
		}
		Board->SetCharacterOptions(Options, 3);
		return Board;
	}

	/** @brief 하나를 검토한 뒤 명시적으로 추가한다. */
	void Choose(UMercenaryHireWidget& Board, const int32 CardIndex)
	{
		Board.ClickCard(CardIndex);
		Board.ClickAdd();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireChooseTest,
	"P_RD.UI.MercenaryHire.Choose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireChooseTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();

	// 목록 클릭은 상세 검토만 바꾸고, 추가 버튼이 편성을 확정한다.
	Board->ClickCard(2);
	TestEqual(TEXT("목록 클릭은 검토 상태"), Board->StateOf(2),
		EMercenaryCardState::Reviewing);
	TestEqual(TEXT("목록 클릭만으로는 편성되지 않음"), Board->GetChosenIndices().Num(), 0);
	Board->ClickAdd();
	TestEqual(TEXT("추가 버튼으로 정해짐"), Board->StateOf(2),
		EMercenaryCardState::Chosen);
	TestEqual(TEXT("한 명 정해짐"), Board->GetChosenIndices().Num(), 1);

	// 정해진 것을 또 눌러도 풀리지 않는다(0807) -- 상세만 갈린다.
	// 빼기는 파티 칸이 맡는다(PartySlotRemove 시험).
	Board->ClickCard(2);
	TestEqual(TEXT("또 눌러도 그대로"), Board->GetChosenIndices().Num(), 1);
	TestEqual(TEXT("여전히 뽑혀 있다"), Board->StateOf(2),
		EMercenaryCardState::Chosen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHirePartyLimitTest,
	"P_RD.UI.MercenaryHire.PartyLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHirePartyLimitTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();
	TestFalse(TEXT("빈 채로는 못 떠난다"), Board->IsReadyToDepart());

	Choose(*Board, 0);
	Choose(*Board, 3);
	Choose(*Board, 5);
	TestTrue(TEXT("셋을 채우면 떠날 수 있다"), Board->IsReadyToDepart());
	TestEqual(TEXT("누른 차례가 파티 칸 순서"),
		Board->GetChosenIndices()[1], 3);

	// 자리가 찼으면 새 후보를 눌러도 편성이 안 바뀐다(0807) -- 상세만
	// 보인다. 바꾸려면 파티 칸에서 먼저 빼야 한다(PartySlotRemove 시험).
	TestEqual(TEXT("고르기 전 마지막 자리는 5"),
		Board->GetChosenIndices()[2], 5);
	Choose(*Board, 1);
	TestEqual(TEXT("여전히 셋"), Board->GetChosenIndices().Num(), 3);
	TestEqual(TEXT("마지막 자리 그대로 5"),
		Board->GetChosenIndices()[2], 5);
	TestFalse(TEXT("새 후보는 안 들어온다"),
		Board->GetChosenIndices().Contains(1));
	TestEqual(TEXT("먼저 고른 둘도 그대로"),
		Board->GetChosenIndices()[0], 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHirePartySlotRemoveTest,
	"P_RD.UI.MercenaryHire.PartySlotRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHirePartySlotRemoveTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();
	Choose(*Board, 0);
	Choose(*Board, 3);
	Choose(*Board, 5);

	Board->ClickPartySlot(1);
	TestEqual(TEXT("가운데 파티 슬롯을 누르면 한 명 빠짐"),
		Board->GetChosenIndices().Num(), 2);
	TestFalse(TEXT("누른 슬롯의 용병이 빠짐"),
		Board->GetChosenIndices().Contains(3));
	TestEqual(TEXT("뒤 슬롯이 앞으로 당겨짐"),
		Board->GetChosenIndices()[1], 5);

	Board->ClickPartySlot(2);
	TestEqual(TEXT("빈 슬롯을 눌러도 변화 없음"),
		Board->GetChosenIndices().Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireLockedTest,
	"P_RD.UI.MercenaryHire.Locked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireLockedTest::RunTest(const FString& Parameters)
{
	// 잠긴 후보는 눌러도 아무 일이 없다. 잠금 판정은 게임 모드가 끝내 놓았고
	// 화면은 그 bool 만 믿는다.
	UMercenaryHireWidget* Board = MakeBoard(/*LockedIndex=*/4);

	Choose(*Board, 4);
	TestEqual(TEXT("잠긴 것은 안 골린다"), Board->GetChosenIndices().Num(), 0);

	Choose(*Board, 0);
	TestEqual(TEXT("안 잠긴 것은 골린다"), Board->GetChosenIndices().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireConfirmTest,
	"P_RD.UI.MercenaryHire.Confirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireConfirmTest::RunTest(const FString& Parameters)
{
	// 출발이 넘기는 것은 식별자다. 런을 만드는 데 필요한 것이 그것뿐이다.
	UMercenaryHireWidget* Board = MakeBoard();
	TArray<FPrimaryAssetId> Sent;
	Board->mOnPartyConfirmed.AddLambda(
		[&Sent](const TArray<FPrimaryAssetId>& Party) { Sent = Party; });

	// 받는 쪽이 파티 칸 수만큼 오기를 요구한다. 빈 칸은 무효 id 다 --
	// 고른 것만 보내면 "파티 멤버 부족" 으로 죽는다.
	auto ValidCount = [](const TArray<FPrimaryAssetId>& Party)
	{
		int32 Count = 0;
		for (const FPrimaryAssetId& Id : Party)
		{
			Count += Id.IsValid() ? 1 : 0;
		}
		return Count;
	};

	Board->ConfirmParty();
	TestEqual(TEXT("아무도 안 고르면 안 넘어간다"), Sent.Num(), 0);

	// 용병 자료가 셋을 채울 만큼 안 들어와서, 한 명으로도 출발하게 열어 두었다.
	// 자료가 갖춰지면 mMinPartySize 를 셋으로 올리고 이 시험도 같이 조인다.
	Choose(*Board, 1);
	Board->ConfirmParty();
	TestEqual(TEXT("한 명이어도 칸 수만큼 온다"), Sent.Num(), 3);
	TestEqual(TEXT("그중 한 칸만 찼다"), ValidCount(Sent), 1);

	Choose(*Board, 2);
	Choose(*Board, 0);
	Board->ConfirmParty();
	TestEqual(TEXT("셋을 넘긴다"), Sent.Num(), 3);
	TestEqual(TEXT("셋 다 찼다"), ValidCount(Sent), 3);
	TestEqual(TEXT("고른 차례대로 온다"), Sent[2].PrimaryAssetName,
		FName(TEXT("후보0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireRuntimeBindingTest,
	"P_RD.UI.MercenaryHire.RuntimeBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireRuntimeBindingTest::RunTest(const FString& Parameters)
{
	const UGamePlaySettings* Settings = GetDefault<UGamePlaySettings>();
	if (!TestNotNull(TEXT("게임 플레이 설정이 있어야 한다"), Settings))
	{
		return false;
	}

	const int32 WidgetIndex = static_cast<int32>(EWorldWidgetType::MercenaryHire);
	UClass* WidgetClass = Settings->mWorldWidgetClasses[WidgetIndex].Get();
	if (!TestNotNull(TEXT("용병 선택 인게임 클래스가 로드되어야 한다"), WidgetClass))
	{
		return false;
	}

	TestTrue(TEXT("인게임 클래스가 UMercenaryHireWidget을 상속한다"),
		WidgetClass->IsChildOf(UMercenaryHireWidget::StaticClass()));
	TestEqual(TEXT("인게임 용병 선택은 신규 Marchbound WBP를 사용한다"),
		WidgetClass->GetPathName(),
		FString(TEXT("/Game/UI/CombatLayouts/WBP_MercenaryHire_Marchbound."
			"WBP_MercenaryHire_Marchbound_C")));

	static const TCHAR* MercenaryNames[6] = {
		TEXT("Knight"), TEXT("Mage"), TEXT("Ranger"),
		TEXT("Rogue"), TEXT("Barbarian"), TEXT("Druid")
	};
	for (const TCHAR* Name : MercenaryNames)
	{
		for (const TCHAR* Kind : { TEXT("Icon"), TEXT("Hero") })
		{
			const FString Path = FString::Printf(
				TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_Hire%s_%s.T_MB_Hire%s_%s"),
				Kind, Name, Kind, Name);
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
			if (TestNotNull(*Path, Texture))
			{
				TestEqual(*FString::Printf(TEXT("%s 원본 폭"), *Path),
					Texture->GetImportedSize().X, 1254);
				TestEqual(*FString::Printf(TEXT("%s 원본 높이"), *Path),
					Texture->GetImportedSize().Y, 1254);
			}
		}
	}

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
		AddInfo(TEXT("월드가 없어 런타임 검수용 후보 시험만 건너뜀"));
		return true;
	}

	UMercenaryHireWidget* Board = CreateWidget<UMercenaryHireWidget>(World, WidgetClass);
	if (!TestNotNull(TEXT("Marchbound 용병 선택 인스턴스"), Board))
	{
		return false;
	}
	Board->TakeWidget();
	TArray<FFrontendCharacterOption> KnightOnly;
	KnightOnly.Add(MakeOption(0, TEXT("기사"), true));
	Board->SetCharacterOptions(KnightOnly, 3);
	Board->ClickCard(5);
	Board->ClickAdd();
	TestFalse(TEXT("서버가 주지 않은 가짜 여섯째 후보는 선택되지 않는다"),
		Board->GetChosenIndices().Contains(5));
	TestEqual(TEXT("실제 후보 배열은 UI에서 임의로 늘리지 않는다"),
		Board->GetChosenIndices().Num(), 0);

	UButton* PartySlotButton = Cast<UButton>(
		Board->WidgetTree->FindWidget(TEXT("PartySlotButton_1")));
	UButton* AddButton = Cast<UButton>(
		Board->WidgetTree->FindWidget(TEXT("HireAddButton")));
	if (TestNotNull(TEXT("상세 아래 추가 버튼"), AddButton))
	{
		TestTrue(TEXT("추가 버튼 동작이 묶여 있다"), AddButton->OnClicked.IsBound());
	}
	if (TestNotNull(TEXT("파티 슬롯 해제 버튼"), PartySlotButton))
	{
		TestTrue(TEXT("파티 슬롯 해제 동작이 묶여 있다"),
			PartySlotButton->OnClicked.IsBound());
		PartySlotButton->OnClicked.Broadcast();
		TestFalse(TEXT("파티 슬롯을 누르면 해당 용병이 빠진다"),
			Board->GetChosenIndices().Contains(1));
	}

	Board->ApplyResponsiveLayoutForTest(FVector2D(900.0f, 1600.0f));
	UWidget* Card0 = Board->WidgetTree->FindWidget(TEXT("HireCard_0"));
	UWidget* Card1 = Board->WidgetTree->FindWidget(TEXT("HireCard_1"));
	UWidget* Card2 = Board->WidgetTree->FindWidget(TEXT("HireCard_2"));
	UCanvasPanelSlot* Card0Slot = Card0 != nullptr
		? Cast<UCanvasPanelSlot>(Card0->Slot) : nullptr;
	UCanvasPanelSlot* Card1Slot = Card1 != nullptr
		? Cast<UCanvasPanelSlot>(Card1->Slot) : nullptr;
	UCanvasPanelSlot* Card2Slot = Card2 != nullptr
		? Cast<UCanvasPanelSlot>(Card2->Slot) : nullptr;
	if (TestNotNull(TEXT("고정 레이아웃 카드 0 슬롯"), Card0Slot)
		&& TestNotNull(TEXT("고정 레이아웃 카드 1 슬롯"), Card1Slot)
		&& TestNotNull(TEXT("고정 레이아웃 카드 2 슬롯"), Card2Slot))
	{
		// 창 비율이 바뀌어도 좌측 목록·중앙 상세·우측 파티 구조는
		// 고정한다. 세로형에서 카드가 2열로 바뀌던 과거 계약은 폐기했다.
		TestEqual(TEXT("세로 비율에서도 후보는 같은 열"),
			Card0Slot->GetPosition().X, Card1Slot->GetPosition().X);
		TestTrue(TEXT("세로 비율에서도 1번은 0번 아래"),
			Card1Slot->GetPosition().Y > Card0Slot->GetPosition().Y);
		TestTrue(TEXT("세로 비율에서도 2번은 1번 아래"),
			Card2Slot->GetPosition().Y > Card1Slot->GetPosition().Y);
	}

	Board->ApplyResponsiveLayoutForTest(FVector2D(1920.0f, 1080.0f));
	if (Card0Slot != nullptr && Card1Slot != nullptr)
	{
		TestEqual(TEXT("가로에서 후보는 같은 열"),
			Card0Slot->GetPosition().X, Card1Slot->GetPosition().X);
		TestTrue(TEXT("가로에서 1번은 0번 아래"),
			Card1Slot->GetPosition().Y > Card0Slot->GetPosition().Y);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDCardNestingTest,
	"P_RD.UI.CombatHUD.CardNesting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 카드 묶음이 제 부품을 실제로 품고 있나.
 *
 * @details
 * 카드가 안 접힌다를 여러 번 되풀이했다. 그때마다 입력 쪽을 뒤졌는데 첫
 * 원인은 위젯 구조였다 -- 묶음이 빈 껍데기여서 접어도 접을 것이 없었다.
 * 화면은 멀쩡히 나오므로 캡처로는 절대 안 잡힌다.
 *
 * 이 시험이 통과하는데 게임에서 안 되면 원인은 위젯이 아니라 입력이 거기까지
 * 안 온다는 뜻이다. 그 둘을 가르는 것이 이 시험의 일이다.
 *
 * 새 파일을 만들지 않고 여기 붙인 이유: P_RDTests 에 .cpp 를 하나 더하면
 * 유니티 빌드 묶음이 바뀌면서, 여러 시험 파일이 익명 이름공간에 같은 이름으로
 * 둔 도우미들이 한꺼번에 부딪힌다.
 */
bool FCombatHUDCardNestingTest::RunTest(const FString& Parameters)
{
	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD 클래스를 찾았다"), HUDClass))
	{
		return false;
	}

	// 위젯 나무는 WBP 가 만든 클래스가 원본으로 들고 있다. 인스턴스나 클래스
	// 기본값에서 읽으면 아직 비어 있어 늘 널이다.
	UWidgetBlueprintGeneratedClass* Generated =
		Cast<UWidgetBlueprintGeneratedClass>(HUDClass);
	UWidgetTree* Tree = Generated != nullptr
		? Generated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("위젯 나무"), Tree))
	{
		return false;
	}

	auto CountChildren = [Tree](const TCHAR* Name) -> int32
	{
		UPanelWidget* Panel = Cast<UPanelWidget>(Tree->FindWidget(FName(Name)));
		return Panel != nullptr ? Panel->GetChildrenCount() : -1;
	};

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::CommandSlotCount; ++Index)
	{
		const FString Name = FString::Printf(TEXT("CommandCard_%d"), Index);
		// 판, 아이콘, 글자, 버튼이 이 안에 있어야 한다. 하나라도 밖에 있으면
		// 접었을 때 그것만 남는다.
		TestTrue(*FString::Printf(TEXT("%s 가 부품을 품는다"), *Name),
			CountChildren(*Name) >= 4);
	}

	// 새 파티 카드는 부품을 중간 컨테이너(PartyContent)로 한 번 감싼다. 접힘
	// 계약은 "부품이 카드 계보 안에 있는가"이므로 직계가 아니라 자손으로 센다.
	auto CountDescendants = [Tree](const TCHAR* Name) -> int32
	{
		UPanelWidget* Panel = Cast<UPanelWidget>(Tree->FindWidget(FName(Name)));
		if (Panel == nullptr)
		{
			return -1;
		}
		int32 Count = 0;
		TArray<UWidget*> Pending;
		for (int32 Child = 0; Child < Panel->GetChildrenCount(); ++Child)
		{
			Pending.Add(Panel->GetChildAt(Child));
		}
		while (Pending.Num() > 0)
		{
			UWidget* Widget = Pending.Pop();
			if (Widget == nullptr)
			{
				continue;
			}
			++Count;
			if (UPanelWidget* Nested = Cast<UPanelWidget>(Widget))
			{
				for (int32 Child = 0; Child < Nested->GetChildrenCount(); ++Child)
				{
					Pending.Add(Nested->GetChildAt(Child));
				}
			}
		}
		return Count;
	};
	auto IsInsideCard = [](UWidget* Widget, UPanelWidget* Card) -> bool
	{
		for (UPanelWidget* Parent = Widget != nullptr ? Widget->GetParent() : nullptr;
			Parent != nullptr; Parent = Parent->GetParent())
		{
			if (Parent == Card)
			{
				return true;
			}
		}
		return false;
	};

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::PartySlotCount; ++Index)
	{
		const FString Name = FString::Printf(TEXT("PartyCard_%d"), Index);
		TestTrue(*FString::Printf(TEXT("%s 가 부품을 품는다"), *Name),
			CountDescendants(*Name) >= 4);

		// 다시 펴는 손잡이. 카드 계보 안에 있어야 접힌 카드가 안 눌린다.
		const FString ButtonName = FString::Printf(TEXT("PartyButton_%d"), Index);
		UWidget* Button = Tree->FindWidget(FName(*ButtonName));
		if (TestNotNull(*ButtonName, Button))
		{
			TestTrue(*FString::Printf(TEXT("%s 가 카드 안에 있다"), *ButtonName),
				IsInsideCard(Button,
					Cast<UPanelWidget>(Tree->FindWidget(FName(*Name)))));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDMercenaryTabStructureTest,
	"P_RD.UI.CombatHUD.MercenaryTabStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 전투 HUD의 네 칸 메뉴와 보유 용병/AP/아티팩트 묶음을 함께 검사한다.
 *
 * @details
 * 보유 용병 카드는 더 이상 좌하단 상시 HUD가 아니다. 용병 패널 안의
 * MercenaryBoard로 옮기고, 좌하단에는 AP와 그 위 아티팩트만 남긴다.
 * 이름만 만든 뒤 부모를 옮기지 않는 회귀도 잡도록 실제 부모까지 확인한다.
 */
bool FCombatHUDMercenaryTabStructureTest::RunTest(const FString& Parameters)
{
	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD 클래스를 찾았다"), HUDClass))
	{
		return false;
	}

	UWidgetBlueprintGeneratedClass* Generated =
		Cast<UWidgetBlueprintGeneratedClass>(HUDClass);
	UWidgetTree* Tree = Generated != nullptr
		? Generated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("위젯 나무"), Tree))
	{
		return false;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FString Name = FString::Printf(TEXT("MenuButton_%d"), Index);
		TestNotNull(*FString::Printf(TEXT("%s 유지"), *Name),
			Cast<UButton>(Tree->FindWidget(FName(*Name))));
	}

	UImage* OptionsRailFrame =
		Cast<UImage>(Tree->FindWidget(TEXT("OptionsRailFrame")));
	if (TestNotNull(TEXT("옵션 네 칸 프레임"), OptionsRailFrame))
	{
		TestNotNull(TEXT("옵션 프레임 텍스처"),
			Cast<UTexture2D>(OptionsRailFrame->GetBrush().GetResourceObject()));
	}
	for (const TCHAR* Name : {
		TEXT("MenuMapIcon"), TEXT("MenuMercenaryIcon"),
		TEXT("MenuMonsterIcon"), TEXT("MenuSettingsIcon") })
	{
		UImage* Icon = Cast<UImage>(Tree->FindWidget(FName(Name)));
		if (TestNotNull(*FString::Printf(TEXT("%s 아이콘"), Name), Icon))
		{
			TestNotNull(*FString::Printf(TEXT("%s 텍스처"), Name),
				Cast<UTexture2D>(Icon->GetBrush().GetResourceObject()));
		}
	}
	for (const TCHAR* Name : {
		TEXT("MenuMercenaryMaskLabel"), TEXT("MenuEmptyMaskLabel") })
	{
		TestNull(*FString::Printf(TEXT("%s 제거"), Name),
			Tree->FindWidget(FName(Name)));
	}

	// 전장 유닛 탭은 상세창을 열지 않고 같은 크기의 요약판 두 종류로 읽는다.
	// 아군/적 중 하나만 선택되지만 WBP에는 두 계약이 모두 있어야 한다.
	for (const TCHAR* Prefix : { TEXT("Enemy"), TEXT("Ally") })
	{
		const FName PanelName(*FString::Printf(TEXT("%sPanel"), Prefix));
		UWidget* SummaryPanel = Tree->FindWidget(PanelName);
		if (TestNotNull(*FString::Printf(TEXT("%s 요약판"), Prefix), SummaryPanel))
		{
			TestEqual(*FString::Printf(TEXT("%s 요약판 기본값은 닫힘"), Prefix),
				SummaryPanel->GetVisibility(), ESlateVisibility::Collapsed);
			if (const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(SummaryPanel->Slot))
			{
				TestEqual(*FString::Printf(TEXT("%s 요약판 너비"), Prefix),
					Slot->GetSize().X, 600.0);
				TestEqual(*FString::Printf(TEXT("%s 요약판 높이"), Prefix),
					Slot->GetSize().Y, 430.0);
			}
		}
		for (const TCHAR* Suffix : {
			TEXT("Portrait"), TEXT("Name"), TEXT("HPBar"), TEXT("HPText"),
			TEXT("APText"), TEXT("SpeedText"), TEXT("Status") })
		{
			const FString WidgetName = FString::Printf(TEXT("%s%s"), Prefix, Suffix);
			TestNotNull(*WidgetName, Tree->FindWidget(FName(*WidgetName)));
		}
		for (int32 StatusIndex = 0; StatusIndex < 3; ++StatusIndex)
		{
			for (const TCHAR* Suffix : { TEXT("Frame"), TEXT("Icon"), TEXT("Count") })
			{
				const FString WidgetName = FString::Printf(
					TEXT("%sStatus%s_%d"), Prefix, Suffix, StatusIndex);
				TestNotNull(*WidgetName, Tree->FindWidget(FName(*WidgetName)));
			}
		}
	}
	// 구형 요약판 빌더의 큰 AP 판은 (54,184), 230x58이었다. 호환 위젯의
	// Visibility는 BP 그래프/겹 순서가 관리할 수 있으므로, 실제 회귀 원인이었던
	// 좌표 서명을 검사한다.
	if (UWidget* EnemyAPPlate = Tree->FindWidget(TEXT("EnemyAPPlate")))
	{
		if (const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(EnemyAPPlate->Slot))
		{
			const bool bLegacyBuilderLayout =
				Slot->GetPosition().Equals(FVector2D(54.f, 184.f))
				&& Slot->GetSize().Equals(FVector2D(230.f, 58.f));
			TestFalse(TEXT("몬스터 AP 판이 구형 전체 HUD 빌더 좌표로 돌아가지 않는다"),
				bLegacyBuilderLayout);
		}
	}
	UPanelWidget* EnemyAPPipRow = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("EnemyAPPipRow")));
	if (TestNotNull(TEXT("몬스터 요약판 AP 보석 WBP 행"), EnemyAPPipRow))
	{
		TestNotNull(TEXT("몬스터 AP 보석 행은 배경 없는 Canvas"),
			Cast<UCanvasPanel>(EnemyAPPipRow));
		TestNull(TEXT("몬스터 AP 보석 뒤 구식 막대는 제거됨"),
			Tree->FindWidget(TEXT("EnemyCritPlate")));
		for (int32 Index = 0; Index < 10; ++Index)
		{
			UImage* Pip = Cast<UImage>(Tree->FindWidget(FName(*FString::Printf(
				TEXT("EnemyAPPip_%d"), Index))));
			UImage* UsedPip = Cast<UImage>(Tree->FindWidget(FName(*FString::Printf(
				TEXT("EnemyAPPipUsed_%d"), Index))));
			if (TestNotNull(*FString::Printf(TEXT("몬스터 AP 보석 %d"), Index), Pip))
			{
				TestEqual(*FString::Printf(TEXT("몬스터 AP 보석 %d 부모"), Index),
					Pip->GetParent(), EnemyAPPipRow);
				TestNotNull(*FString::Printf(TEXT("몬스터 AP 보석 %d 텍스처"), Index),
					Pip->GetBrush().GetResourceObject());
				if (UCanvasPanelSlot* PipSlot = Cast<UCanvasPanelSlot>(Pip->Slot))
				{
					TestEqual(*FString::Printf(TEXT("몬스터 AP 보석 %d 크기"), Index),
						PipSlot->GetSize(), FVector2D(30.f, 30.f));
				}
			}
			if (TestNotNull(*FString::Printf(TEXT("몬스터 빈 AP 보석 %d"), Index),
				UsedPip))
			{
				TestEqual(*FString::Printf(TEXT("몬스터 빈 AP 보석 %d 부모"), Index),
					UsedPip->GetParent(), EnemyAPPipRow);
				TestNotNull(*FString::Printf(TEXT("몬스터 빈 AP 보석 %d 텍스처"), Index),
					UsedPip->GetBrush().GetResourceObject());
			}
		}
	}

	UPanelWidget* MercenaryPanel =
		Cast<UPanelWidget>(Tree->FindWidget(TEXT("MercenaryPanel")));
	UPanelWidget* MercenaryBoard =
		Cast<UPanelWidget>(Tree->FindWidget(TEXT("MercenaryBoard")));
	TestNotNull(TEXT("보유 용병 패널"), MercenaryPanel);
	TestNotNull(TEXT("보유 용병 판"), MercenaryBoard);
	TestNotNull(TEXT("보유 골드 글자"),
		Cast<UTextBlock>(Tree->FindWidget(TEXT("MercenaryGoldText"))));
	TestNotNull(TEXT("용병 패널 닫기 단추"),
		Cast<UButton>(Tree->FindWidget(TEXT("MercenaryCloseButton"))));
	TestNotNull(TEXT("용병 패널 뒤로 프레임"),
		Cast<UImage>(Tree->FindWidget(TEXT("MercenaryBackArt"))));
	if (UTextBlock* BackText =
		Cast<UTextBlock>(Tree->FindWidget(TEXT("MercenaryCloseText"))))
	{
		// 확정 시안: 보유 용병 조회 탭이라 "뒤로" 대신 "닫기"다.
		TestEqual(TEXT("용병 패널 닫기 문구"),
			BackText->GetText().ToString(), FString(TEXT("닫기")));
	}
	else
	{
		AddError(TEXT("MercenaryCloseText를 찾을 수 없다"));
	}
	if (MercenaryPanel != nullptr)
	{
		TestEqual(TEXT("용병 패널 WBP 기본값은 닫힘"),
			MercenaryPanel->GetVisibility(), ESlateVisibility::Collapsed);
	}
	TestNull(TEXT("새 용병 셸을 덮던 구식 암막은 제거됨"),
		Tree->FindWidget(TEXT("MercenaryScrim")));

	for (int32 Index = 0;
		Index < UCombatLayoutHUDWidget::PartySlotCount; ++Index)
	{
		const FString ScaleName =
			FString::Printf(TEXT("MercenaryCardScale_%d"), Index);
		const FString CardName = FString::Printf(TEXT("PartyCard_%d"), Index);
		UPanelWidget* Scale =
			Cast<UPanelWidget>(Tree->FindWidget(FName(*ScaleName)));
		UWidget* Card = Tree->FindWidget(FName(*CardName));
		if (TestNotNull(*ScaleName, Scale)
			&& TestNotNull(*CardName, Card))
		{
			// 카드들은 편집 편의를 위해 로스터 구역(MercRosterSection)으로
			// 묶였다. 구역이 판 안에 있고, 카드가 구역 안에 있으면 된다.
			UPanelWidget* Roster = Cast<UPanelWidget>(
				Tree->FindWidget(TEXT("MercRosterSection")));
			TestEqual(*FString::Printf(TEXT("%s 는 로스터 구역 안"), *ScaleName),
				Scale->GetParent(), Roster != nullptr ? Roster : MercenaryBoard);
			if (Roster != nullptr)
			{
				TestEqual(TEXT("로스터 구역은 용병 판 안"),
					Roster->GetParent(), MercenaryBoard);
			}
			TestEqual(*FString::Printf(TEXT("%s 는 크기 래퍼 안"), *CardName),
				Card->GetParent(), Scale);
		}
	}

	UPanelWidget* InventoryTab = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("MercenaryInventoryTab")));
	UPanelWidget* InventoryScale = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("MercenaryInventoryScale")));
	if (TestNotNull(TEXT("용병 목록 아래 인벤토리 WBP 탭"), InventoryTab))
	{
		UPanelWidget* Roster = Cast<UPanelWidget>(
			Tree->FindWidget(TEXT("MercRosterSection")));
		if (TestNotNull(TEXT("인벤토리 탭 크기 래퍼"), InventoryScale))
		{
			TestEqual(TEXT("인벤토리 탭 래퍼는 세 용병과 같은 로스터 안"),
				InventoryScale->GetParent(), Roster);
			TestEqual(TEXT("인벤토리 카드는 크기 래퍼 안"),
				InventoryTab->GetParent(), InventoryScale);
		}
		UWidget* SecondCard = Tree->FindWidget(TEXT("MercenaryCardScale_1"));
		UWidget* ThirdCard = Tree->FindWidget(TEXT("MercenaryCardScale_2"));
		UCanvasPanelSlot* SecondSlot = SecondCard != nullptr
			? Cast<UCanvasPanelSlot>(SecondCard->Slot) : nullptr;
		UCanvasPanelSlot* ThirdSlot = ThirdCard != nullptr
			? Cast<UCanvasPanelSlot>(ThirdCard->Slot) : nullptr;
		UCanvasPanelSlot* InventorySlot = InventoryScale != nullptr
			? Cast<UCanvasPanelSlot>(InventoryScale->Slot) : nullptr;
		if (TestNotNull(TEXT("두 번째 용병 카드 Canvas 슬롯"), SecondSlot)
			&& TestNotNull(TEXT("세 번째 용병 카드 Canvas 슬롯"), ThirdSlot)
			&& TestNotNull(TEXT("인벤토리 탭 Canvas 슬롯"), InventorySlot))
		{
			const FVector2D ExpectedPosition = ThirdSlot->GetPosition()
				+ (ThirdSlot->GetPosition() - SecondSlot->GetPosition());
			const FVector2D ActualPosition = InventorySlot->GetPosition();
			TestTrue(*FString::Printf(
				TEXT("인벤토리 탭은 세 용병 바로 다음 행 (expected=%s actual=%s)"),
				*ExpectedPosition.ToString(), *ActualPosition.ToString()),
				ActualPosition.Equals(ExpectedPosition, 0.5f));
			TestEqual(TEXT("인벤토리 탭 크기는 용병 카드와 동일"),
				InventorySlot->GetSize(), ThirdSlot->GetSize());
		}
		for (const TCHAR* Name : {
			TEXT("MercenaryInventoryTabPlate"),
			TEXT("MercenaryInventoryTabIcon"),
			TEXT("MercenaryInventoryTabText"),
			TEXT("MercenaryInventoryButton") })
		{
			UWidget* Part = Tree->FindWidget(FName(Name));
			if (TestNotNull(*FString::Printf(TEXT("%s WBP 부품"), Name), Part))
			{
				TestEqual(*FString::Printf(TEXT("%s 는 인벤토리 탭 안"), Name),
					Part->GetParent(), InventoryTab);
			}
		}
		if (UImage* Icon = Cast<UImage>(
			Tree->FindWidget(TEXT("MercenaryInventoryTabIcon"))))
		{
			TestNotNull(TEXT("인벤토리 탭 아이콘 텍스처"),
				Cast<UTexture2D>(Icon->GetBrush().GetResourceObject()));
		}
		UImage* InventoryPlate = Cast<UImage>(
			Tree->FindWidget(TEXT("MercenaryInventoryTabPlate")));
		UImage* PartyPlate = Cast<UImage>(Tree->FindWidget(TEXT("PartyPlate_2")));
		if (TestNotNull(TEXT("인벤토리 탭 카드 판"), InventoryPlate)
			&& TestNotNull(TEXT("용병 카드 판"), PartyPlate))
		{
			TestEqual(TEXT("인벤토리 탭은 용병 카드와 같은 에셋"),
				InventoryPlate->GetBrush().GetResourceObject(),
				PartyPlate->GetBrush().GetResourceObject());
		}
	}
	UPanelWidget* InventoryPage = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("MercenaryInventoryPage")));
	if (TestNotNull(TEXT("용병 판 내부 인벤토리 페이지"), InventoryPage))
	{
		TestEqual(TEXT("인벤토리 페이지는 용병 판 안"),
			InventoryPage->GetParent(), MercenaryBoard);
		TestEqual(TEXT("인벤토리 페이지 WBP 기본값은 닫힘"),
			InventoryPage->GetVisibility(), ESlateVisibility::Collapsed);
		UCanvasPanelSlot* InventoryPageSlot =
			Cast<UCanvasPanelSlot>(InventoryPage->Slot);
		if (TestNotNull(TEXT("인벤토리 페이지 Canvas 슬롯"), InventoryPageSlot))
		{
			TestTrue(TEXT("인벤토리 페이지는 왼쪽 로스터를 침범하지 않는다"),
				InventoryPageSlot->GetPosition().X >= 400.f);
			TestTrue(TEXT("인벤토리 페이지는 전체 화면 덮개가 아니다"),
				InventoryPageSlot->GetSize().X < 1600.f
				&& InventoryPageSlot->GetSize().Y < 900.f);
		}
		TestNotNull(TEXT("인벤토리 페이지 골드"),
			Tree->FindWidget(TEXT("MercenaryInventoryGoldText")));
		TestNotNull(TEXT("인벤토리 페이지 골드 실에셋"),
			Tree->FindWidget(TEXT("MercenaryInventoryGoldIcon")));
		// 0809 확정: 설명 띠는 접어 둔다 -- 상세는 칸 클릭 팝업으로 본다.
		for (const TCHAR* Retired : { TEXT("MercenaryInventoryDescriptionPlate"),
			TEXT("MercenaryInventoryDescriptionText") })
		{
			if (UWidget* Widget = Tree->FindWidget(FName(Retired)))
			{
				TestEqual(*FString::Printf(TEXT("%s 는 접혀 있다"), Retired),
					Widget->GetVisibility(), ESlateVisibility::Collapsed);
			}
		}
		// 0809 시안: 격자 4x2 = 골드 + 아티팩트 7칸. 고름 테두리는 없다.
		for (int32 Index = 0; Index < 7; ++Index)
		{
			TestNotNull(*FString::Printf(TEXT("인벤토리 아티팩트 프레임 %d"), Index),
				Tree->FindWidget(FName(*FString::Printf(
					TEXT("MercenaryInventoryArtifactFrame_%d"), Index))));
			TestNotNull(*FString::Printf(TEXT("인벤토리 아티팩트 아이콘 %d"), Index),
				Tree->FindWidget(FName(*FString::Printf(
					TEXT("MercenaryInventoryArtifactIcon_%d"), Index))));
			TestNotNull(*FString::Printf(TEXT("인벤토리 아티팩트 버튼 %d"), Index),
				Tree->FindWidget(FName(*FString::Printf(
					TEXT("MercenaryInventoryArtifactButton_%d"), Index))));
		}
		TestNull(TEXT("고름 테두리는 뺐다(0809)"),
			Tree->FindWidget(TEXT("MercenaryInventoryArtifactSelection_0")));
	}

	UWidget* TurnAPPanel = Tree->FindWidget(TEXT("TurnAPPanel"));
	TestNotNull(TEXT("좌하단 AP 루트"), TurnAPPanel);

	TestNull(TEXT("전투 화면 독립 아티팩트 WBP 줄 제거"),
		Tree->FindWidget(TEXT("ArtifactStrip")));

	UClass* MonsterTabClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound.WBP_MonsterTab_Marchbound_C"));
	UWidgetBlueprintGeneratedClass* MonsterGenerated =
		Cast<UWidgetBlueprintGeneratedClass>(MonsterTabClass);
	UWidgetTree* MonsterTree = MonsterGenerated != nullptr
		? MonsterGenerated->GetWidgetTreeArchetype() : nullptr;
	if (TestNotNull(TEXT("몬스터 탭 WBP 나무"), MonsterTree))
	{
		TestNotNull(TEXT("몬스터 탭 뒤로 프레임"),
			Cast<UImage>(MonsterTree->FindWidget(TEXT("MonsterBackArt"))));
		TestNotNull(TEXT("몬스터 탭 뒤로 단추"),
			Cast<UButton>(MonsterTree->FindWidget(TEXT("MonsterBackButton"))));
		if (UTextBlock* BackText =
			Cast<UTextBlock>(MonsterTree->FindWidget(TEXT("MonsterBackText"))))
		{
			// 확정 시안: 몬스터 탭도 "뒤로" 대신 "닫기"다.
			TestEqual(TEXT("몬스터 탭 닫기 문구"),
				BackText->GetText().ToString(), FString(TEXT("닫기")));
		}
		else
		{
			AddError(TEXT("MonsterBackText를 찾을 수 없다"));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDMercenaryTabBehaviorTest,
	"P_RD.UI.CombatHUD.MercenaryTabBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 용병 메뉴와 몬스터 메뉴, 골드와 아티팩트 갱신을 확인한다.
 */
bool FCombatHUDMercenaryTabBehaviorTest::RunTest(const FString& Parameters)
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
	UCombatLayoutHUDWidget* HUD = HUDClass != nullptr
		? CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass) : nullptr;
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->TakeWidget();
	HUD->SetDetailOverlayWidgetClassForTest(
		UMercenaryDetailTestWidget::StaticClass());

	UButton* MercenaryMenu = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MenuButton_1")));
	UButton* MonsterMenu = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MenuButton_2")));
	UImage* MercenaryMenuIcon = Cast<UImage>(
		HUD->WidgetTree->FindWidget(TEXT("MenuMercenaryIcon")));
	UImage* MonsterMenuIcon = Cast<UImage>(
		HUD->WidgetTree->FindWidget(TEXT("MenuMonsterIcon")));
	UWidget* Panel = HUD->WidgetTree->FindWidget(TEXT("MercenaryPanel"));
	UTextBlock* Gold = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("MercenaryGoldText")));
	UButton* Close = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MercenaryCloseButton")));
	UButton* PartyButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("PartyButton_0")));
	UButton* InventoryButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("MercenaryInventoryButton")));
	UWidget* TurnAPRoot = HUD->WidgetTree->FindWidget(TEXT("TurnAPPanel"));
	UWidget* EnemyPanel = HUD->WidgetTree->FindWidget(TEXT("EnemyPanel"));
	UTextBlock* EnemyAPText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("EnemyAPText")));
	UWidget* AllyPanel = HUD->WidgetTree->FindWidget(TEXT("AllyPanel"));
	UTextBlock* AllyName = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("AllyName")));
	UWidget* InventoryPage = HUD->WidgetTree->FindWidget(
		TEXT("MercenaryInventoryPage"));
	UWidget* DetailSection = HUD->WidgetTree->FindWidget(TEXT("MercDetailSection"));
	UTextBlock* InventoryGold = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("MercenaryInventoryGoldText")));
	UImage* InventoryArtifact0 = Cast<UImage>(HUD->WidgetTree->FindWidget(
		TEXT("MercenaryInventoryArtifactIcon_0")));
	UButton* InventoryArtifactButton1 = Cast<UButton>(HUD->WidgetTree->FindWidget(
		TEXT("MercenaryInventoryArtifactButton_1")));
	UImage* RuntimeShell = Cast<UImage>(
		HUD->WidgetTree->FindWidget(TEXT("RuntimeMercenaryRosterShell")));
	if (!TestNotNull(TEXT("용병 메뉴"), MercenaryMenu)
		|| !TestNotNull(TEXT("몬스터 메뉴"), MonsterMenu)
		|| !TestNotNull(TEXT("용병 메뉴 아이콘"), MercenaryMenuIcon)
		|| !TestNotNull(TEXT("몬스터 메뉴 아이콘"), MonsterMenuIcon)
		|| !TestNotNull(TEXT("용병 패널"), Panel)
		|| !TestNotNull(TEXT("보유 골드"), Gold)
		|| !TestNotNull(TEXT("닫기"), Close)
		|| !TestNotNull(TEXT("첫 용병"), PartyButton)
		|| !TestNotNull(TEXT("인벤토리 탭 단추"), InventoryButton)
		|| !TestNotNull(TEXT("공용 AP 막대"), TurnAPRoot)
		|| !TestNotNull(TEXT("몬스터 요약판"), EnemyPanel)
		|| !TestNotNull(TEXT("몬스터 요약판 AP"), EnemyAPText)
		|| !TestNotNull(TEXT("용병 요약판"), AllyPanel)
		|| !TestNotNull(TEXT("용병 요약판 이름"), AllyName)
		|| !TestNotNull(TEXT("용병 내부 인벤토리 페이지"), InventoryPage)
		|| !TestNotNull(TEXT("용병 상세 구역"), DetailSection)
		|| !TestNotNull(TEXT("내부 인벤토리 골드"), InventoryGold)
		|| !TestNotNull(TEXT("내부 인벤토리 첫 아티팩트"), InventoryArtifact0)
		|| !TestNotNull(TEXT("내부 인벤토리 둘째 선택 버튼"), InventoryArtifactButton1)
		|| !TestNotNull(TEXT("런타임 프리미엄 셸"), RuntimeShell))
	{
		return false;
	}
	TestEqual(TEXT("프리미엄 셸은 용병 패널의 직접 자식"),
		RuntimeShell->GetParent(), Cast<UPanelWidget>(Panel));
	TestEqual(TEXT("프리미엄 셸은 입력을 받지 않는다"),
		RuntimeShell->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestNotNull(TEXT("프리미엄 셸 텍스처를 실제로 읽는다"),
		Cast<UTexture2D>(RuntimeShell->GetBrush().GetResourceObject()));
	if (UCanvasPanelSlot* ShellSlot =
		Cast<UCanvasPanelSlot>(RuntimeShell->Slot))
	{
		TestEqual(TEXT("프리미엄 셸은 가장 뒤에 놓인다"),
			ShellSlot->GetZOrder(), -100);
		TestEqual(TEXT("프리미엄 셸 왼쪽 위 앵커"),
			ShellSlot->GetAnchors().Minimum, FVector2D::ZeroVector);
		// 0809 시안: 용병 판은 화면을 가득 채우는 전면 판이다 (모달 여백 없음).
		TestEqual(TEXT("프리미엄 셸은 화면 전체로 늘어난다"),
			ShellSlot->GetAnchors().Maximum, FVector2D(1.f, 1.f));
		TestEqual(TEXT("프리미엄 셸은 가장자리에 붙는다"),
			ShellSlot->GetOffsets(), FMargin(0.f, 0.f, 0.f, 0.f));
	}
	else
	{
		AddError(TEXT("프리미엄 셸은 CanvasPanelSlot이어야 한다"));
	}
	for (const TCHAR* Name : {
		TEXT("MercenaryScrim"), TEXT("MercenaryHeaderPlate"),
		TEXT("MercenaryBoardPlate"),
		TEXT("MercenaryBoardShadow"), TEXT("MercenaryBoardInner"),
		TEXT("MercenaryClosePlate") })
	{
		TestNull(*FString::Printf(TEXT("%s 구식 판은 제거됨"), Name),
			HUD->WidgetTree->FindWidget(FName(Name)));
	}
	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::PartySlotCount; ++Index)
	{
		UImage* CardPlate = Cast<UImage>(HUD->WidgetTree->FindWidget(FName(
			*FString::Printf(TEXT("PartyPlate_%d"), Index))));
		if (TestNotNull(*FString::Printf(TEXT("용병 카드 프레임 %d"), Index),
			CardPlate))
		{
			TestEqual(*FString::Printf(TEXT("용병 카드 프레임 %d 표시"), Index),
				CardPlate->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
			TestNotNull(*FString::Printf(TEXT("용병 카드 프레임 %d 텍스처"), Index),
				Cast<UTexture2D>(CardPlate->GetBrush().GetResourceObject()));
		}
	}

	TestTrue(TEXT("용병 메뉴에 열기 동작이 묶여 있다"),
		MercenaryMenu->OnClicked.IsBound());
	TestTrue(TEXT("몬스터 메뉴는 활성"), MonsterMenu->GetIsEnabled());
	TestTrue(TEXT("몬스터 메뉴에는 상세 열기 동작이 있다"),
		MonsterMenu->OnClicked.IsBound());
	TestNotNull(TEXT("용병 메뉴 아이콘 텍스처"),
		MercenaryMenuIcon->GetBrush().GetResourceObject());
	TestNotNull(TEXT("몬스터 메뉴 아이콘 텍스처"),
		MonsterMenuIcon->GetBrush().GetResourceObject());
	TestTrue(TEXT("닫기 단추에 동작이 묶여 있다"),
		Close->OnClicked.IsBound());
	TestTrue(TEXT("인벤토리 탭에 열기 동작이 묶여 있다"),
		InventoryButton->OnClicked.IsBound());
	TestEqual(TEXT("전투 진입 때 용병 패널은 닫힘"),
		Panel->GetVisibility(), ESlateVisibility::Collapsed);

	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);
	UMercenaryDetailTestResponder* DetailResponder =
		NewObject<UMercenaryDetailTestResponder>(HUD);
	DetailResponder->Bind(Model);
	FUnitUI MonsterUnit;
	MonsterUnit.mUnitId = 888;
	MonsterUnit.mIsPlayer = false;
	MonsterUnit.mName = FText::FromString(TEXT("상세를 볼 몬스터"));
	MonsterUnit.mHP = 50.f;
	MonsterUnit.mMaxHP = 50.f;
	MonsterUnit.mActionPoints = 3;
	MonsterUnit.mMaxActionPoints = 5;
	MonsterUnit.mMovementPoint = 3.f;
	MonsterUnit.mMaxMovementPoint = 5.f;
	Model->SetUnitUIs({ MonsterUnit });
	FTurnUI MonsterTurn;
	MonsterTurn.mCurrentUnitId = MonsterUnit.mUnitId;
	MonsterTurn.mTurnOrderUnitIds.Add(MonsterUnit.mUnitId);
	Model->SetTurnUI(MonsterTurn);
	TestEqual(TEXT("몬스터 차례에는 공용 AP 막대를 접는다"),
		TurnAPRoot->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("몬스터 차례에는 몬스터 요약판을 보인다"),
		EnemyPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("몬스터 차례에는 겹친 용병 요약판을 접는다"),
		AllyPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("몬스터 AP는 요약판에서 읽는다"),
		EnemyAPText->GetText().ToString(), FString(TEXT("AP 3/5")));
	for (int32 Index = 0; Index < 10; ++Index)
	{
		UWidget* Pip = HUD->WidgetTree->FindWidget(FName(*FString::Printf(
			TEXT("EnemyAPPip_%d"), Index)));
		UWidget* UsedPip = HUD->WidgetTree->FindWidget(FName(*FString::Printf(
			TEXT("EnemyAPPipUsed_%d"), Index)));
		if (TestNotNull(*FString::Printf(TEXT("몬스터 AP 보석 %d"), Index), Pip))
		{
			TestEqual(*FString::Printf(TEXT("남은 AP에 맞춘 보석 %d"), Index),
				Pip->GetVisibility(), Index < 3
					? ESlateVisibility::SelfHitTestInvisible
					: ESlateVisibility::Collapsed);
		}
		if (TestNotNull(*FString::Printf(TEXT("몬스터 빈 AP 보석 %d"), Index),
			UsedPip))
		{
			TestEqual(*FString::Printf(TEXT("남은 10칸을 빈 보석으로 표시 %d"), Index),
				UsedPip->GetVisibility(), Index >= 3
					? ESlateVisibility::SelfHitTestInvisible
					: ESlateVisibility::Collapsed);
		}
	}
	MonsterUnit.mActionPoints = 2;
	MonsterUnit.mMovementPoint = 2.f;
	Model->SetUnitUIs({ MonsterUnit });
	TestEqual(TEXT("몬스터의 실제 AP가 한 칸 줄면 숫자가 즉시 갱신된다"),
		EnemyAPText->GetText().ToString(), FString(TEXT("AP 2/5")));
	if (UWidget* SpentPip = HUD->WidgetTree->FindWidget(TEXT("EnemyAPPip_2")))
	{
		TestEqual(TEXT("몬스터가 AP를 쓰면 보석 하나가 사라진다"),
			SpentPip->GetVisibility(), ESlateVisibility::Collapsed);
	}
	if (UWidget* EmptyPip = HUD->WidgetTree->FindWidget(TEXT("EnemyAPPipUsed_2")))
	{
		TestEqual(TEXT("몬스터의 실제 AP가 줄면 빈 보석이 남는다"),
			EmptyPip->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}
	MonsterMenu->OnClicked.Broadcast();
	if (HUD->IsMonsterTabShown() == true)
	{
		// 몬스터 탭 WBP가 있는 환경: 탭이 열리고 선택 몬스터의 상세를 청한다.
		TestEqual(TEXT("몬스터 탭은 선택 몬스터를 InspectUnit으로 청한다"),
			DetailResponder->mLastPayload, MonsterUnit.mUnitId);
		TestFalse(TEXT("몬스터 탭이 열리면 PR457 상세 겹은 뜨지 않는다"),
			HUD->IsDetailOverlayShown());
		UUserWidget* MonsterTab = HUD->GetMonsterTabWidgetForTest();
		UButton* MonsterBack = MonsterTab != nullptr
			? Cast<UButton>(MonsterTab->GetWidgetFromName(TEXT("MonsterBackButton")))
			: nullptr;
		if (TestNotNull(TEXT("몬스터 탭 뒤로 단추"), MonsterBack))
		{
			MonsterBack->OnClicked.Broadcast();
			TestFalse(TEXT("몬스터 뒤로 단추를 누르면 탭이 닫힌다"),
				HUD->IsMonsterTabShown());
		}
	}
	else
	{
		// 탭 WBP가 없는 환경: 예전 계약대로 첫 생존 적의 상세 겹을 연다.
		TestEqual(TEXT("몬스터 메뉴는 첫 생존 적을 InspectUnit으로 연다"),
			DetailResponder->mLastPayload, MonsterUnit.mUnitId);
		TestTrue(TEXT("몬스터 메뉴는 상세 오버레이를 연다"),
			HUD->IsDetailOverlayShown());
	}

	FPlayerMetaUI Meta;
	Meta.mGold = 123456;
	UTexture2D* FirstArtifact =
		NewObject<UTexture2D>(GetTransientPackage());
	UTexture2D* SecondArtifact =
		NewObject<UTexture2D>(GetTransientPackage());
	FCombatArtifactUI& First = Meta.mArtifacts.AddDefaulted_GetRef();
	First.mName = FText::FromString(TEXT("첫 아티팩트"));
	First.mIcon = FirstArtifact;
	First.mEffectDescriptions.Add(FText::FromString(TEXT("첫 효과 설명")));
	FCombatArtifactUI& Second = Meta.mArtifacts.AddDefaulted_GetRef();
	Second.mName = FText::FromString(TEXT("둘째 아티팩트"));
	Second.mIcon = SecondArtifact;
	Second.mEffectDescriptions.Add(FText::FromString(TEXT("둘째 효과 설명")));
	Model->SetPlayerMeta(Meta);

	TestEqual(TEXT("보유 골드는 메타의 현재 값"),
		Gold->GetText().ToString(), FText::AsNumber(Meta.mGold).ToString());
	TestNull(TEXT("실행 HUD에도 독립 아티팩트 줄 없음"),
		HUD->WidgetTree->FindWidget(TEXT("ArtifactStrip")));

	MercenaryMenu->OnClicked.Broadcast();
	TestEqual(TEXT("용병 메뉴를 누르면 패널이 열린다"),
		Panel->GetVisibility(), ESlateVisibility::Visible);
	Meta.mGold = 123457;
	Model->SetPlayerMeta(Meta);
	TestEqual(TEXT("골드가 갱신돼도 열어 둔 용병 패널은 유지된다"),
		Panel->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("열린 패널의 골드도 즉시 바뀐다"),
		Gold->GetText().ToString(), FText::AsNumber(Meta.mGold).ToString());
	Close->OnClicked.Broadcast();
	TestEqual(TEXT("닫기를 누르면 패널이 닫힌다"),
		Panel->GetVisibility(), ESlateVisibility::Collapsed);

	FUnitUI PartyUnit;
	PartyUnit.mUnitId = 777;
	PartyUnit.mIsPlayer = true;
	PartyUnit.mName = FText::FromString(TEXT("상세를 볼 용병"));
	PartyUnit.mHP = 90.f;
	PartyUnit.mMaxHP = 100.f;
	PartyUnit.mActionPoints = 7;
	PartyUnit.mMaxActionPoints = 7;
	PartyUnit.mMovementPoint = 7.f;
	PartyUnit.mMaxMovementPoint = 7.f;
	// 플레이어가 적을 짚어 둔 상태여도 같은 자리를 쓰는 적 요약판이 용병판을
	// 덮으면 안 된다. 요약판의 진영은 현재 턴이 결정한다.
	Model->SetUnitUIs({ PartyUnit, MonsterUnit });
	FCombatTargetUI EnemyTarget;
	EnemyTarget.mIsValid = true;
	EnemyTarget.mUnitId = MonsterUnit.mUnitId;
	Model->SetTarget(EnemyTarget);
	FTurnUI PlayerTurn;
	PlayerTurn.mCurrentUnitId = PartyUnit.mUnitId;
	PlayerTurn.mTurnOrderUnitIds.Add(PartyUnit.mUnitId);
	Model->SetTurnUI(PlayerTurn);
	TestEqual(TEXT("플레이어 차례에는 공용 AP 막대를 보인다"),
		TurnAPRoot->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("플레이어 차례라도 적을 클릭하면 적 요약판을 보인다"),
		EnemyPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("선택한 적 요약판과 용병 요약판은 겹치지 않는다"),
		AllyPanel->GetVisibility(), ESlateVisibility::Collapsed);
	Model->SetTarget(FCombatTargetUI());
	TestEqual(TEXT("적 선택을 풀면 현재 용병 요약판으로 돌아온다"),
		AllyPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("용병 요약판은 현재 턴 용병을 표시한다"),
		AllyName->GetText().ToString(), PartyUnit.mName.ToString());
	MercenaryMenu->OnClicked.Broadcast();
	PartyButton->OnClicked.Broadcast();
	/*
	 * 0806 확정: 목록에서 고르면 **패널 안에서** 오른쪽 상세만 갈린다.
	 *
	 * 전에는 목록을 닫고 상세 겹을 따로 띄웠는데, 같은 내용을 두 판으로
	 * 두 번 보여 주고 목록이 사라지는 흐름이었다.
	 */
	TestEqual(TEXT("용병을 골라도 목록은 열려 있다"),
		Panel->GetVisibility(), ESlateVisibility::Visible);
	TestFalse(TEXT("따로 뜨는 상세 겹은 없다"), HUD->IsDetailOverlayShown());
	if (UTextBlock* DetailName = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("MercenaryDetailName"))))
	{
		TestEqual(TEXT("오른쪽 상세가 고른 용병으로 갈린다"),
			DetailName->GetText().ToString(), PartyUnit.mName.ToString());
	}
	InventoryButton->OnClicked.Broadcast();
	TestEqual(TEXT("인벤토리 탭을 눌러도 용병 패널은 유지된다"),
		Panel->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("인벤토리는 용병 판 내부 페이지로 열린다"),
		InventoryPage->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("인벤토리 페이지에서는 용병 상세를 숨긴다"),
		DetailSection->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("내부 인벤토리에 현재 골드를 표시한다"),
		InventoryGold->GetText().ToString(), FText::AsNumber(Meta.mGold).ToString());
	TestEqual(TEXT("내부 인벤토리에 첫 아티팩트를 표시한다"),
		InventoryArtifact0->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("내부 인벤토리 아티팩트 그림"),
		InventoryArtifact0->GetBrush().GetResourceObject(),
		static_cast<UObject*>(Meta.mArtifacts[0].mIcon.Get()));
	/*
	 * 0809 확정: 설명 띠는 뺐다. 아티팩트 상세는 칸을 눌러 팝업(기존 상세
	 * WBP)으로만 본다 -- 면 안에 선택 상태나 설명을 남기지 않는다.
	 */
	InventoryArtifactButton1->OnClicked.Broadcast();
	TestTrue(TEXT("아티팩트 슬롯을 누르면 기존 상세 WBP가 열린다"),
		HUD->IsDetailOverlayShown());
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("용병 줄을 누르면 인벤토리에서 상세로 돌아간다"),
		InventoryPage->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("용병 상세 구역을 다시 보인다"),
		DetailSection->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDTurnBarStructureTest,
	"P_RD.UI.CombatHUD.TurnBarStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 컴팩트 턴바의 판과 열 칸이 WBP에 함께 구워졌는지 확인한다.
 *
 * 상수만 열로 올리고 WBP가 여섯 칸이면 7~10번째 유닛이 조용히 사라진다.
 * 반대로 WBP만 늘리면 C++이 여섯 칸만 찾는다. 둘을 한 계약으로 묶는다.
 */
bool FCombatHUDTurnBarStructureTest::RunTest(const FString& Parameters)
{
	UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C"));
	if (!TestNotNull(TEXT("HUD 클래스를 찾았다"), HUDClass))
	{
		return false;
	}

	UWidgetBlueprintGeneratedClass* Generated =
		Cast<UWidgetBlueprintGeneratedClass>(HUDClass);
	UWidgetTree* Tree = Generated != nullptr
		? Generated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("위젯 나무"), Tree))
	{
		return false;
	}

	UImage* Plate = Cast<UImage>(Tree->FindWidget(TEXT("TurnPlate")));
	if (TestNotNull(TEXT("구식 긴 턴바 판"), Plate))
	{
		TestEqual(TEXT("구식 긴 턴바 판은 숨김"),
			Plate->GetVisibility(), ESlateVisibility::Collapsed);
	}

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::TurnSlotCount; ++Index)
	{
		const FString TokenName = FString::Printf(TEXT("TurnToken_%d"), Index);
		const FString CropName = FString::Printf(TEXT("TurnPortraitCrop_%d"), Index);
		const FString PortraitName = FString::Printf(TEXT("TurnPortrait_%d"), Index);
		const FString CurrentName = FString::Printf(TEXT("TurnCurrent_%d"), Index);
		const FString DividerName =
			FString::Printf(TEXT("TurnRoundDivider_%d"), Index);
		const FString LabelName =
			FString::Printf(TEXT("TurnRoundLabel_%d"), Index);
		const FString SpeedName =
			FString::Printf(TEXT("TurnSpeed_%d"), Index);
		const FString SpeedIconName =
			FString::Printf(TEXT("TurnSpeedIcon_%d"), Index);
		const FString FrameName =
			FString::Printf(TEXT("TurnFrame_%d"), Index);

		UPanelWidget* Token =
			Cast<UPanelWidget>(Tree->FindWidget(FName(*TokenName)));
		UScaleBox* Crop =
			Cast<UScaleBox>(Tree->FindWidget(FName(*CropName)));
		UWidget* Portrait = Tree->FindWidget(FName(*PortraitName));
		UWidget* Current = Tree->FindWidget(FName(*CurrentName));
		UWidget* Divider = Tree->FindWidget(FName(*DividerName));
		UTextBlock* RoundLabel =
			Cast<UTextBlock>(Tree->FindWidget(FName(*LabelName)));
		UTextBlock* Speed =
			Cast<UTextBlock>(Tree->FindWidget(FName(*SpeedName)));
		UImage* SpeedIcon =
			Cast<UImage>(Tree->FindWidget(FName(*SpeedIconName)));
		UImage* Frame =
			Cast<UImage>(Tree->FindWidget(FName(*FrameName)));
		if (TestNotNull(*TokenName, Token)
			&& TestNotNull(*CropName, Crop)
			&& TestNotNull(*PortraitName, Portrait)
			&& TestNotNull(*CurrentName, Current))
		{
			TestEqual(*FString::Printf(TEXT("%s 부모"), *PortraitName),
				Portrait->GetParent(), static_cast<UPanelWidget*>(Crop));
			TestEqual(*FString::Printf(TEXT("%s 부모"), *CropName),
				Crop->GetParent(), Token);
			TestEqual(*FString::Printf(TEXT("%s 부모"), *CurrentName),
				Current->GetParent(), Token);
		}
		if (TestNotNull(*DividerName, Divider))
		{
			TestEqual(*FString::Printf(TEXT("%s 기본 숨김"), *DividerName),
				Divider->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (TestNotNull(*LabelName, RoundLabel))
		{
			TestEqual(*FString::Printf(TEXT("%s 기본 숨김"), *LabelName),
				RoundLabel->GetVisibility(), ESlateVisibility::Collapsed);
		}
		TestNotNull(*SpeedName, Speed);
		if (TestNotNull(*FrameName, Frame))
		{
			TestEqual(*FString::Printf(TEXT("%s 부모"), *FrameName),
				Frame->GetParent(), Token);
			UTexture2D* Texture =
				Cast<UTexture2D>(Frame->GetBrush().GetResourceObject());
			if (TestNotNull(*FString::Printf(TEXT("%s 텍스처"), *FrameName),
				Texture))
			{
				const FIntPoint ImportedSize = Texture->GetImportedSize();
				TestEqual(*FString::Printf(TEXT("%s 원본 폭"), *FrameName),
					ImportedSize.X, 731);
				TestEqual(*FString::Printf(TEXT("%s 원본 높이"), *FrameName),
					ImportedSize.Y, 995);
			}
		}
		if (TestNotNull(*SpeedIconName, SpeedIcon))
		{
			TestNotNull(*FString::Printf(TEXT("%s 텍스처"), *SpeedIconName),
				Cast<UTexture2D>(SpeedIcon->GetBrush().GetResourceObject()));
		}
	}

	TestNotNull(TEXT("왼쪽 넘김 버튼"),
		Tree->FindWidget(TEXT("TurnPageLeft")));
	TestNotNull(TEXT("오른쪽 넘김 버튼"),
		Tree->FindWidget(TEXT("TurnPageRight")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDTurnBarPagingTest,
	"P_RD.UI.CombatHUD.TurnBarPaging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 현재 라운드 뒤로 다음 라운드를 잇고 미래 슬롯을 반투명으로 그린다.
 */
bool FCombatHUDTurnBarPagingTest::RunTest(const FString& Parameters)
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
	UCombatLayoutHUDWidget* HUD = HUDClass != nullptr
		? CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass) : nullptr;
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->TakeWidget();

	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);

	TArray<FUnitUI> Units;
	FTurnUI Turn;
	UTexture2D* CardPortrait = NewObject<UTexture2D>(HUD);
	UTexture2D* TurnPortrait = NewObject<UTexture2D>(HUD);
	for (int32 Index = 0; Index < 12; ++Index)
	{
		FUnitUI& Unit = Units.AddDefaulted_GetRef();
		Unit.mUnitId = 100 + Index;
		Unit.mName = FText::FromString(
			FString::Printf(TEXT("유닛%d"), Index));
		Unit.mPortrait = CardPortrait;
		Unit.mTurnPortrait = TurnPortrait;
		Unit.mSpeedPoint = 10.f + Index;
		Turn.mTurnOrderUnitIds.Add(Unit.mUnitId);
	}
	// 새 계약(0806): mTurnOrderUnitIds = 이번 라운드 잔여분,
	// mNextRoundUnitIds = 다음 라운드 미리보기. 표기는 거기까지만.
	for (const FUnitUI& Unit : Units)
	{
		Turn.mNextRoundUnitIds.Add(Unit.mUnitId);
	}
	Turn.mCurrentUnitId = Units[0].mUnitId;
	Turn.mRound = 3;
	Turn.mCurrentRoundRemainingTurnCount = 12;
	Model->SetUnitUIs(Units);
	Model->SetTurnUI(Turn);

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::TurnSlotCount; ++Index)
	{
		UWidget* Token = HUD->WidgetTree->FindWidget(FName(
			*FString::Printf(TEXT("TurnToken_%d"), Index)));
		if (TestNotNull(*FString::Printf(TEXT("턴 칸 %d"), Index), Token))
		{
			TestEqual(*FString::Printf(TEXT("턴 칸 %d 표시"), Index),
				Token->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
			TestEqual(*FString::Printf(TEXT("턴 칸 %d 라운드 투명도"), Index),
				Token->GetRenderOpacity(), 1.f);
		}
	}

	UWidget* Current0 = HUD->WidgetTree->FindWidget(TEXT("TurnCurrent_0"));
	if (TestNotNull(TEXT("현재 턴 강조"), Current0))
	{
		TestEqual(TEXT("첫 슬롯만 현재 턴"),
			Current0->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}
	UImage* TurnPortrait0 = Cast<UImage>(
		HUD->WidgetTree->FindWidget(TEXT("TurnPortrait_0")));
	if (TestNotNull(TEXT("턴바 얼굴 초상"), TurnPortrait0))
	{
		TestEqual(TEXT("턴바는 카드 초상이 아닌 전용 얼굴 초상을 사용"),
			TurnPortrait0->GetBrush().GetResourceObject(),
			static_cast<UObject*>(TurnPortrait));
	}
	UTextBlock* TurnSpeed0 = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnSpeed_0")));
	if (TestNotNull(TEXT("턴바 속도"), TurnSpeed0))
	{
		TestEqual(TEXT("첫 턴 유닛 속도 수치 표기"),
			TurnSpeed0->GetText().ToString(), FString(TEXT("10")));
	}

	UButton* Left = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("TurnPageLeft")));
	UButton* Right = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("TurnPageRight")));
	UTextBlock* LeftText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnPageLeftText")));
	UTextBlock* RightText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnPageRightText")));
	if (!TestNotNull(TEXT("왼쪽 넘김"), Left)
		|| !TestNotNull(TEXT("오른쪽 넘김"), Right)
		|| !TestNotNull(TEXT("왼쪽 숨은 수"), LeftText)
		|| !TestNotNull(TEXT("오른쪽 숨은 수"), RightText))
	{
		return false;
	}

	TestEqual(TEXT("첫 창 왼쪽은 없다"), Left->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("첫 창 오른쪽은 누를 수 있다"), Right->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("오른쪽에 열네 순서가 더 있다"),
		RightText->GetText().ToString(), FString(TEXT("14")));

	Right->OnClicked.Broadcast();
	TestEqual(TEXT("둘째 창 왼쪽은 누를 수 있다"), Left->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("둘째 창 오른쪽도 누를 수 있다"), Right->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("둘째 창 왼쪽에 열 순서가 숨었다"),
		LeftText->GetText().ToString(), FString(TEXT("10")));
	TestEqual(TEXT("둘째 창 오른쪽에 네 순서가 더 있다"),
		RightText->GetText().ToString(), FString(TEXT("4")));

	UWidget* Divider2 = HUD->WidgetTree->FindWidget(TEXT("TurnRoundDivider_2"));
	UTextBlock* RoundLabel2 = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("TurnRoundLabel_2")));
	if (TestNotNull(TEXT("다음 라운드 구분선"), Divider2))
	{
		TestEqual(TEXT("다음 라운드 앞 구분선 표시"),
			Divider2->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}
	if (TestNotNull(TEXT("다음 라운드 라벨"), RoundLabel2))
	{
		TestEqual(TEXT("다음 라운드는 간결한 R4"),
			RoundLabel2->GetText().ToString(), FString(TEXT("R4")));
	}
	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::TurnSlotCount; ++Index)
	{
		UWidget* Token = HUD->WidgetTree->FindWidget(FName(
			*FString::Printf(TEXT("TurnToken_%d"), Index)));
		if (TestNotNull(*FString::Printf(TEXT("둘째 창 턴 칸 %d"), Index), Token))
		{
			TestEqual(*FString::Printf(TEXT("둘째 창 %d 라운드 투명도"), Index),
				Token->GetRenderOpacity(), Index < 2 ? 1.f : 0.45f);
		}
	}

	UWidget* RepeatedCurrent =
		HUD->WidgetTree->FindWidget(TEXT("TurnCurrent_2"));
	if (TestNotNull(TEXT("다음 라운드의 같은 유닛 강조"), RepeatedCurrent))
	{
		TestEqual(TEXT("미래의 같은 유닛은 현재 턴이 아니다"),
			RepeatedCurrent->GetVisibility(), ESlateVisibility::Collapsed);
	}

	Right->OnClicked.Broadcast();
	TestEqual(TEXT("마지막 창 왼쪽은 누를 수 있다"), Left->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("마지막 창 오른쪽은 없다"), Right->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("마지막 창 왼쪽에 열네 순서가 숨었다"),
		LeftText->GetText().ToString(), FString(TEXT("14")));

	Left->OnClicked.Broadcast();
	TestEqual(TEXT("마지막에서 왼쪽은 둘째 창으로 이동"),
		LeftText->GetText().ToString(), FString(TEXT("10")));
	TestEqual(TEXT("둘째 창은 오른쪽 페이지가 있다"),
		Right->GetVisibility(), ESlateVisibility::Visible);

	// 턴이 바뀌면 페이지가 0으로 돌아간 뒤 같은 갱신 프레임에 다시 그려진다.
	// 턴 목록은 소비형이다 -- 지나간 턴은 잔여 배열에서 빠진다.
	Turn.mTurnOrderUnitIds.RemoveAt(0);
	Turn.mCurrentUnitId = Units[1].mUnitId;
	Turn.mCurrentRoundRemainingTurnCount = 11;
	Model->SetTurnUI(Turn);
	TestEqual(TEXT("턴 변경 뒤 왼쪽은 다시 없다"), Left->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("턴 변경 뒤 오른쪽 페이지가 다시 열린다"),
		Right->GetVisibility(), ESlateVisibility::Visible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDCardToggleTest,
	"P_RD.UI.CombatHUD.CardToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 아군 칸 버튼이 실제로 묶여 카드를 접었다 폈다 하나.
 *
 * @details
 * 묶는 것을 빠뜨려도 화면은 똑같이 나오고 누르면 아무 일이 없다. 눈으로는
 * "입력이 안 온다" 와 구분이 안 된다. 이 시험이 통과하는데 게임에서 안 되면
 * 원인은 배선이 아니라 입력이 버튼까지 안 온다는 뜻이다.
 */
bool FCombatHUDCardToggleTest::RunTest(const FString& Parameters)
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

	UCombatLayoutHUDWidget* HUD =
		CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	// 위젯 캐시와 버튼 묶기는 NativeConstruct 에서 일어난다.
	HUD->TakeWidget();

	UButton* PartyButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("PartyButton_0")));
	if (!TestNotNull(TEXT("아군 칸 버튼"), PartyButton))
	{
		return false;
	}
	TestTrue(TEXT("아군 칸 버튼에 무언가 묶여 있다"),
		PartyButton->OnClicked.IsBound());

	// 새 계약(0807)은 모델이 있어야 굴러간다 -- 시험이 직접 구성한다.
	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);
	FUnitUI PlayerUnit;
	PlayerUnit.mUnitId = 101;
	PlayerUnit.mIsPlayer = true;
	Model->SetUnitUIs({ PlayerUnit });
	FTurnUI PlayerTurn;
	PlayerTurn.mCurrentUnitId = PlayerUnit.mUnitId;
	PlayerTurn.mTurnOrderUnitIds.Add(PlayerUnit.mUnitId);
	Model->SetTurnUI(PlayerTurn);
	Model->OnBeginAnyTurn.Broadcast(nullptr);

	UWidget* Card = HUD->WidgetTree->FindWidget(TEXT("CommandCard_0"));
	if (!TestNotNull(TEXT("명령 카드"), Card))
	{
		return false;
	}

	// 아군 칸은 뒤집기가 아니라 여는 손이다.
	//
	// 차례가 와도 카드는 저절로 안 열린다(0807). 아군 칸을 누르면 그 용병의
	// 카드가 펴지고, 몇 번을 눌러도 접히지 않는다 -- 스킬을 보러 눌렀는데
	// 접히면 아무 일도 안 일어난 것처럼 보인다.
	TestEqual(TEXT("누르기 전에는 접혀 있다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("누르면 카드가 펴진다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("다시 눌러도 접히지 않는다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	Model->OnEndAnyTurn.Broadcast(nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHUDSkillLifecycleTest,
	"P_RD.UI.CombatHUD.SkillLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 스킬 카드는 플레이어의 실제 입력 구간에만 보인다.
 *
 * @details
 * 턴 종료 뒤 TurnUI가 이전 플레이어를 잠시 가리키는 동안 카드가 남았고,
 * BuildAction 종료와 SkillAction 시작 사이에도 한 프레임 다시 나타났다.
 * 턴/액션 표시 알림을 직접 흘려 두 회귀를 함께 막는다.
 */
bool FCombatHUDSkillLifecycleTest::RunTest(const FString& Parameters)
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

	UCombatLayoutHUDWidget* HUD =
		CreateWidget<UCombatLayoutHUDWidget>(World, HUDClass);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	HUD->TakeWidget();

	// WBP 기본값은 실제 전투용이라 preview model을 만들지 않을 수 있다.
	// 시험이 런타임 모델 연결과 같은 경로를 직접 구성한다.
	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);

	FUnitUI PlayerUnit;
	PlayerUnit.mUnitId = 101;
	PlayerUnit.mIsPlayer = true;
	Model->SetUnitUIs({ PlayerUnit });

	FTurnUI PlayerTurn;
	PlayerTurn.mCurrentUnitId = PlayerUnit.mUnitId;
	PlayerTurn.mTurnOrderUnitIds.Add(PlayerUnit.mUnitId);
	Model->SetTurnUI(PlayerTurn);
	Model->OnBeginAnyTurn.Broadcast(nullptr);

	UWidget* Card = HUD->WidgetTree->FindWidget(TEXT("CommandCard_0"));
	UButton* SkillButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("SkillToggleButton")));
	if (!TestNotNull(TEXT("전투 UI 모델"), Model)
		|| !TestNotNull(TEXT("명령 카드"), Card)
		|| !TestNotNull(TEXT("스킬 단추"), SkillButton))
	{
		return false;
	}

	// 새 계약(0807): 차례가 와도 카드는 저절로 안 열린다. 스킬 단추가 정문.
	TestEqual(TEXT("플레이어 턴에도 저절로 펴지지 않는다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);
	SkillButton->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 단추로 편다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);

	Model->OnEndAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("턴 종료 알림 즉시 카드를 내린다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);

	Model->OnBeginAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("다음 턴 시작에도 저절로 펴지지 않는다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);
	SkillButton->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 단추로 다시 편다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);

	Model->OnBeginAnyTurnAction.Broadcast(nullptr);
	TestEqual(TEXT("행동 시작부터 카드를 감춘다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);

	Model->OnEndAnyTurnAction.Broadcast(nullptr);
	TestEqual(TEXT("행동 종료와 후속 행동 사이에는 즉시 다시 보이지 않는다"),
		Card->GetVisibility(), ESlateVisibility::Collapsed);

	// 다음 틱 예약이 시험 뒤 다른 상태를 건드리지 않게 턴 종료로 무효화한다.
	Model->OnEndAnyTurn.Broadcast(nullptr);
	return true;
}

#endif
