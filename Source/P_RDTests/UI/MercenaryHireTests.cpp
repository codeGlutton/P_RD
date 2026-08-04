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

	/** @brief 하나를 정한다. 한 번이면 된다. */
	void Choose(UMercenaryHireWidget& Board, const int32 CardIndex)
	{
		Board.ClickCard(CardIndex);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMercenaryHireChooseTest,
	"P_RD.UI.MercenaryHire.Choose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMercenaryHireChooseTest::RunTest(const FString& Parameters)
{
	UMercenaryHireWidget* Board = MakeBoard();

	// 한 번 누르면 정해진다.
	Board->ClickCard(2);
	TestEqual(TEXT("한 번 누르면 정해짐"), Board->StateOf(2),
		EMercenaryCardState::Chosen);
	TestEqual(TEXT("한 명 정해짐"), Board->GetChosenIndices().Num(), 1);

	// 정해진 것을 또 누르면 풀린다.
	Board->ClickCard(2);
	TestEqual(TEXT("또 누르면 풀린다"), Board->GetChosenIndices().Num(), 0);
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

	// 자리가 찼는데 새 후보를 누르면 마지막 자리를 내준다.
	TestEqual(TEXT("고르기 전 마지막 자리는 5"),
		Board->GetChosenIndices()[2], 5);
	Choose(*Board, 1);
	TestEqual(TEXT("여전히 셋"), Board->GetChosenIndices().Num(), 3);
	TestEqual(TEXT("마지막 자리가 새 후보로"),
		Board->GetChosenIndices()[2], 1);
	TestFalse(TEXT("내준 후보는 빠져 있다"),
		Board->GetChosenIndices().Contains(5));
	TestEqual(TEXT("먼저 고른 둘은 그대로"),
		Board->GetChosenIndices()[0], 0);

	// 내준 자리는 다시 고를 수 있다. 이번엔 방금 들어온 1 이 밀려난다.
	Choose(*Board, 5);
	TestEqual(TEXT("바뀐 뒤에도 셋"), Board->GetChosenIndices().Num(), 3);
	TestTrue(TEXT("다시 고른 후보가 들어와 있다"),
		Board->GetChosenIndices().Contains(5));
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
	TestTrue(TEXT("실데이터가 없어도 여섯째 드루이드를 검수 선택할 수 있다"),
		Board->GetChosenIndices().Contains(5));

	UImage* Hero = Cast<UImage>(Board->WidgetTree->FindWidget(TEXT("Backdrop_Art")));
	if (TestNotNull(TEXT("선택 영웅 일러스트"), Hero))
	{
		TestEqual(TEXT("드루이드 선택은 드루이드 일러스트를 건다"),
			Hero->GetBrush().GetResourceObject()->GetPathName(),
			FString(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"
				"T_MB_HireHero_Druid.T_MB_HireHero_Druid")));
	}

	Board->ClickCard(1);
	Board->ClickCard(2);
	UTextBlock* RangerName = Cast<UTextBlock>(
		Board->WidgetTree->FindWidget(TEXT("HireName_2")));
	if (TestNotNull(TEXT("세 번째 행 이름"), RangerName))
	{
		TestEqual(TEXT("Archer placeholder의 옛 도적명이 아닌 레인저 표시"),
			RangerName->GetText().ToString(), FString(TEXT("레인저")));
	}
	TestTrue(TEXT("실데이터가 없는 마법사도 검수 선택 가능"),
		Board->GetChosenIndices().Contains(1));
	TestTrue(TEXT("Archer placeholder 대신 레인저도 검수 선택 가능"),
		Board->GetChosenIndices().Contains(2));

	UWidget* MageReviewFrame = Board->WidgetTree->FindWidget(TEXT("HireSelected_1"));
	UWidget* RangerReviewFrame = Board->WidgetTree->FindWidget(TEXT("HireSelected_2"));
	UWidget* DruidReviewFrame = Board->WidgetTree->FindWidget(TEXT("HireSelected_5"));
	if (TestNotNull(TEXT("마법사 파란 검토 테두리"), MageReviewFrame)
		&& TestNotNull(TEXT("레인저 파란 검토 테두리"), RangerReviewFrame)
		&& TestNotNull(TEXT("드루이드 파란 검토 테두리"), DruidReviewFrame))
	{
		TestEqual(TEXT("현재 보고 있는 레인저만 파란 테두리"),
			RangerReviewFrame->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		TestEqual(TEXT("파티원이어도 현재 보지 않는 마법사는 파란 테두리 없음"),
			MageReviewFrame->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("파티원이어도 현재 보지 않는 드루이드는 파란 테두리 없음"),
			DruidReviewFrame->GetVisibility(), ESlateVisibility::Collapsed);
	}

	UWidget* MagePartySeal = Board->WidgetTree->FindWidget(TEXT("HireSeal_1"));
	UWidget* RangerPartySeal = Board->WidgetTree->FindWidget(TEXT("HireSeal_2"));
	UWidget* DruidPartySeal = Board->WidgetTree->FindWidget(TEXT("HireSeal_5"));
	if (TestNotNull(TEXT("마법사 빨간 파티 인장"), MagePartySeal)
		&& TestNotNull(TEXT("레인저 빨간 파티 인장"), RangerPartySeal)
		&& TestNotNull(TEXT("드루이드 빨간 파티 인장"), DruidPartySeal))
	{
		TestEqual(TEXT("마법사 파티 인장 유지"),
			MagePartySeal->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		TestEqual(TEXT("레인저 파티 인장 유지"),
			RangerPartySeal->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		TestEqual(TEXT("드루이드 파티 인장 유지"),
			DruidPartySeal->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}

	UButton* PartySlotButton = Cast<UButton>(
		Board->WidgetTree->FindWidget(TEXT("PartySlotButton_1")));
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
	if (TestNotNull(TEXT("세로 카드 0 슬롯"), Card0Slot)
		&& TestNotNull(TEXT("세로 카드 1 슬롯"), Card1Slot)
		&& TestNotNull(TEXT("세로 카드 2 슬롯"), Card2Slot))
	{
		TestEqual(TEXT("세로에서 0·1번은 같은 행"),
			Card0Slot->GetPosition().Y, Card1Slot->GetPosition().Y);
		TestTrue(TEXT("세로에서 1번은 0번 오른쪽"),
			Card1Slot->GetPosition().X > Card0Slot->GetPosition().X);
		TestTrue(TEXT("세로에서 2번은 다음 행"),
			Card2Slot->GetPosition().Y > Card0Slot->GetPosition().Y);
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
		UWidget* Label = Tree->FindWidget(FName(Name));
		if (TestNotNull(*FString::Printf(TEXT("%s 유지"), Name), Label))
		{
			TestEqual(*FString::Printf(TEXT("%s 텍스트 숨김"), Name),
				Label->GetVisibility(), ESlateVisibility::Collapsed);
		}
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
		TestEqual(TEXT("용병 패널 뒤로 문구"),
			BackText->GetText().ToString(), FString(TEXT("뒤로")));
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
	if (UWidget* MercenaryScrim =
		Tree->FindWidget(TEXT("MercenaryScrim")))
	{
		TestEqual(TEXT("새 용병 셸을 덮던 구식 암막은 WBP에서 숨김"),
			MercenaryScrim->GetVisibility(), ESlateVisibility::Collapsed);
	}
	else
	{
		AddError(TEXT("MercenaryScrim을 찾을 수 없다"));
	}

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
			TestEqual(*FString::Printf(TEXT("%s 는 용병 판 안"), *ScaleName),
				Scale->GetParent(), MercenaryBoard);
			TestEqual(*FString::Printf(TEXT("%s 는 크기 래퍼 안"), *CardName),
				Card->GetParent(), Scale);
		}
	}

	UPanelWidget* TurnAPScale =
		Cast<UPanelWidget>(Tree->FindWidget(TEXT("TurnAPScale")));
	UWidget* TurnAPPanel = Tree->FindWidget(TEXT("TurnAPPanel"));
	if (TestNotNull(TEXT("좌하단 AP 크기 래퍼"), TurnAPScale)
		&& TestNotNull(TEXT("좌하단 AP 판"), TurnAPPanel))
	{
		TestEqual(TEXT("AP 판은 크기 래퍼 안"),
			TurnAPPanel->GetParent(), TurnAPScale);
	}

	UPanelWidget* ArtifactStrip =
		Cast<UPanelWidget>(Tree->FindWidget(TEXT("ArtifactStrip")));
	if (TestNotNull(TEXT("AP 위 아티팩트 줄"), ArtifactStrip))
	{
		UWidget* StripPlate = Tree->FindWidget(TEXT("ArtifactStripPlate"));
		UWidget* StripLabel = Tree->FindWidget(TEXT("ArtifactStripLabel"));
		if (TestNotNull(TEXT("옛 아티팩트 배경판"), StripPlate))
		{
			TestEqual(TEXT("아티팩트 배경판은 숨김"),
				StripPlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (TestNotNull(TEXT("옛 아티팩트 제목"), StripLabel))
		{
			TestEqual(TEXT("아티팩트 제목은 숨김"),
				StripLabel->GetVisibility(), ESlateVisibility::Collapsed);
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FString Name =
				FString::Printf(TEXT("ArtifactIcon_%d"), Index);
			const FString FrameName =
				FString::Printf(TEXT("ArtifactFrame_%d"), Index);
			UImage* Icon =
				Cast<UImage>(Tree->FindWidget(FName(*Name)));
			UWidget* Frame = Tree->FindWidget(FName(*FrameName));
			if (TestNotNull(*Name, Icon))
			{
				TestEqual(*FString::Printf(TEXT("%s 는 아티팩트 줄 안"), *Name),
					Icon->GetParent(), ArtifactStrip);
			}
			if (TestNotNull(*FrameName, Frame))
			{
				TestEqual(*FString::Printf(TEXT("%s 는 숨김"), *FrameName),
					Frame->GetVisibility(), ESlateVisibility::Collapsed);
			}
		}
	}

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
			TestEqual(TEXT("몬스터 탭 뒤로 문구"),
				BackText->GetText().ToString(), FString(TEXT("뒤로")));
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
		TestEqual(TEXT("프리미엄 셸 오른쪽 아래 앵커"),
			ShellSlot->GetAnchors().Maximum, FVector2D(1.f, 1.f));
		TestEqual(TEXT("프리미엄 셸은 패널을 빈틈없이 채운다"),
			ShellSlot->GetOffsets(), FMargin(0.f));
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
		if (UWidget* LegacyPlate = HUD->WidgetTree->FindWidget(FName(Name)))
		{
			TestEqual(*FString::Printf(TEXT("%s 구식 판은 숨김"), Name),
				LegacyPlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
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
	Model->SetUnitUIs({ MonsterUnit });
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
	FCombatArtifactUI& Second = Meta.mArtifacts.AddDefaulted_GetRef();
	Second.mName = FText::FromString(TEXT("둘째 아티팩트"));
	Second.mIcon = SecondArtifact;
	Model->SetPlayerMeta(Meta);

	TestEqual(TEXT("보유 골드는 메타의 현재 값"),
		Gold->GetText().ToString(), FText::AsNumber(Meta.mGold).ToString());
	for (int32 Index = 0; Index < 6; ++Index)
	{
		UImage* Icon = Cast<UImage>(HUD->WidgetTree->FindWidget(FName(
			*FString::Printf(TEXT("ArtifactIcon_%d"), Index))));
		if (!TestNotNull(*FString::Printf(TEXT("아티팩트 칸 %d"), Index), Icon))
		{
			continue;
		}

		const bool bFilled = Index < Meta.mArtifacts.Num();
		TestEqual(*FString::Printf(TEXT("아티팩트 칸 %d 표시"), Index),
			Icon->GetVisibility(), bFilled
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		if (bFilled)
		{
			TestEqual(*FString::Printf(TEXT("아티팩트 칸 %d 그림"), Index),
				Icon->GetBrush().GetResourceObject(),
				static_cast<UObject*>(Meta.mArtifacts[Index].mIcon.Get()));
		}
	}

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
	Model->SetUnitUIs({ PartyUnit });
	MercenaryMenu->OnClicked.Broadcast();
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("용병을 고르면 목록이 닫힌다"),
		Panel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("용병 카드는 InspectUnit 요청을 보낸다"),
		DetailResponder->mLastType, ECombatInputType::InspectUnit);
	TestEqual(TEXT("카드의 실제 UnitId를 보낸다"),
		DetailResponder->mLastPayload, PartyUnit.mUnitId);
	TestEqual(TEXT("몬스터와 용병 InspectUnit은 각각 한 번 요청된다"),
		DetailResponder->mInspectRequestCount, 2);
	TestEqual(TEXT("상세 응답도 같은 UnitId다"),
		Model->GetUnitDetail().mUnitId, PartyUnit.mUnitId);
	TestTrue(TEXT("동기 상세 응답 뒤 PR457 오버레이가 열린다"),
		HUD->IsDetailOverlayShown());
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

	UWidget* Card = HUD->WidgetTree->FindWidget(TEXT("CommandCard_0"));
	if (!TestNotNull(TEXT("명령 카드"), Card))
	{
		return false;
	}

	// 아군 칸은 뒤집기가 아니다.
	//
	// 전에는 누를 때마다 카드를 접었다 폈다 했다. 지금은 "누구의 스킬을 볼지"
	// 고르는 자리이고, 접고 펴는 것은 판 탭이 맡는다. 그래서 몇 번을 눌러도
	// 카드는 펴져 있어야 한다 -- 스킬을 보러 눌렀는데 접히면 아무 일도 안
	// 일어난 것처럼 보인다.
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("누르면 카드가 펴진다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("다시 눌러도 접히지 않는다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
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
	if (!TestNotNull(TEXT("전투 UI 모델"), Model)
		|| !TestNotNull(TEXT("명령 카드"), Card))
	{
		return false;
	}

	TestEqual(TEXT("플레이어 턴에는 카드가 보인다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);

	Model->OnEndAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("턴 종료 알림 즉시 카드를 내린다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);

	Model->OnBeginAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("다음 플레이어 턴 시작에 다시 보인다"), Card->GetVisibility(),
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
