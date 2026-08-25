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
#include "UI/Combat/CombatUITypes.h"
#include "UI/Combat/SkillDetailOverlayPresenter.h"
#include "UI/Combat/SkillDetailUIBuilder.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
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

bool UMercenaryDetailTestWidget::Initialize()
{
	const bool bWasInitialized = Super::Initialize();
	if (WidgetTree == nullptr)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	if (WidgetTree->RootWidget != nullptr)
	{
		return bWasInitialized;
	}

	// CreateWidget가 반환되기 전에 최소 named tree를 만들어, HUD의 실제
	// EnsureDetailOverlayWidget 캐시 순서를 WBP와 동일하게 시험한다.
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("DetailPanelRoot"));
	WidgetTree->RootWidget = Root;
	auto AddText = [this, Root](const FName Name)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), Name);
		Root->AddChild(Text);
	};
	AddText(TEXT("DetailTitleText"));
	AddText(TEXT("DetailSubtitleText"));
	AddText(TEXT("DetailBodyText"));
	for (int32 Index = 0; Index < 5; ++Index)
	{
		AddText(FName(*FString::Printf(TEXT("DetailChip%dLabel"), Index)));
		AddText(FName(*FString::Printf(TEXT("DetailChip%dValue"), Index)));
	}
	return bWasInitialized;
}

void UMercenaryDetailTestResponder::Bind(UCombatUIModel* UIModel)
{
	mUIModel = UIModel;
	if (mUIModel != nullptr)
	{
		mUIModel->OnCombatCommand.AddUniqueDynamic(
			this, &UMercenaryDetailTestResponder::HandleCombatCommand);
	}
}

void UMercenaryDetailTestResponder::ConfigureUnitSkillResponse(
	const FSkillDetailUI& SkillDetail)
{
	mResponseSkillDetail = SkillDetail;
}

void UMercenaryDetailTestResponder::HandleCombatCommand(
	const ECombatInputType Type, const int32 IntPayload)
{
	mLastType = Type;
	mLastPayload = IntPayload;
	if (mUIModel == nullptr)
	{
		return;
	}
	if (Type == ECombatInputType::InspectUnitSkill
		|| Type == ECombatInputType::LongPressSkill)
	{
		++mInspectSkillRequestCount;
		if (IntPayload == mResponseSkillDetail.mSkillIndex)
		{
			mUIModel->SetSkillDetail(mResponseSkillDetail);
		}
		return;
	}
	if (Type != ECombatInputType::InspectUnit)
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
	if (mResponseSkillDetail.mSkillIndex != INDEX_NONE)
	{
		FUnitDetailSkillUI& Skill = Detail.mSkills.AddDefaulted_GetRef();
		Skill.mSkillIndex = mResponseSkillDetail.mSkillIndex;
		Skill.mName = mResponseSkillDetail.mName;
	}
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
	TestEqual(TEXT("목록 클릭만으로는 편성되지 않음"),
		Board->GetChosenIndices().Num(), 0);
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
	// 한글 표시 문자열을 단언하므로 ko 컬처로 고정한다. en/ko 번역이 모두
	// 채워진 뒤로는 실행 컬처에 따라 표시가 달라진다(0823).
	struct FScopedKoreanCulture
	{
		FString mOriginal;
		FScopedKoreanCulture()
			: mOriginal(FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
		}
		~FScopedKoreanCulture()
		{
			FInternationalization::Get().SetCurrentCulture(mOriginal);
		}
	};
	const FScopedKoreanCulture ScopedKoreanCulture;
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

	UWidgetBlueprintGeneratedClass* HireGenerated =
		Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	UWidgetTree* HireTree = HireGenerated != nullptr
		? HireGenerated->GetWidgetTreeArchetype() : nullptr;
	if (!TestNotNull(TEXT("Marchbound 용병 선택 WBP 나무"), HireTree))
	{
		return false;
	}
	UWidget* HireTitlePanel = HireTree->FindWidget(TEXT("HireTitlePanel"));
	UWidget* HireDetailNamePanel = HireTree->FindWidget(TEXT("HireDetailNamePanel"));
	UWidget* HireStatsPanel = HireTree->FindWidget(TEXT("HireDetailStatsPanel"));
	if (TestNotNull(TEXT("중앙 용병 선택 제목판"), HireTitlePanel))
	{
		TestEqual(TEXT("용병 선택 제목판은 일러스트를 가리지 않음"),
			HireTitlePanel->GetVisibility(), ESlateVisibility::Collapsed);
	}
	if (TestNotNull(TEXT("중앙 클래스명 판"), HireDetailNamePanel))
	{
		TestEqual(TEXT("중앙 클래스명 판은 목록과 중복되어 숨김"),
			HireDetailNamePanel->GetVisibility(), ESlateVisibility::Collapsed);
	}
	UOverlay* HireViewportRoot = Cast<UOverlay>(
		HireTree->FindWidget(TEXT("HireViewportRoot")));
	UBorder* HireAmbientFill = Cast<UBorder>(
		HireTree->FindWidget(TEXT("HireBackgroundLetterboxFill")));
	UScaleBox* HireGeneratedScale = Cast<UScaleBox>(
		HireTree->FindWidget(TEXT("HireGeneratedBackgroundScale")));
	UImage* HireGeneratedArt = Cast<UImage>(
		HireTree->FindWidget(TEXT("HireGeneratedBackgroundArt")));
	UScaleBox* HireBackgroundScale = Cast<UScaleBox>(
		HireTree->FindWidget(TEXT("HireBackgroundScale")));
	UImage* HireBackgroundArt = Cast<UImage>(
		HireTree->FindWidget(TEXT("Backdrop_Art")));
	if (TestNotNull(TEXT("용병 선택 배경 Overlay"), HireViewportRoot)
		&& TestNotNull(TEXT("생성 배경 안전 바탕"), HireAmbientFill)
		&& TestNotNull(TEXT("직업별 생성 배경 ScaleBox"), HireGeneratedScale)
		&& TestNotNull(TEXT("직업별 생성 배경 그림"), HireGeneratedArt)
		&& TestNotNull(TEXT("높이 맞춤 ScaleBox"), HireBackgroundScale)
		&& TestNotNull(TEXT("전경 용병 일러스트"), HireBackgroundArt))
	{
		TestEqual(TEXT("직업별 생성 배경은 화면을 채움"),
			HireGeneratedScale->GetStretch(), EStretch::ScaleToFill);
		TestEqual(TEXT("전경 일러스트는 상하 전체가 보이는 fit"),
			HireBackgroundScale->GetStretch(), EStretch::ScaleToFit);
		TestEqual(TEXT("전경은 양방향 배율 허용"),
			HireBackgroundScale->GetStretchDirection(), EStretchDirection::Both);
		TestEqual(TEXT("생성 배경 그림은 ScaleBox 직계 자식"),
			HireGeneratedArt->GetParent(), static_cast<UPanelWidget*>(HireGeneratedScale));
		TestEqual(TEXT("전경 그림은 fit ScaleBox 직계 자식"),
			HireBackgroundArt->GetParent(), static_cast<UPanelWidget*>(HireBackgroundScale));
		const int32 FillIndex = HireViewportRoot->GetChildIndex(HireAmbientFill);
		const int32 GeneratedIndex = HireViewportRoot->GetChildIndex(HireGeneratedScale);
		const int32 ForegroundIndex = HireViewportRoot->GetChildIndex(HireBackgroundScale);
		TestTrue(TEXT("안전 바탕→생성 배경→전경 순서"),
			FillIndex >= 0 && GeneratedIndex > FillIndex
			&& ForegroundIndex > GeneratedIndex);
		TestNull(TEXT("옛 확대 앰비언트 위젯은 제거"),
			HireTree->FindWidget(TEXT("HireBackgroundAmbientScale")));
		TestNull(TEXT("옛 단계 페더 위젯은 제거"),
			HireTree->FindWidget(TEXT("HireBackgroundEdgeFade")));
	}
	if (TestNotNull(TEXT("중앙 스탯 띠"), HireStatsPanel))
	{
		if (UCanvasPanelSlot* StatsSlot = Cast<UCanvasPanelSlot>(HireStatsPanel->Slot))
		{
			TestEqual(TEXT("스탯 띠 위치"), StatsSlot->GetPosition(),
				FVector2D(122.5f, 728.0f));
			TestEqual(TEXT("스탯 띠 크기"), StatsSlot->GetSize(),
				FVector2D(600.0f, 84.0f));
		}
		else
		{
			AddError(TEXT("스탯 띠는 중앙 Canvas 슬롯이어야 함"));
		}
	}
	for (const TCHAR* Name : { TEXT("HireDetailHP"), TEXT("HireDetailAP"),
		TEXT("HireDetailSpeed") })
	{
		if (UTextBlock* StatText = Cast<UTextBlock>(HireTree->FindWidget(Name)))
		{
			TestEqual(*FString::Printf(TEXT("%s 글꼴 크기"), Name),
				// 빌더 기준 크기를 모바일 전용 확대 없이 그대로 저장한다.
				StatText->GetFont().Size, 24.0f);
		}
		else
		{
			AddError(FString::Printf(TEXT("%s 스탯 글자 없음"), Name));
		}
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FString Tail = FString::Printf(TEXT("_%d"), Index);
		UCanvasPanel* SkillPanel = Cast<UCanvasPanel>(
			HireTree->FindWidget(FName(*(FString(TEXT("HireDetailSkill")) + Tail))));
		UScaleBox* SkillFit = Cast<UScaleBox>(
			HireTree->FindWidget(FName(*(FString(TEXT("HireDetailSkillText")) + Tail + TEXT("_Fit")))));
		UTextBlock* SkillText = Cast<UTextBlock>(
			HireTree->FindWidget(FName(*(FString(TEXT("HireDetailSkillText")) + Tail))));
		UImage* SkillIcon = Cast<UImage>(
			HireTree->FindWidget(FName(*(FString(TEXT("HireDetailSkillIcon")) + Tail))));
		UPanelWidget* SkillIconMount = Cast<UPanelWidget>(
			HireTree->FindWidget(FName(*(FString(TEXT("HireDetailSkillIconMount")) + Tail))));
		if (!TestNotNull(*FString::Printf(TEXT("스킬 %d 패널"), Index), SkillPanel)
			|| !TestNotNull(*FString::Printf(TEXT("스킬 %d 텍스트 축소 래퍼"), Index), SkillFit)
			|| !TestNotNull(*FString::Printf(TEXT("스킬 %d 텍스트"), Index), SkillText)
			|| !TestNotNull(*FString::Printf(TEXT("스킬 %d 아이콘"), Index), SkillIcon)
			|| !TestNotNull(*FString::Printf(TEXT("스킬 %d 아이콘 래퍼"), Index), SkillIconMount))
		{
			return false;
		}

		TestEqual(*FString::Printf(TEXT("스킬 %d 패널 경계 클립"), Index),
			SkillPanel->GetClipping(), EWidgetClipping::ClipToBoundsAlways);
		TestTrue(*FString::Printf(TEXT("스킬 %d 텍스트는 ScaleBox 자식"), Index),
			SkillText->GetParent() == SkillFit);
		TestEqual(*FString::Printf(TEXT("스킬 %d 넘침 시에만 축소"), Index),
			SkillFit->GetStretchDirection(), EStretchDirection::DownOnly);
		TestEqual(*FString::Printf(TEXT("스킬 %d 축소 래퍼 경계 클립"), Index),
			SkillFit->GetClipping(), EWidgetClipping::ClipToBoundsAlways);
		if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(SkillText->Slot))
		{
			TestEqual(*FString::Printf(TEXT("스킬 %d 텍스트 가로 중앙"), Index),
				TextSlot->GetHorizontalAlignment(), HAlign_Center);
			TestEqual(*FString::Printf(TEXT("스킬 %d 텍스트 세로 중앙"), Index),
				TextSlot->GetVerticalAlignment(), VAlign_Center);
		}
		else
		{
			AddError(FString::Printf(TEXT("스킬 %d 텍스트 슬롯은 ScaleBoxSlot이어야 함"), Index));
		}
		TestEqual(*FString::Printf(TEXT("스킬 %d 수동 좌표 보정 없음"), Index),
			SkillText->GetRenderTransform().Translation, FVector2D::ZeroVector);
		TestTrue(*FString::Printf(TEXT("스킬 %d 아이콘은 전용 래퍼 자식"), Index),
			SkillIcon->GetParent() == SkillIconMount);
		TestEqual(*FString::Printf(TEXT("스킬 %d 기본 아이콘은 숨김"), Index),
			SkillIcon->GetVisibility(), ESlateVisibility::Collapsed);
	}

	HireTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			TestEqual(*FString::Printf(TEXT("%s 수동 Render Translation 없음"),
				*Text->GetName()), Text->GetRenderTransform().Translation,
				FVector2D::ZeroVector);
		}
	});

	static const TCHAR* MercenaryNames[6] = {
		TEXT("Knight"), TEXT("Mage"), TEXT("Ranger"),
		TEXT("Rogue"), TEXT("Barbarian"), TEXT("Druid")
	};
	for (const TCHAR* Name : MercenaryNames)
	{
		const FString IconPath = FString::Printf(
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"
				"T_MB_HireIcon_%s.T_MB_HireIcon_%s"), Name, Name);
		UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, *IconPath);
		if (TestNotNull(*IconPath, IconTexture))
		{
			TestEqual(*FString::Printf(TEXT("%s 원본 폭"), *IconPath),
				IconTexture->GetImportedSize().X, 1254);
			TestEqual(*FString::Printf(TEXT("%s 원본 높이"), *IconPath),
				IconTexture->GetImportedSize().Y, 1254);
		}

		const FString HeroCutoutPath = FString::Printf(
			TEXT("/Game/UI/MercenaryHire/HeroCutouts/"
				"T_HireHeroCutout_%s_v1.T_HireHeroCutout_%s_v1"), Name, Name);
		UTexture2D* HeroCutout = LoadObject<UTexture2D>(nullptr, *HeroCutoutPath);
		if (TestNotNull(*HeroCutoutPath, HeroCutout))
		{
			// HasAlphaChannel()은 현재 만들어진 GPU 포맷을 묻는 함수라 NullRHI
			// 자동화에서는 플랫폼 데이터가 없어 항상 false가 될 수 있다. 원본
			// PNG가 복잡한 알파로 임포트됐고 알파 압축을 끄지 않았다는 직렬화
			// 계약을 검사하면 실제 Android cook의 투명도 보존을 정확히 잠근다.
			TestFalse(*FString::Printf(TEXT("%s는 알파 압축 제거를 금지"),
				*HeroCutoutPath), HeroCutout->CompressionNoAlpha);
			TestEqual(*FString::Printf(TEXT("%s UI 텍스처 그룹"),
				*HeroCutoutPath), HeroCutout->LODGroup, TEXTUREGROUP_UI);
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
	KnightOnly[0].mJobType = EUnitJobType::Knight;
	KnightOnly[0].mRoleText = FText::FromString(TEXT("방패 탱커 · 근접"));
	// 실제 플레이어 유닛 DA와 같은 다섯 스킬. 이동은 DA 목록 밖의 별도
	// 커맨드라 위젯이 여섯 번째 표시 칸을 앞에 보태야 한다.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FText SkillName = FText::FromString(
			FString::Printf(TEXT("검수 스킬 %d"), Index));
		KnightOnly[0].mSkillNames.Add(SkillName);
		KnightOnly[0].mSkillIcons.Add(nullptr);
		FFrontendSkillOption& Detail =
			KnightOnly[0].mSkillDetails.AddDefaulted_GetRef();
		Detail.mName = SkillName;
		Detail.mDescription = FText::FromString(
			FString::Printf(TEXT("검수 설명 %d"), Index));
		Detail.mActionPointCost = Index + 1;
		Detail.mAimRange = Index + 1;
		Detail.mEffectArea = 1;
	}
	Board->SetCharacterOptions(KnightOnly, 3);
	UTexture2D* ExpectedKnightHero = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/"
			"T_HireHeroCutout_Knight_v1.T_HireHeroCutout_Knight_v1"));
	UImage* RuntimeForegroundArt = Cast<UImage>(
		Board->WidgetTree->FindWidget(TEXT("Backdrop_Art")));
	UTexture2D* ExpectedKnightBackground = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/"
			"T_HireGeneratedBG_Knight_v1.T_HireGeneratedBG_Knight_v1"));
	UImage* RuntimeGeneratedArt = Cast<UImage>(
		Board->WidgetTree->FindWidget(TEXT("HireGeneratedBackgroundArt")));
	if (TestNotNull(TEXT("런타임 기사 전신 원화"), ExpectedKnightHero)
		&& TestNotNull(TEXT("런타임 기사 생성 배경"), ExpectedKnightBackground)
		&& TestNotNull(TEXT("런타임 높이 맞춤 전경"), RuntimeForegroundArt)
		&& TestNotNull(TEXT("런타임 직업별 생성 배경"), RuntimeGeneratedArt))
	{
		UObject* ForegroundResource =
			RuntimeForegroundArt->GetBrush().GetResourceObject();
		UObject* GeneratedResource = RuntimeGeneratedArt->GetBrush().GetResourceObject();
		TestEqual(TEXT("SetCharacterOptions는 전경을 현재 Hero로 갱신"),
			ForegroundResource, static_cast<UObject*>(ExpectedKnightHero));
		TestEqual(TEXT("SetCharacterOptions는 기사 전용 생성 배경을 적용"),
			GeneratedResource, static_cast<UObject*>(ExpectedKnightBackground));
		TestTrue(TEXT("전경 원화와 생성 배경은 별도 자산"),
			ForegroundResource != GeneratedResource);
	}
	// 배열 순서가 아니라 직업 키로 배경을 골라야 서버 후보 정렬이 바뀌어도
	// 잘못된 색장이 나오지 않는다. 한 번 실제로 Mage로 전환해 둘 다 검증한다.
	TArray<FFrontendCharacterOption> MageOnly;
	MageOnly.Add(MakeOption(0, TEXT("마법사"), true));
	MageOnly[0].mJobType = EUnitJobType::Mage;
	Board->SetCharacterOptions(MageOnly, 3);
	UTexture2D* ExpectedMageHero = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/MercenaryHire/HeroCutouts/"
			"T_HireHeroCutout_Mage_v1.T_HireHeroCutout_Mage_v1"));
	UTexture2D* ExpectedMageBackground = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/UI/MercenaryHire/GeneratedBackgrounds/"
			"T_HireGeneratedBG_Mage_v1.T_HireGeneratedBG_Mage_v1"));
	if (TestNotNull(TEXT("런타임 마법사 전신 원화"), ExpectedMageHero)
		&& TestNotNull(TEXT("런타임 마법사 생성 배경"), ExpectedMageBackground))
	{
		TestEqual(TEXT("직업 전환은 마법사 전경 적용"),
			RuntimeForegroundArt->GetBrush().GetResourceObject(),
			static_cast<UObject*>(ExpectedMageHero));
		TestEqual(TEXT("직업 전환은 마법사 전용 생성 배경 적용"),
			RuntimeGeneratedArt->GetBrush().GetResourceObject(),
			static_cast<UObject*>(ExpectedMageBackground));
	}
	// 아래 스킬/편성 계약은 다섯 스킬이 든 기사 fixture를 사용한다.
	Board->SetCharacterOptions(KnightOnly, 3);
	UTextBlock* KnightRole = Cast<UTextBlock>(
		Board->WidgetTree->FindWidget(TEXT("HireRole_0")));
	if (TestNotNull(TEXT("기사 역할 부연설명"), KnightRole))
	{
		TestEqual(TEXT("역할 문구는 실제 후보 데이터를 표시한다"),
			KnightRole->GetText().ToString(), FString(TEXT("방패 탱커 · 근접")));
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		UButton* SkillButton = Cast<UButton>(Board->WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("HireDetailSkillButton_%d"), Index))));
		if (TestNotNull(*FString::Printf(TEXT("스킬 %d 단일 클릭 버튼"), Index),
			SkillButton))
		{
			TestTrue(*FString::Printf(TEXT("스킬 %d 클릭 배선"), Index),
				SkillButton->OnClicked.IsBound());
		}
		UTextBlock* SkillText = Cast<UTextBlock>(Board->WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("HireDetailSkillText_%d"), Index))));
		if (TestNotNull(*FString::Printf(TEXT("커맨드 %d 표시 글자"), Index), SkillText))
		{
			const FString Expected = Index == 0 ? TEXT("이동")
				: FString::Printf(TEXT("검수 스킬 %d"), Index - 1);
			TestEqual(*FString::Printf(TEXT("커맨드 %d는 이동+스킬 순서"), Index),
				SkillText->GetText().ToString(), Expected);
		}
		TestEqual(*FString::Printf(TEXT("커맨드 %d 상세 데이터 대응"), Index),
			Board->GetSkillDataIndexForSlotForTest(Index),
			Index == 0 ? INDEX_NONE : Index - 1);
	}

	for (int32 Index = 1; Index < 6; ++Index)
	{
		UButton* SkillButton = Cast<UButton>(Board->WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("HireDetailSkillButton_%d"), Index))));
		if (!TestNotNull(*FString::Printf(TEXT("스킬 %d 클릭 대상"), Index),
			SkillButton))
		{
			return false;
		}
		SkillButton->OnClicked.Broadcast();
		UUserWidget* DetailOverlay = Board->GetSkillDetailOverlayForTest();
		if (!TestNotNull(*FString::Printf(TEXT("스킬 %d 상세 겹"), Index),
			DetailOverlay))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("스킬 %d 상세 표시"), Index),
			DetailOverlay->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		UTextBlock* DetailTitle = Cast<UTextBlock>(
			DetailOverlay->GetWidgetFromName(TEXT("DetailTitleText")));
		UTextBlock* DetailBody = Cast<UTextBlock>(
			DetailOverlay->GetWidgetFromName(TEXT("DetailBodyText")));
		if (TestNotNull(TEXT("스킬 상세 제목"), DetailTitle))
		{
			TestEqual(*FString::Printf(TEXT("스킬 %d 제목 인덱스"), Index),
				DetailTitle->GetText().ToString(),
				FString::Printf(TEXT("검수 스킬 %d"), Index - 1));
		}
		if (TestNotNull(TEXT("스킬 상세 본문"), DetailBody))
		{
			TestTrue(*FString::Printf(TEXT("스킬 %d 설명 인덱스"), Index),
				DetailBody->GetText().ToString().Contains(
					FString::Printf(TEXT("검수 설명 %d"), Index - 1)));
		}
		if (UButton* CloseCatch = Cast<UButton>(
			DetailOverlay->GetWidgetFromName(TEXT("DetailCloseCatch"))))
		{
			TestEqual(TEXT("과거 닫기 받이는 비활성화"),
				CloseCatch->GetVisibility(), ESlateVisibility::Collapsed);
			CloseCatch->OnClicked.Broadcast();
			TestEqual(TEXT("상세 받이는 입력만 삼키고 상세를 유지"),
				DetailOverlay->GetVisibility(),
				ESlateVisibility::SelfHitTestInvisible);
		}
		if (UButton* Shield = Cast<UButton>(DetailOverlay->GetWidgetFromName(
			TEXT("RuntimeDetailModalInputShield"))))
		{
			TestFalse(TEXT("새 전 화면 받이에는 닫기 이벤트가 없음"),
				Shield->OnClicked.IsBound());
			Shield->OnClicked.Broadcast();
			TestEqual(TEXT("새 전 화면 받이를 눌러도 상세 유지"),
				DetailOverlay->GetVisibility(),
				ESlateVisibility::SelfHitTestInvisible);
		}
		if (UButton* CloseButton = Cast<UButton>(
			DetailOverlay->GetWidgetFromName(TEXT("DetailCloseButton"))))
		{
			CloseButton->OnClicked.Broadcast();
			TestEqual(TEXT("명시적인 닫기 버튼으로만 상세 종료"),
				DetailOverlay->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}

	// 전투 HUD가 StaticSkillData의 Star를 UI용 Diagonal(조준)/Star(타격)로
	// 바꾸는 현재 계약을 고용 상세도 그대로 따른다. 타격 Star는 예전에 Cross로
	// 뭉개졌지만, 게임플레이(TileMapModel::GetEffectTiles)가 8방향으로 치므로
	// 이제 UI도 8방향(Star)을 보존한다.
	KnightOnly[0].mSkillDetails[0].mAimPattern = EAimPattern::Star;
	KnightOnly[0].mSkillDetails[0].mAimRange = 2;
	KnightOnly[0].mSkillDetails[0].mEffectPattern = EEffectPattern::Star;
	KnightOnly[0].mSkillDetails[0].mEffectArea = 2;
	KnightOnly[0].mSkillDetails[0].mDamageMin = 6;
	KnightOnly[0].mSkillDetails[0].mDamageMax = 10;
	// CombatGameMode의 카드 계약과 동일: MaxDamage의 1.5배를 반올림한다.
	KnightOnly[0].mSkillDetails[0].mCriticalDamage = FMath::RoundToInt(
		KnightOnly[0].mSkillDetails[0].mDamageMax * 1.5f);
	Board->SetCharacterOptions(KnightOnly, 3);
	Board->TriggerSkillClickForTest(1);
	/*
	 * 고용 상세는 이제 전투 HUD와 같은 프레젠터를 그대로 쓴다. 그림을 흉내낸
	 * 구식 5x5 격자 단언 대신, 같은 변환기(SkillDetailUIBuilder)가 같은 모양을
	 * 내는지와 프레젠터가 같은 칩 문구를 그리는지를 본다.
	 */
	FSkillDetailUI ParityDetail;
	SkillDetailUIBuilder::FillFromFrontendOption(
		KnightOnly[0].mSkillDetails[0], ParityDetail);
	TestEqual(TEXT("Star 조준은 HUD와 같은 Diagonal 모양"),
		ParityDetail.mTargeting.mSelectShape, ECombatSkillSelectShapeUI::Diagonal);
	TestEqual(TEXT("Star 타격은 게임플레이와 같은 8방향(Star) 모양"),
		ParityDetail.mTargeting.mHitShape, ECombatSkillHitShapeUI::Star);
	TestEqual(TEXT("치명타는 전투 HUD와 같은 MaxDamage 1.5배"),
		ParityDetail.mCriticalDamage, 15);
	if (USkillDetailOverlayPresenter* Presenter = Board->GetSkillDetailPresenterForTest())
	{
		bool bFoundCriticalChip = false;
		for (int32 ChipIndex = 0; ChipIndex < 5; ++ChipIndex)
		{
			if (Presenter->GetChipValueString(ChipIndex).Contains(TEXT("15")))
			{
				bFoundCriticalChip = true;
				break;
			}
		}
		TestTrue(TEXT("프레젠터 칩에 치명타 15가 실린다"), bFoundCriticalChip);
	}
	else
	{
		AddError(TEXT("고용 상세 프레젠터가 만들어지지 않았다"));
	}

	Board->ClickCard(0);
	Board->ClickAdd();
	TestTrue(TEXT("실제 후보는 추가 버튼으로 편성된다"),
		Board->GetChosenIndices().Contains(0));
	Board->ClickCard(5);
	Board->ClickAdd();
	TestFalse(TEXT("서버가 주지 않은 가짜 여섯째 후보는 선택되지 않는다"),
		Board->GetChosenIndices().Contains(5));
	TestEqual(TEXT("실제 후보 배열은 UI에서 임의로 늘리지 않는다"),
		Board->GetChosenIndices().Num(), 1);

	UButton* PartySlotButton = Cast<UButton>(
		Board->WidgetTree->FindWidget(TEXT("PartySlotButton_0")));
	UTextBlock* PartySlotName = Cast<UTextBlock>(
		Board->WidgetTree->FindWidget(TEXT("PartySlotName_0")));
	UTextBlock* PartySlotLevel = Cast<UTextBlock>(
		Board->WidgetTree->FindWidget(TEXT("PartySlotLevel_0")));
	UButton* AddButton = Cast<UButton>(
		Board->WidgetTree->FindWidget(TEXT("HireAddButton")));
	TestNotNull(TEXT("파티 슬롯 이름"), PartySlotName);
	TestNotNull(TEXT("파티 슬롯 레벨 별도 줄"), PartySlotLevel);
	if (TestNotNull(TEXT("상세 아래 추가 버튼"), AddButton))
	{
		TestTrue(TEXT("추가 버튼 동작이 묶여 있다"), AddButton->OnClicked.IsBound());
	}
	// 단일 문구 버튼은 아트와 클릭 영역 사이에 더 작은 라벨 좌표계를 두지 않는다.
	// 문자열이 영어/한국어 또는 비용으로 바뀌어도 이 전체 사각 안에서 중앙 정렬된다.
	struct FSingleLabelButtonContract
	{
		const TCHAR* HolderName;
		const TCHAR* CenterName;
		const TCHAR* LabelName;
		const TCHAR* ButtonName;
		FVector2D Size;
	};
	const FSingleLabelButtonContract SingleLabelButtons[] = {
		{ TEXT("HireAddHolder"), TEXT("HireAddLabel_Center"),
			TEXT("HireAddLabel"), TEXT("HireAddButton"), FVector2D(270.f, 106.f) },
		{ TEXT("DepartHolder"), TEXT("DepartLabel_Center"),
			TEXT("DepartLabel"), TEXT("DepartButton"), FVector2D(224.f, 106.f) },
		{ TEXT("HireBackHolder"), TEXT("HireBackLabel_Center"),
			TEXT("HireBackLabel"), TEXT("HireBackButton"), FVector2D(270.f, 106.f) },
	};
	for (const FSingleLabelButtonContract& Expected : SingleLabelButtons)
	{
		UWidget* Holder = Board->WidgetTree->FindWidget(Expected.HolderName);
		UOverlay* Center = Cast<UOverlay>(
			Board->WidgetTree->FindWidget(Expected.CenterName));
		UTextBlock* Label = Cast<UTextBlock>(
			Board->WidgetTree->FindWidget(Expected.LabelName));
		UScaleBox* AutoFit = Cast<UScaleBox>(Board->WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("%s_AutoFit"), Expected.LabelName))));
		UButton* Button = Cast<UButton>(
			Board->WidgetTree->FindWidget(Expected.ButtonName));
		if (TestNotNull(*FString::Printf(TEXT("%s 전체 라벨"), Expected.CenterName),
			Center) && TestNotNull(*FString::Printf(TEXT("%s 클릭 영역"),
			Expected.ButtonName), Button))
		{
			TestEqual(*FString::Printf(TEXT("%s 라벨 부모"), Expected.CenterName),
				Center->GetParent(), Cast<UPanelWidget>(Holder));
			UCanvasPanelSlot* CenterSlot = Cast<UCanvasPanelSlot>(Center->Slot);
			UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Button->Slot);
			if (TestNotNull(TEXT("라벨 Canvas 슬롯"), CenterSlot)
				&& TestNotNull(TEXT("버튼 Canvas 슬롯"), ButtonSlot))
			{
				TestEqual(TEXT("라벨과 버튼 시작점 일치"),
					CenterSlot->GetPosition(), ButtonSlot->GetPosition());
				TestEqual(TEXT("라벨과 버튼 크기 일치"),
					CenterSlot->GetSize(), ButtonSlot->GetSize());
				TestEqual(TEXT("라벨은 홀더 전체 크기"),
					CenterSlot->GetSize(), Expected.Size);
			}
		}
		if (TestNotNull(*FString::Printf(TEXT("%s 축소 래퍼"), Expected.LabelName),
			AutoFit))
		{
			TestEqual(TEXT("축소 래퍼는 전체 라벨의 자식"), AutoFit->GetParent(),
				static_cast<UPanelWidget*>(Center));
			TestEqual(TEXT("긴 문구만 가로 축소"), AutoFit->GetStretch(),
				EStretch::ScaleToFitX);
			TestEqual(TEXT("짧은 문구는 확대하지 않음"),
				AutoFit->GetStretchDirection(), EStretchDirection::DownOnly);
		}
		if (TestNotNull(*FString::Printf(TEXT("%s 문구"), Expected.LabelName), Label))
		{
			TestEqual(TEXT("문구는 축소 래퍼의 자식"), Label->GetParent(),
				static_cast<UPanelWidget*>(AutoFit));
			TestEqual(TEXT("단일 문구에 좌표 보정 없음"),
				Label->GetRenderTransform().Translation, FVector2D::ZeroVector);
		}
	}
	if (TestNotNull(TEXT("파티 슬롯 해제 버튼"), PartySlotButton))
	{
		TestTrue(TEXT("파티 슬롯 해제 동작이 묶여 있다"),
			PartySlotButton->OnClicked.IsBound());
		PartySlotButton->OnClicked.Broadcast();
		TestFalse(TEXT("파티 슬롯을 누르면 해당 용병이 빠진다"),
			Board->GetChosenIndices().Contains(0));
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

	UPanelWidget* RootCanvas = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("RootCanvas")));
	if (!TestNotNull(TEXT("전투 HUD 루트 Canvas"), RootCanvas))
	{
		return false;
	}

	// #567 크기 조정본의 Canvas 계약. 전체 HUD 재생성기가 우측 HUD를
	// 임의 배율/좌표로 다시 쓰면 폴드에서 설정 바와 요약판이 서로 붙고,
	// 스킬 판만 납작해진다. 존재 여부가 아니라 기준 좌표까지 잠근다.
	auto CheckCanvasContract = [this](const FString& Context, UWidget* Widget,
		const FVector2D Position, const FVector2D Size,
		const FVector2D AnchorMinimum, const FVector2D AnchorMaximum,
		const FVector2D Alignment, const int32 ZOrder)
		-> UCanvasPanelSlot*
	{
		if (!TestNotNull(*Context, Widget))
		{
			return nullptr;
		}
		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot);
		if (!TestNotNull(*FString::Printf(TEXT("%s Canvas 슬롯"), *Context), Slot))
		{
			return nullptr;
		}
		TestTrue(*FString::Printf(TEXT("%s 위치"), *Context),
			Slot->GetPosition().Equals(Position, 0.01f));
		TestTrue(*FString::Printf(TEXT("%s 크기"), *Context),
			Slot->GetSize().Equals(Size, 0.01f));
		TestEqual(*FString::Printf(TEXT("%s 최소 앵커"), *Context),
			Slot->GetAnchors().Minimum, AnchorMinimum);
		TestEqual(*FString::Printf(TEXT("%s 최대 앵커"), *Context),
			Slot->GetAnchors().Maximum, AnchorMaximum);
		TestEqual(*FString::Printf(TEXT("%s 정렬"), *Context),
			Slot->GetAlignment(), Alignment);
		TestFalse(*FString::Printf(TEXT("%s 자동 크기 꺼짐"), *Context),
			Slot->GetAutoSize());
		TestEqual(*FString::Printf(TEXT("%s ZOrder"), *Context),
			Slot->GetZOrder(), ZOrder);
		return Slot;
	};

	auto CheckRenderContract = [this](const FString& Context, UWidget* Widget,
		const FVector2D Scale, const FVector2D Pivot)
	{
		if (Widget == nullptr)
		{
			return;
		}
		const FWidgetTransform& Transform = Widget->GetRenderTransform();
		TestEqual(*FString::Printf(TEXT("%s 렌더 이동"), *Context),
			Transform.Translation, FVector2D::ZeroVector);
		TestEqual(*FString::Printf(TEXT("%s 렌더 배율"), *Context),
			Transform.Scale, Scale);
		TestEqual(*FString::Printf(TEXT("%s 렌더 전단"), *Context),
			Transform.Shear, FVector2D::ZeroVector);
		TestEqual(*FString::Printf(TEXT("%s 렌더 회전"), *Context),
			Transform.Angle, 0.f);
		TestEqual(*FString::Printf(TEXT("%s 렌더 피벗"), *Context),
			Widget->GetRenderTransformPivot(), Pivot);
	};

	auto CheckOverlayContract = [this](const FString& Context, UWidget* Widget,
		const FMargin Padding, const EHorizontalAlignment Horizontal,
		const EVerticalAlignment Vertical) -> UOverlaySlot*
	{
		if (!TestNotNull(*Context, Widget))
		{
			return nullptr;
		}
		UOverlaySlot* Slot = Cast<UOverlaySlot>(Widget->Slot);
		if (!TestNotNull(*FString::Printf(TEXT("%s Overlay 슬롯"), *Context), Slot))
		{
			return nullptr;
		}
		TestEqual(*FString::Printf(TEXT("%s 여백"), *Context),
			Slot->GetPadding(), Padding);
		TestEqual(*FString::Printf(TEXT("%s 가로 정렬"), *Context),
			Slot->GetHorizontalAlignment(), Horizontal);
		TestEqual(*FString::Printf(TEXT("%s 세로 정렬"), *Context),
			Slot->GetVerticalAlignment(), Vertical);
		return Slot;
	};

	UCanvasPanel* ObjectivePanel = Cast<UCanvasPanel>(
		Tree->FindWidget(TEXT("ObjectivePanel")));
	if (TestNotNull(TEXT("#567 우측 설정 바 루트"), ObjectivePanel))
	{
		TestEqual(TEXT("설정 바는 HUD 루트 직계 자식"),
			ObjectivePanel->GetParent(), RootCanvas);
		TestEqual(TEXT("설정 바 기본 표시"), ObjectivePanel->GetVisibility(),
			ESlateVisibility::SelfHitTestInvisible);
		CheckCanvasContract(TEXT("설정 바 루트"), ObjectivePanel,
			FVector2D(-570.f, 2.f), FVector2D(564.f, 207.6f),
			FVector2D(1.f, 0.f), FVector2D(1.f, 0.f),
			FVector2D::ZeroVector, 90);
		CheckRenderContract(TEXT("설정 바 루트"), ObjectivePanel,
			FVector2D(0.75f, 0.75f), FVector2D(1.f, 0.f));
	}
	// donor에는 없는 임무 부품은 BP 그래프 호환 때문에 남아 있을 수 있다.
	// ObjectiveText는 자식 자체보다 접힌 Center 계보에 가려지는 authored
	// 구조이므로, 최상위 임무 판/중앙 래퍼만 화면에서 빠지는지 확인한다.
	for (const TCHAR* RetiredRoot : { TEXT("ObjectivePlate"),
		TEXT("ObjectiveText_Center") })
	{
		if (UWidget* Widget = Tree->FindWidget(FName(RetiredRoot)))
		{
			TestEqual(*FString::Printf(TEXT("구형 %s 는 접힘"), RetiredRoot),
				Widget->GetVisibility(), ESlateVisibility::Collapsed);
		}
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FString Name = FString::Printf(TEXT("MenuButton_%d"), Index);
		TestNotNull(*FString::Printf(TEXT("%s 유지"), *Name),
			Cast<UButton>(Tree->FindWidget(FName(*Name))));
	}

	UImage* OptionsRailFrame =
		Cast<UImage>(Tree->FindWidget(TEXT("OptionsRailFrame")));
	UOverlay* OptionsRailFrameMount = Cast<UOverlay>(
		Tree->FindWidget(TEXT("OptionsRailFrameMount")));
	if (TestNotNull(TEXT("옵션 프레임 Overlay"), OptionsRailFrameMount))
	{
		TestEqual(TEXT("옵션 프레임은 설정 바 안"),
			OptionsRailFrameMount->GetParent(),
			static_cast<UPanelWidget*>(ObjectivePanel));
		CheckCanvasContract(TEXT("옵션 프레임 Overlay"), OptionsRailFrameMount,
			FVector2D::ZeroVector, FVector2D::ZeroVector,
			FVector2D::ZeroVector, FVector2D(1.f, 1.f),
			FVector2D::ZeroVector, 1);
	}
	if (TestNotNull(TEXT("옵션 네 칸 프레임"), OptionsRailFrame))
	{
		TestEqual(TEXT("옵션 프레임 그림은 Overlay 안"),
			OptionsRailFrame->GetParent(),
			static_cast<UPanelWidget*>(OptionsRailFrameMount));
		CheckOverlayContract(TEXT("옵션 프레임 그림"), OptionsRailFrame,
			FMargin(0.f), HAlign_Fill, VAlign_Fill);
		TestNotNull(TEXT("옵션 프레임 텍스처"),
			Cast<UTexture2D>(OptionsRailFrame->GetBrush().GetResourceObject()));
	}

	struct FOptionIconContract
	{
		const TCHAR* Name;
		FVector2D Position;
		FVector2D Size;
	};
	const FOptionIconContract OptionIcons[] = {
		{ TEXT("MenuMapIcon"), FVector2D(-228.f, -51.f), FVector2D(96.f, 97.2f) },
		{ TEXT("MenuMercenaryIcon"), FVector2D(-97.2f, -60.6f), FVector2D(75.6f, 115.2f) },
		{ TEXT("MenuMonsterIcon"), FVector2D(15.6f, -46.2f), FVector2D(92.4f, 99.6f) },
		{ TEXT("MenuSettingsIcon"), FVector2D(135.6f, -46.2f), FVector2D(92.4f, 96.f) },
	};
	for (const FOptionIconContract& Expected : OptionIcons)
	{
		UImage* Icon = Cast<UImage>(Tree->FindWidget(FName(Expected.Name)));
		if (TestNotNull(*FString::Printf(TEXT("%s 아이콘"), Expected.Name), Icon))
		{
			TestEqual(*FString::Printf(TEXT("%s 설정 바 직계 자식"), Expected.Name),
				Icon->GetParent(), static_cast<UPanelWidget*>(ObjectivePanel));
			CheckCanvasContract(FString::Printf(TEXT("%s 아이콘"), Expected.Name),
				Icon, Expected.Position, Expected.Size, FVector2D(.5f, .5f),
				FVector2D(.5f, .5f), FVector2D::ZeroVector, 31);
			TestNotNull(*FString::Printf(TEXT("%s 텍스처"), Expected.Name),
				Cast<UTexture2D>(Icon->GetBrush().GetResourceObject()));
		}
	}

	const FMargin MenuButtonPadding[] = {
		FMargin(37.f, 31.f, 339.f, 30.f),
		FMargin(138.f, 31.f, 238.f, 30.f),
		FMargin(242.f, 31.f, 140.f, 30.f),
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(MenuButtonPadding); ++Index)
	{
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(
			*FString::Printf(TEXT("MenuButton_%d"), Index))));
		if (Button != nullptr)
		{
			TestEqual(*FString::Printf(TEXT("메뉴 %d 버튼은 옵션 Overlay 안"), Index),
				Button->GetParent(), static_cast<UPanelWidget*>(OptionsRailFrameMount));
			CheckOverlayContract(FString::Printf(TEXT("메뉴 %d 버튼"), Index),
				Button, MenuButtonPadding[Index], HAlign_Fill, VAlign_Fill);
		}
	}
	if (UButton* SettingsButton = Cast<UButton>(
		Tree->FindWidget(TEXT("MenuButton_3"))))
	{
		TestEqual(TEXT("설정 버튼은 옵션 Overlay 안"),
			SettingsButton->GetParent(),
			static_cast<UPanelWidget*>(OptionsRailFrameMount));
		CheckOverlayContract(TEXT("설정 버튼"), SettingsButton,
			FMargin(344.f, 36.f, 42.5f, 35.f), HAlign_Fill, VAlign_Fill);
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
			TestEqual(*FString::Printf(TEXT("%s 요약판은 HUD 루트 직계 자식"),
				Prefix), SummaryPanel->GetParent(), RootCanvas);
			TestEqual(*FString::Printf(TEXT("%s 요약판 기본값은 닫힘"), Prefix),
				SummaryPanel->GetVisibility(), ESlateVisibility::Collapsed);
			CheckCanvasContract(FString::Printf(TEXT("%s 요약판"), Prefix),
				SummaryPanel, FVector2D(0.f, 140.f), FVector2D(575.f, 430.f),
				FVector2D(1.f, 0.f), FVector2D(1.f, 0.f),
				FVector2D(1.f, 0.f), 60);
			CheckRenderContract(FString::Printf(TEXT("%s 요약판"), Prefix),
				SummaryPanel, FVector2D(1.f, 1.f), FVector2D(1.f, 0.f));
		}
		if (UImage* SummaryPlate = Cast<UImage>(Tree->FindWidget(FName(
			*FString::Printf(TEXT("%sPlate"), Prefix)))))
		{
			UObject* Resource = SummaryPlate->GetBrush().GetResourceObject();
			if (TestNotNull(*FString::Printf(TEXT("%s 하단 없는 요약판 에셋"),
				Prefix), Resource))
			{
				TestEqual(TEXT("요약판은 하단 빈 바 제거 에셋 사용"),
					Resource->GetPathName(), FString(TEXT(
						"/Game/UI/Generated/CombatHUD/"
						"T_MB_GenericDetailPanel_NoFooter_v1."
						"T_MB_GenericDetailPanel_NoFooter_v1")));
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
			const FString StatusButtonName = FString::Printf(
				TEXT("%sStatusButton_%d"), Prefix, StatusIndex);
			if (UButton* StatusButton = Cast<UButton>(
				Tree->FindWidget(FName(*StatusButtonName))))
			{
				TestEqual(*FString::Printf(TEXT("%s 기본 숨김"),
					*StatusButtonName), StatusButton->GetVisibility(),
					ESlateVisibility::Collapsed);
			}
			else
			{
				AddError(FString::Printf(TEXT("%s 상태 상세 버튼 없음"),
					*StatusButtonName));
			}
			for (const TCHAR* Suffix : { TEXT("Frame"), TEXT("Icon"), TEXT("Count") })
			{
				const FString WidgetName = FString::Printf(
					TEXT("%sStatus%s_%d"), Prefix, Suffix, StatusIndex);
				TestNotNull(*WidgetName, Tree->FindWidget(FName(*WidgetName)));
			}
		}
	}

	// 우하단 두 버튼은 같은 authored 396.172241x181.435410 판을 쓴다.
	// 버튼/그림/중앙 문구가 별도 Canvas 좌표로 흩어지면 한 요소만 줄어드는
	// 회귀가 다시 생기므로, 동일 크기 Overlay 계보 전체를 함께 확인한다.
	struct FActionPanelContract
	{
		const TCHAR* PanelName;
		const TCHAR* MountName;
		const TCHAR* PlateName;
		const TCHAR* ButtonName;
		const TCHAR* LabelCenterName;
		const TCHAR* LabelName;
		FVector2D Position;
	};
	const FVector2D ActionPanelSize(396.172241f, 181.435410f);
	const FActionPanelContract ActionPanels[] = {
		{ TEXT("SkillTogglePanel"), TEXT("SkillTogglePlateMount"),
			TEXT("SkillTogglePlate"), TEXT("SkillToggleButton"),
			TEXT("SkillToggleLabel_Center"), TEXT("SkillToggleLabel"),
			FVector2D(-10.334961f, -202.f) },
		{ TEXT("EndTurnPanel"), TEXT("EndTurnPlateMount"),
			TEXT("EndTurnPlate"), TEXT("EndTurnButton"),
			TEXT("EndTurnLabel_Center"), TEXT("EndTurnLabel"),
			FVector2D(-10.334961f, -26.f) },
	};
	TArray<FVector2D> AuthoredActionSizes;
	for (const FActionPanelContract& Expected : ActionPanels)
	{
		UCanvasPanel* Panel = Cast<UCanvasPanel>(
			Tree->FindWidget(FName(Expected.PanelName)));
		UOverlay* Mount = Cast<UOverlay>(
			Tree->FindWidget(FName(Expected.MountName)));
		UImage* Plate = Cast<UImage>(
			Tree->FindWidget(FName(Expected.PlateName)));
		UButton* Button = Cast<UButton>(
			Tree->FindWidget(FName(Expected.ButtonName)));
		UOverlay* LabelCenter = Cast<UOverlay>(
			Tree->FindWidget(FName(Expected.LabelCenterName)));
		UTextBlock* Label = Cast<UTextBlock>(
			Tree->FindWidget(FName(Expected.LabelName)));
		UScaleBox* LabelFit = Cast<UScaleBox>(Tree->FindWidget(FName(
			*FString::Printf(TEXT("%s_AutoFit"), Expected.LabelName))));

		if (TestNotNull(*FString::Printf(TEXT("%s 우하단 루트"),
			Expected.PanelName), Panel))
		{
			TestEqual(*FString::Printf(TEXT("%s는 HUD 루트 직계 자식"),
				Expected.PanelName), Panel->GetParent(), RootCanvas);
			TestEqual(*FString::Printf(TEXT("%s 기본 표시"), Expected.PanelName),
				Panel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
			if (UCanvasPanelSlot* Slot = CheckCanvasContract(
				FString::Printf(TEXT("%s 우하단 루트"), Expected.PanelName),
				Panel, Expected.Position, ActionPanelSize,
				FVector2D(1.f, 1.f), FVector2D(1.f, 1.f),
				FVector2D(1.f, 1.f), 60))
			{
				AuthoredActionSizes.Add(Slot->GetSize());
			}
			CheckRenderContract(
				FString::Printf(TEXT("%s 우하단 루트"), Expected.PanelName),
				Panel, FVector2D(1.f, 1.f), FVector2D(0.5f, 0.5f));
		}
		if (TestNotNull(*FString::Printf(TEXT("%s Overlay"), Expected.MountName),
			Mount))
		{
			TestEqual(*FString::Printf(TEXT("%s 부모"), Expected.MountName),
				Mount->GetParent(), static_cast<UPanelWidget*>(Panel));
			CheckCanvasContract(FString::Printf(TEXT("%s Overlay"),
				Expected.MountName), Mount, FVector2D::ZeroVector,
				ActionPanelSize, FVector2D::ZeroVector, FVector2D::ZeroVector,
				FVector2D::ZeroVector, 0);
		}
		for (const TPair<const TCHAR*, UWidget*> OverlayChild : {
			TPair<const TCHAR*, UWidget*>(Expected.PlateName, Plate),
			TPair<const TCHAR*, UWidget*>(Expected.ButtonName, Button),
			TPair<const TCHAR*, UWidget*>(Expected.LabelCenterName, LabelCenter) })
		{
			if (TestNotNull(*FString::Printf(TEXT("%s Overlay 자식"),
				OverlayChild.Key), OverlayChild.Value))
			{
				TestEqual(*FString::Printf(TEXT("%s 부모"), OverlayChild.Key),
					OverlayChild.Value->GetParent(),
					static_cast<UPanelWidget*>(Mount));
				CheckOverlayContract(FString::Printf(TEXT("%s Overlay 자식"),
					OverlayChild.Key), OverlayChild.Value, FMargin(0.f),
					HAlign_Fill, VAlign_Fill);
			}
		}
		if (TestNotNull(*FString::Printf(TEXT("%s 축소 래퍼"),
			Expected.LabelName), LabelFit))
		{
			TestEqual(*FString::Printf(TEXT("%s 중앙 래퍼"), Expected.LabelName),
				LabelFit->GetParent(), static_cast<UPanelWidget*>(LabelCenter));
			CheckOverlayContract(FString::Printf(TEXT("%s 축소 영역"),
				Expected.LabelName), LabelFit, FMargin(0.f), HAlign_Fill, VAlign_Fill);
			TestEqual(TEXT("긴 액션명만 가로 축소"), LabelFit->GetStretch(),
				EStretch::ScaleToFitX);
			TestEqual(TEXT("짧은 액션명은 확대하지 않음"),
				LabelFit->GetStretchDirection(), EStretchDirection::DownOnly);
		}
		if (TestNotNull(*FString::Printf(TEXT("%s 중앙 문구"),
			Expected.LabelName), Label))
		{
			TestEqual(*FString::Printf(TEXT("%s 축소 자식"), Expected.LabelName),
				Label->GetParent(), static_cast<UPanelWidget*>(LabelFit));
			UScaleBoxSlot* LabelSlot = Cast<UScaleBoxSlot>(Label->Slot);
			if (TestNotNull(TEXT("액션 문구 ScaleBox 슬롯"), LabelSlot))
			{
				TestEqual(TEXT("액션 문구 가로 중앙"),
					LabelSlot->GetHorizontalAlignment(), HAlign_Center);
				TestEqual(TEXT("액션 문구 세로 중앙"),
					LabelSlot->GetVerticalAlignment(), VAlign_Center);
			}
			TestEqual(*FString::Printf(TEXT("%s 좌표 보정 없음"),
				Expected.LabelName), Label->GetRenderTransform().Translation,
				FVector2D::ZeroVector);
		}
	}
	if (AuthoredActionSizes.Num() == UE_ARRAY_COUNT(ActionPanels))
	{
		TestEqual(TEXT("스킬/턴 종료 루트는 정확히 같은 크기"),
			AuthoredActionSizes[0], AuthoredActionSizes[1]);
	}
	// 조준 확정은 평상시 스킬 단추와 자리를 바꾸지만 동일한 단일 문구
	// 버튼이다. 숨겨져 있어도 라벨/클릭 영역 계약은 같아야 한다.
	UOverlay* ConfirmMount = Cast<UOverlay>(
		Tree->FindWidget(TEXT("ConfirmPlateMount")));
	UOverlay* ConfirmCenter = Cast<UOverlay>(
		Tree->FindWidget(TEXT("ConfirmLabel_Center")));
	UTextBlock* ConfirmLabel = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("ConfirmLabel")));
	UScaleBox* ConfirmFit = Cast<UScaleBox>(
		Tree->FindWidget(TEXT("ConfirmLabel_AutoFit")));
	UButton* ConfirmButton = Cast<UButton>(
		Tree->FindWidget(TEXT("ConfirmButton")));
	if (TestNotNull(TEXT("확정 버튼 Overlay"), ConfirmMount)
		&& TestNotNull(TEXT("확정 전체 라벨"), ConfirmCenter)
		&& TestNotNull(TEXT("확정 클릭 영역"), ConfirmButton))
	{
		for (const TPair<const TCHAR*, UWidget*> Child : {
			TPair<const TCHAR*, UWidget*>(TEXT("ConfirmLabel_Center"), ConfirmCenter),
			TPair<const TCHAR*, UWidget*>(TEXT("ConfirmButton"), ConfirmButton) })
		{
			TestEqual(*FString::Printf(TEXT("%s 부모"), Child.Key),
				Child.Value->GetParent(), static_cast<UPanelWidget*>(ConfirmMount));
			CheckOverlayContract(FString::Printf(TEXT("%s 전체 영역"), Child.Key),
				Child.Value, FMargin(0.f), HAlign_Fill, VAlign_Fill);
		}
	}
	if (TestNotNull(TEXT("확정 문구"), ConfirmLabel))
	{
		if (TestNotNull(TEXT("확정 문구 축소 래퍼"), ConfirmFit))
		{
			TestEqual(TEXT("확정 축소 래퍼 중앙 부모"), ConfirmFit->GetParent(),
				static_cast<UPanelWidget*>(ConfirmCenter));
		}
		TestEqual(TEXT("확정 문구 축소 자식"), ConfirmLabel->GetParent(),
			static_cast<UPanelWidget*>(ConfirmFit));
		TestEqual(TEXT("확정 문구 좌표 보정 없음"),
			ConfirmLabel->GetRenderTransform().Translation, FVector2D::ZeroVector);
	}
	UImage* SkillPlate = Cast<UImage>(Tree->FindWidget(TEXT("SkillTogglePlate")));
	UImage* EndTurnPlate = Cast<UImage>(Tree->FindWidget(TEXT("EndTurnPlate")));
	if (SkillPlate != nullptr && EndTurnPlate != nullptr)
	{
		TestEqual(TEXT("스킬/턴 종료는 같은 authored 판 에셋"),
			SkillPlate->GetBrush().GetResourceObject(),
			EndTurnPlate->GetBrush().GetResourceObject());
	}

	// 우측 HUD 수술이 좌상단 사용자 배치와 용병 탭을 건드리지 않았는지
	// 같은 자산에서 대표 좌표를 함께 잠근다. 세부 계약은 전용 테스트가 이어서
	// 전 슬롯을 검사한다.
	struct FPreservedHudRect
	{
		const TCHAR* Name;
		FVector2D Position;
		FVector2D Size;
	};
	const FPreservedHudRect PreservedHudRects[] = {
		{ TEXT("RoundPanel"), FVector2D(18.f, 10.f), FVector2D(218.f, 136.f) },
		{ TEXT("TurnPanel"), FVector2D(246.f, 10.f), FVector2D(1090.f, 150.f) },
		{ TEXT("MercRosterSection"), FVector2D(231.f, 279.f),
			FVector2D(375.f, 527.33f) },
	};
	for (const FPreservedHudRect& Sentinel : PreservedHudRects)
	{
		if (UWidget* Widget = Tree->FindWidget(FName(Sentinel.Name)))
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				TestTrue(*FString::Printf(TEXT("%s 기존 위치 보존"), Sentinel.Name),
					Slot->GetPosition().Equals(Sentinel.Position, 0.01f));
				TestTrue(*FString::Printf(TEXT("%s 기존 크기 보존"), Sentinel.Name),
					Slot->GetSize().Equals(Sentinel.Size, 0.01f));
			}
		}
		else
		{
			AddError(FString::Printf(TEXT("%s 보존 위젯 없음"), Sentinel.Name));
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
		if (UWidget* LegacyAPBar = Tree->FindWidget(TEXT("EnemyCritPlate")))
		{
			TestEqual(TEXT("몬스터 치명타 프레임은 용병 요약판과 같이 표시"),
				LegacyAPBar->GetVisibility(),
				ESlateVisibility::SelfHitTestInvisible);
		}
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
	UButton* MercenaryCloseButton = Cast<UButton>(
		Tree->FindWidget(TEXT("MercenaryCloseButton")));
	UOverlay* MercenaryCloseCenter = Cast<UOverlay>(
		Tree->FindWidget(TEXT("MercenaryCloseText_Center")));
	TestNotNull(TEXT("용병 패널 닫기 단추"), MercenaryCloseButton);
	if (TestNotNull(TEXT("용병 패널 닫기 전체 라벨"), MercenaryCloseCenter)
		&& MercenaryCloseButton != nullptr)
	{
		const UCanvasPanelSlot* CenterSlot = Cast<UCanvasPanelSlot>(
			MercenaryCloseCenter->Slot);
		const UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(
			MercenaryCloseButton->Slot);
		if (TestNotNull(TEXT("용병 닫기 라벨 Canvas 슬롯"), CenterSlot)
			&& TestNotNull(TEXT("용병 닫기 버튼 Canvas 슬롯"), ButtonSlot))
		{
			TestEqual(TEXT("용병 닫기 라벨/버튼 시작점 일치"),
				CenterSlot->GetPosition(), ButtonSlot->GetPosition());
			TestEqual(TEXT("용병 닫기 라벨/버튼 크기 일치"),
				CenterSlot->GetSize(), ButtonSlot->GetSize());
		}
	}
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

	// 전체 HUD 빌더가 되살아나면 이 위젯이 MercenaryPanel 바로 아래에
	// 1664x710 고정 사각형으로 생긴다. 본문은 별도의 1920x1080 ScaleBox 안에
	// 있어서 접힌 화면(1280x1066)에서는 둘의 좌표계가 갈라지고 검정 판만
	// 일러스트 위를 덮는다. 이름뿐 아니라 한 스케일 계보도 함께 잠근다.
	TestNull(TEXT("접힌 화면을 덮는 구형 검정 용병 바탕은 없어야 함"),
		Tree->FindWidget(TEXT("MercenaryContentWell")));
	UScaleBox* MercenaryBoardScale = Cast<UScaleBox>(
		Tree->FindWidget(TEXT("MercenaryBoardScale")));
	USizeBox* MercenaryBoardDesignSize = Cast<USizeBox>(
		Tree->FindWidget(TEXT("MercenaryBoardDesignSize")));
	if (TestNotNull(TEXT("용병 본문 공용 반응형 ScaleBox"), MercenaryBoardScale))
	{
		TestEqual(TEXT("용병 본문 ScaleBox는 패널 바로 아래"),
			MercenaryBoardScale->GetParent(), MercenaryPanel);
		TestEqual(TEXT("용병 본문은 비율을 지켜 화면에 맞춤"),
			MercenaryBoardScale->GetStretch(), EStretch::ScaleToFit);
		TestEqual(TEXT("용병 본문은 접힌 화면에서 축소 가능"),
			MercenaryBoardScale->GetStretchDirection(), EStretchDirection::Both);
		if (UCanvasPanelSlot* ScaleSlot =
			Cast<UCanvasPanelSlot>(MercenaryBoardScale->Slot))
		{
			TestEqual(TEXT("용병 본문 ScaleBox 왼쪽 위 앵커"),
				ScaleSlot->GetAnchors().Minimum, FVector2D::ZeroVector);
			TestEqual(TEXT("용병 본문 ScaleBox 오른쪽 아래 앵커"),
				ScaleSlot->GetAnchors().Maximum, FVector2D(1.f, 1.f));
			TestEqual(TEXT("용병 본문 ScaleBox는 패널 전체를 사용"),
				ScaleSlot->GetOffsets(), FMargin(0.f));
		}
		else
		{
			AddError(TEXT("MercenaryBoardScale은 CanvasPanelSlot이어야 함"));
		}
	}
	if (TestNotNull(TEXT("용병 본문 1920x1080 디자인 크기"),
		MercenaryBoardDesignSize))
	{
		TestEqual(TEXT("디자인 크기는 공용 ScaleBox 안"),
			MercenaryBoardDesignSize->GetParent(),
			Cast<UPanelWidget>(MercenaryBoardScale));
		TestEqual(TEXT("용병 본문 디자인 폭"),
			MercenaryBoardDesignSize->GetWidthOverride(), 1920.f);
		TestEqual(TEXT("용병 본문 디자인 높이"),
			MercenaryBoardDesignSize->GetHeightOverride(), 1080.f);
	}
	if (MercenaryBoard != nullptr && MercenaryBoardDesignSize != nullptr)
	{
		TestEqual(TEXT("용병 판은 디자인 크기 안"),
			MercenaryBoard->GetParent(),
			Cast<UPanelWidget>(MercenaryBoardDesignSize));
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

	// 0811 정상 자산의 핵심 배치. 구형 전체 HUD 빌더 좌표는 로스터를
	// (210,246), 390x142로 풀어 놓고 초상을 300x300으로 키워 접힌 화면에서
	// 카드/상세가 겹쳤다. 반응형 ScaleBox의 디자인 좌표를 그대로 보존한다.
	struct FMercenaryCanvasRect
	{
		const TCHAR* Name;
		FVector2D Position;
		FVector2D Size;
	};
	const FMercenaryCanvasRect CanonicalRects[] = {
		{ TEXT("MercRosterSection"), FVector2D(231.f, 279.f),
			FVector2D(375.f, 527.33f) },
		{ TEXT("MercenaryHeroPortrait"), FVector2D(685.55f, 300.8f),
			FVector2D(247.9f, 266.4f) },
	};
	for (const FMercenaryCanvasRect& Expected : CanonicalRects)
	{
		UWidget* Widget = Tree->FindWidget(FName(Expected.Name));
		if (TestNotNull(*FString::Printf(TEXT("%s 정상 배치 위젯"), Expected.Name),
			Widget))
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
			{
				TestTrue(*FString::Printf(TEXT("%s 정상 위치"), Expected.Name),
					Slot->GetPosition().Equals(Expected.Position, 0.1f));
				TestTrue(*FString::Printf(TEXT("%s 정상 크기"), Expected.Name),
					Slot->GetSize().Equals(Expected.Size, 0.1f));
			}
			else
			{
				AddError(FString::Printf(TEXT("%s 는 CanvasPanelSlot이어야 함"),
					Expected.Name));
			}
		}
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
				UPanelWidget* VisualParent = Part->GetParent();
				if (UScaleBox* AutoFit = Cast<UScaleBox>(VisualParent))
				{
					VisualParent = AutoFit->GetParent();
				}
				TestEqual(*FString::Printf(TEXT("%s 는 인벤토리 탭 안"), Name),
					VisualParent, InventoryTab);
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
		UButton* InventoryButton = Cast<UButton>(
			Tree->FindWidget(TEXT("MercenaryInventoryButton")));
		if (TestNotNull(TEXT("인벤토리 탭 카드 판"), InventoryPlate)
			&& TestNotNull(TEXT("용병 카드 판"), PartyPlate))
		{
			TestEqual(TEXT("인벤토리 탭은 용병 카드와 같은 에셋"),
				InventoryPlate->GetBrush().GetResourceObject(),
				PartyPlate->GetBrush().GetResourceObject());
			const UCanvasPanelSlot* InventoryPlateSlot =
				Cast<UCanvasPanelSlot>(InventoryPlate->Slot);
			const UCanvasPanelSlot* InventoryButtonSlot = InventoryButton != nullptr
				? Cast<UCanvasPanelSlot>(InventoryButton->Slot) : nullptr;
			if (TestNotNull(TEXT("인벤토리 판 슬롯"), InventoryPlateSlot)
				&& TestNotNull(TEXT("인벤토리 버튼 슬롯"), InventoryButtonSlot))
			{
				TestEqual(TEXT("인벤토리 입력면은 판 전체와 동일"),
					InventoryButtonSlot->GetSize(), InventoryPlateSlot->GetSize());
			}
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

	UPanelWidget* TurnAPScale =
		Cast<UPanelWidget>(Tree->FindWidget(TEXT("TurnAPScale")));
	UWidget* TurnAPPanel = Tree->FindWidget(TEXT("TurnAPPanel"));
	if (TestNotNull(TEXT("좌하단 AP 크기 래퍼"), TurnAPScale)
		&& TestNotNull(TEXT("좌하단 AP 판"), TurnAPPanel))
	{
		TestEqual(TEXT("AP 판은 크기 래퍼 안"),
			TurnAPPanel->GetParent(), TurnAPScale);
	}

	TestNull(TEXT("전투 화면 독립 아티팩트 WBP 줄 제거"),
		Tree->FindWidget(TEXT("ArtifactStrip")));

	constexpr TCHAR CanonicalPortraitFramePath[] =
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/"
			"T_KitA_Cell_Normal.T_KitA_Cell_Normal");
	auto TestPortraitFrame = [this](UWidgetTree* WidgetTree,
		const TCHAR* TestLabel)
	{
		UImage* Frame = WidgetTree != nullptr
			? Cast<UImage>(WidgetTree->FindWidget(TEXT("MercenaryPortraitFrame")))
			: nullptr;
		if (TestNotNull(TestLabel, Frame))
		{
			const UObject* Resource = Frame->GetBrush().GetResourceObject();
			TestTrue(*FString::Printf(TEXT("%s canonical KitA cell"), TestLabel),
				Resource != nullptr
				&& Resource->GetPathName() == CanonicalPortraitFramePath);
			UWidget* Hero = WidgetTree->FindWidget(TEXT("MercenaryHeroPortrait"));
			UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Frame->Slot);
			UCanvasPanelSlot* HeroSlot = Hero != nullptr
				? Cast<UCanvasPanelSlot>(Hero->Slot) : nullptr;
			if (TestNotNull(*FString::Printf(TEXT("%s frame Canvas 슬롯"), TestLabel),
				FrameSlot)
				&& TestNotNull(*FString::Printf(TEXT("%s hero Canvas 슬롯"), TestLabel),
					HeroSlot))
			{
				TestTrue(*FString::Printf(TEXT("%s 셀은 초상보다 아래"), TestLabel),
					FrameSlot->GetZOrder() < HeroSlot->GetZOrder());
			}
		}
	};
	TestPortraitFrame(Tree, TEXT("인라인 용병 초상화 프레임"));
	UClass* ModularMercenaryClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/CombatLayouts/WBP_MercenaryPanel.WBP_MercenaryPanel_C"));
	UWidgetBlueprintGeneratedClass* ModularMercenaryGenerated =
		Cast<UWidgetBlueprintGeneratedClass>(ModularMercenaryClass);
	TestPortraitFrame(ModularMercenaryGenerated != nullptr
		? ModularMercenaryGenerated->GetWidgetTreeArchetype() : nullptr,
		TEXT("모듈형 용병 초상화 프레임"));

	// 스킬 그림만 있고 실제 Button이 빠지면 모바일 탭은 아무 이벤트도 만들지
	// 못한다. 인라인/모듈형 두 WBP 모두 프레임과 같은 터치 영역을 가져야 한다.
	auto TestMercenarySkillButtons = [this](UWidgetTree* WidgetTree,
		const TCHAR* TestLabel)
	{
		if (WidgetTree == nullptr)
		{
			AddError(FString::Printf(TEXT("%s 위젯 나무가 없다"), TestLabel));
			return;
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			UWidget* Frame = WidgetTree->FindWidget(FName(*FString::Printf(
				TEXT("MercenarySkillFrame_%d"), Index)));
			UButton* Button = Cast<UButton>(WidgetTree->FindWidget(FName(*FString::Printf(
				TEXT("MercenarySkillButton_%d"), Index))));
			if (TestNotNull(*FString::Printf(TEXT("%s 스킬 버튼 %d"), TestLabel, Index), Button)
				&& TestNotNull(*FString::Printf(TEXT("%s 스킬 프레임 %d"), TestLabel, Index), Frame))
			{
				TestEqual(*FString::Printf(TEXT("%s 스킬 버튼 부모 %d"), TestLabel, Index),
					Button->GetParent(), Frame->GetParent());
				UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(Frame->Slot);
				UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Button->Slot);
				if (FrameSlot != nullptr && ButtonSlot != nullptr)
				{
					TestTrue(*FString::Printf(TEXT("%s 스킬 버튼 위치 %d"), TestLabel, Index),
						ButtonSlot->GetPosition().Equals(FrameSlot->GetPosition(), .01f));
					TestTrue(*FString::Printf(TEXT("%s 스킬 버튼 크기 %d"), TestLabel, Index),
						ButtonSlot->GetSize().Equals(FrameSlot->GetSize(), .01f));
					TestTrue(*FString::Printf(TEXT("%s 스킬 버튼 ZOrder %d"), TestLabel, Index),
						ButtonSlot->GetZOrder() > FrameSlot->GetZOrder());
				}
				else
				{
					UOverlaySlot* OverlayButtonSlot = Cast<UOverlaySlot>(Button->Slot);
					TestNotNull(*FString::Printf(TEXT("%s Overlay 입력 슬롯 %d"),
						TestLabel, Index), OverlayButtonSlot);
				}
			}
		}
	};
	TestMercenarySkillButtons(Tree, TEXT("인라인 용병판"));
	TestMercenarySkillButtons(ModularMercenaryGenerated != nullptr
		? ModularMercenaryGenerated->GetWidgetTreeArchetype() : nullptr,
		TEXT("모듈형 용병판"));

	UClass* MonsterTabClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/MonsterTab/WBP_MonsterTab_Marchbound.WBP_MonsterTab_Marchbound_C"));
	UWidgetBlueprintGeneratedClass* MonsterGenerated =
		Cast<UWidgetBlueprintGeneratedClass>(MonsterTabClass);
	UWidgetTree* MonsterTree = MonsterGenerated != nullptr
		? MonsterGenerated->GetWidgetTreeArchetype() : nullptr;
	if (TestNotNull(TEXT("몬스터 탭 WBP 나무"), MonsterTree))
	{
		UImage* MonsterRowNormal = Cast<UImage>(
			MonsterTree->FindWidget(TEXT("MonsterRowNormal_0")));
		UImage* MonsterPortraitFrame = Cast<UImage>(
			MonsterTree->FindWidget(TEXT("MonsterPortraitFrame")));
		TestNotNull(TEXT("현대식 몬스터 이름 명패"),
			MonsterTree->FindWidget(TEXT("MonsterNamePlate")));
		TestNotNull(TEXT("현대식 몬스터 스킬 구분선"),
			MonsterTree->FindWidget(TEXT("MonsterSkillDivider")));
		if (TestNotNull(TEXT("현대식 몬스터 목록 행"), MonsterRowNormal))
		{
			const UObject* Resource = MonsterRowNormal->GetBrush().GetResourceObject();
			TestTrue(TEXT("몬스터 목록 canonical modern row"), Resource != nullptr
				&& Resource->GetPathName().Contains(
					TEXT("/Marchbound/MonsterTab/T_MT_RowNormal")));
		}
		if (TestNotNull(TEXT("몬스터 canonical 초상화 프레임"), MonsterPortraitFrame))
		{
			const UObject* Resource = MonsterPortraitFrame->GetBrush().GetResourceObject();
			TestTrue(TEXT("몬스터와 용병은 같은 KitA 초상화 셀"),
				Resource != nullptr
				&& Resource->GetPathName() == CanonicalPortraitFramePath);
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			UWidget* SkillSocket = MonsterTree->FindWidget(FName(*FString::Printf(
				TEXT("MonsterSkillSlot_%d"), Index)));
			UButton* SkillButton = Cast<UButton>(MonsterTree->FindWidget(
				FName(*FString::Printf(TEXT("MonsterSkillButton_%d"), Index))));
			UCanvasPanelSlot* SocketSlot = SkillSocket != nullptr
				? Cast<UCanvasPanelSlot>(SkillSocket->Slot) : nullptr;
			UCanvasPanelSlot* ButtonSlot = SkillButton != nullptr
				? Cast<UCanvasPanelSlot>(SkillButton->Slot) : nullptr;
			if (TestNotNull(*FString::Printf(TEXT("몬스터 스킬 버튼 %d"), Index),
				SkillButton)
				&& TestNotNull(*FString::Printf(TEXT("몬스터 스킬 소켓 슬롯 %d"), Index),
					SocketSlot)
				&& TestNotNull(*FString::Printf(TEXT("몬스터 스킬 버튼 슬롯 %d"), Index),
					ButtonSlot))
			{
				TestTrue(TEXT("몬스터 스킬 입력 위치는 authored socket과 동일"),
					ButtonSlot->GetPosition().Equals(SocketSlot->GetPosition(), .01f));
				TestTrue(TEXT("몬스터 스킬 입력 크기는 authored socket과 동일"),
					ButtonSlot->GetSize().Equals(SocketSlot->GetSize(), .01f));
				TestTrue(TEXT("몬스터 스킬 입력은 아이콘보다 앞"),
					ButtonSlot->GetZOrder() > SocketSlot->GetZOrder());
			}
		}
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
	// 한글 표시 문자열을 단언하므로 ko 컬처로 고정한다. en/ko 번역이 모두
	// 채워진 뒤로는 실행 컬처에 따라 표시가 달라진다(0823).
	struct FScopedKoreanCulture
	{
		FString mOriginal;
		FScopedKoreanCulture()
			: mOriginal(FInternationalization::Get().GetCurrentCulture()->GetName())
		{
			FInternationalization::Get().SetCurrentCulture(TEXT("ko"));
		}
		~FScopedKoreanCulture()
		{
			FInternationalization::Get().SetCurrentCulture(mOriginal);
		}
	};
	const FScopedKoreanCulture ScopedKoreanCulture;
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
	UWidget* TurnAPScale = HUD->WidgetTree->FindWidget(TEXT("TurnAPScale"));
	UWidget* EnemyPanel = HUD->WidgetTree->FindWidget(TEXT("EnemyPanel"));
	UTextBlock* EnemyAPText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("EnemyAPText")));
	UButton* EnemyStatusButton0 = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("EnemyStatusButton_0")));
	UButton* EnemyStatusButton1 = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("EnemyStatusButton_1")));
	UWidget* AllyPanel = HUD->WidgetTree->FindWidget(TEXT("AllyPanel"));
	UTextBlock* AllyName = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("AllyName")));
	UWidget* InventoryPage = HUD->WidgetTree->FindWidget(
		TEXT("MercenaryInventoryPage"));
	UWidget* DetailSection = HUD->WidgetTree->FindWidget(TEXT("MercDetailSection"));
	UButton* MercenarySkillButton1 = Cast<UButton>(HUD->WidgetTree->FindWidget(
		TEXT("MercenarySkillButton_1")));
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
		|| !TestNotNull(TEXT("공용 AP 막대"), TurnAPScale)
		|| !TestNotNull(TEXT("몬스터 요약판"), EnemyPanel)
		|| !TestNotNull(TEXT("몬스터 요약판 AP"), EnemyAPText)
		|| !TestNotNull(TEXT("몬스터 첫 상태 상세 버튼"), EnemyStatusButton0)
		|| !TestNotNull(TEXT("몬스터 빈 상태 상세 버튼"), EnemyStatusButton1)
		|| !TestNotNull(TEXT("용병 요약판"), AllyPanel)
		|| !TestNotNull(TEXT("용병 요약판 이름"), AllyName)
		|| !TestNotNull(TEXT("용병 내부 인벤토리 페이지"), InventoryPage)
		|| !TestNotNull(TEXT("용병 상세 구역"), DetailSection)
		|| !TestNotNull(TEXT("용병 첫 스킬 탭 영역"), MercenarySkillButton1)
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
	TestTrue(TEXT("인벤토리 탭에 열기 동작이 묶여 있다"),
		InventoryButton->OnClicked.IsBound());
	TestTrue(TEXT("용병 스킬 탭에 상세 동작이 묶여 있다"),
		MercenarySkillButton1->OnClicked.IsBound());
	TestEqual(TEXT("전투 진입 때 용병 패널은 닫힘"),
		Panel->GetVisibility(), ESlateVisibility::Collapsed);

	UCombatUIModel* Model = NewObject<UCombatUIModel>(HUD);
	HUD->BindUIModel(Model);
	UMercenaryDetailTestResponder* DetailResponder =
		NewObject<UMercenaryDetailTestResponder>(HUD);
	DetailResponder->Bind(Model);
	// 몬스터와 플레이어 레일이 모두 0번 슬롯을 쓰되 수치는 전혀 다르게 둔다.
	// 상세 HUD가 index로 플레이어 레일을 다시 찾으면 아래 검증이 즉시 실패한다.
	const int32 MonsterSkillIndex = 0;
	const FText MonsterSkillName = FText::FromString(TEXT("거미 독니"));
	const FText MonsterSkillDescription = FText::FromString(
		TEXT("몬스터 스킬 상세가 탭 위에 떠야 한다."));
	FSkillDetailUI MonsterSkillDetail;
	MonsterSkillDetail.mSkillIndex = MonsterSkillIndex;
	MonsterSkillDetail.mName = MonsterSkillName;
	MonsterSkillDetail.mDescription = MonsterSkillDescription;
	MonsterSkillDetail.mActionPointCost = 9;
	MonsterSkillDetail.mActionPointGain = 4;
	MonsterSkillDetail.mCooldownTurns = 7;
	MonsterSkillDetail.mDamageMin = 41;
	MonsterSkillDetail.mDamageMax = 43;
	MonsterSkillDetail.mCriticalDamage = 65;
	MonsterSkillDetail.mTargeting.mSelectRange = 8.f;
	DetailResponder->ConfigureUnitSkillResponse(MonsterSkillDetail);

	FSkillUI PlayerRailSkill;
	PlayerRailSkill.mSkillIndex = MonsterSkillIndex;
	PlayerRailSkill.mActionPointCost = 2;
	PlayerRailSkill.mCooldownTurns = 3;
	PlayerRailSkill.mDamageMin = 6;
	PlayerRailSkill.mDamageMax = 10;
	PlayerRailSkill.mCriticalDamage = 15;
	PlayerRailSkill.mTargeting.mSelectRange = 1.f;
	Model->SetSkillUIs({ PlayerRailSkill });
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
	FStatusEffectUI MonsterStatus;
	MonsterStatus.mTag = FGameplayTag::RequestGameplayTag(
		TEXT("GameplayEffect.StatusEffect.RoundDuration.Debuff.Vulnerability"));
	MonsterStatus.mStackCount = 2;
	MonsterUnit.mStatusEffects.Add(MonsterStatus);
	Model->SetUnitUIs({ MonsterUnit });
	FTurnUI MonsterTurn;
	MonsterTurn.mCurrentUnitId = MonsterUnit.mUnitId;
	MonsterTurn.mTurnOrderUnitIds.Add(MonsterUnit.mUnitId);
	Model->SetTurnUI(MonsterTurn);
	TestEqual(TEXT("몬스터 차례에는 공용 AP 막대를 접는다"),
		TurnAPScale->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("몬스터 차례에는 몬스터 요약판을 보인다"),
		EnemyPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("몬스터 차례에는 겹친 용병 요약판을 접는다"),
		AllyPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("몬스터 AP는 요약판에서 읽는다"),
		EnemyAPText->GetText().ToString(), FString(TEXT("AP 3/5")));
	TestEqual(TEXT("상태가 있는 요약판 버튼만 입력 가능"),
		EnemyStatusButton0->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("빈 상태 소켓 버튼은 입력을 막지 않음"),
		EnemyStatusButton1->GetVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("상태 버튼 누름 배선"), EnemyStatusButton0->OnPressed.IsBound());
	TestTrue(TEXT("상태 버튼 뗌 배선"), EnemyStatusButton0->OnReleased.IsBound());

	// 짧은 탭은 상세를 열지 않고 타이머를 취소한다.
	EnemyStatusButton0->OnPressed.Broadcast();
	TestTrue(TEXT("상태 아이콘 누름은 롱프레스 후보를 예약"),
		HUD->IsStatusLongPressPendingForTest());
	TestTrue(TEXT("적 상태 0번이 현재 후보"),
		HUD->IsStatusPressActiveForTest(false, 0));
	EnemyStatusButton0->OnReleased.Broadcast();
	TestFalse(TEXT("짧게 떼면 상태 롱프레스 취소"),
		HUD->IsStatusLongPressPendingForTest());
	TestFalse(TEXT("짧은 탭은 상세를 열지 않음"), HUD->IsDetailOverlayShown());

	// 0.5초를 채운 것과 같은 테스트 발화는 기존 공용 상세 겹을 한 번 연다.
	EnemyStatusButton0->OnPressed.Broadcast();
	HUD->TriggerStatusLongPressForTest(false, 0);
	TestTrue(TEXT("상태 아이콘 롱프레스는 상세를 연다"),
		HUD->IsDetailOverlayShown());
	TestFalse(TEXT("상태 상세 발화 뒤 예약 타이머 제거"),
		HUD->IsStatusLongPressPendingForTest());
	// 뒤쪽 몬스터 탭 검증은 상세가 닫힌 기본 상태에서 시작한다. 항상 WBP가
	// 있는 용병 패널을 써서, 몬스터 탭 fallback 상세가 다시 열리는 경우를 피한다.
	MercenaryMenu->OnClicked.Broadcast();
	TestFalse(TEXT("다른 모달을 열면 상태 상세도 정리됨"),
		HUD->IsDetailOverlayShown());
	MercenaryMenu->OnClicked.Broadcast();
	// 0823 확정: AP 는 문구로만 남기고 보석 아이콘 행은 걷었다.
	if (UWidget* PipRow = HUD->WidgetTree->FindWidget(TEXT("EnemyAPPipRow")))
	{
		TestEqual(TEXT("몬스터 AP 보석 행은 통째로 접혀 있다"),
			PipRow->GetVisibility(), ESlateVisibility::Collapsed);
	}
	MonsterUnit.mActionPoints = 2;
	MonsterUnit.mMovementPoint = 2.f;
	Model->SetUnitUIs({ MonsterUnit });
	TestEqual(TEXT("몬스터의 실제 AP가 한 칸 줄면 숫자가 즉시 갱신된다"),
		EnemyAPText->GetText().ToString(), FString(TEXT("AP 2/5")));
	MonsterMenu->OnClicked.Broadcast();
	if (HUD->IsMonsterTabShown() == true)
	{
		// 몬스터 탭 WBP가 있는 환경: 탭이 열리고 선택 몬스터의 상세를 청한다.
		TestEqual(TEXT("몬스터 탭은 선택 몬스터를 InspectUnit으로 청한다"),
			DetailResponder->mLastPayload, MonsterUnit.mUnitId);
		TestFalse(TEXT("몬스터 탭이 열리면 PR457 상세 겹은 뜨지 않는다"),
			HUD->IsDetailOverlayShown());
		UUserWidget* MonsterTab = HUD->GetMonsterTabWidgetForTest();
		UButton* MonsterSkillButton0 = MonsterTab != nullptr
			? Cast<UButton>(MonsterTab->GetWidgetFromName(TEXT("MonsterSkillButton_0")))
			: nullptr;
		if (TestNotNull(TEXT("몬스터 첫 스킬 탭 받이"), MonsterSkillButton0))
		{
			TestEqual(TEXT("상세 viewport는 몬스터 탭보다 위"),
				HUD->GetDetailOverlayViewportZOrderForTest()
					> HUD->GetMonsterTabViewportZOrderForTest(), true);
			TestEqual(TEXT("실제 몬스터 스킬은 입력 가능"),
				MonsterSkillButton0->GetVisibility(), ESlateVisibility::Visible);
			TestTrue(TEXT("몬스터 스킬 클릭 배선"),
				MonsterSkillButton0->OnClicked.IsBound());
			TestEqual(TEXT("몬스터 스킬 터치는 드래그 이탈 시 취소"),
				MonsterSkillButton0->GetTouchMethod(), EButtonTouchMethod::PreciseTap);

			// 전투 용병 스킬과 동일하게 한 번 탭하면 즉시 실제 DTO index를
			// 보내고, 응답 상세를 몬스터 탭 위에 연다.
			MonsterSkillButton0->OnClicked.Broadcast();
			TestEqual(TEXT("한 번 탭한 몬스터 스킬 DTO index 왕복"),
				DetailResponder->mLastPayload, MonsterSkillIndex);
			TestEqual(TEXT("한 번 탭하면 몬스터 스킬 상세 요청 한 번"),
				DetailResponder->mInspectSkillRequestCount, 1);
			TestTrue(TEXT("몬스터 탭을 유지한 채 상세가 즉시 열림"),
				HUD->IsMonsterTabShown() && HUD->IsDetailOverlayShown());
			TestFalse(TEXT("일반 탭 경로에는 롱프레스 타이머가 없음"),
				HUD->IsMonsterSkillLongPressPendingForTest());
			if (UGameViewportSubsystem* Viewport = UGameViewportSubsystem::Get(World))
			{
				UUserWidget* Overlay = HUD->GetDetailOverlayWidgetForTest();
				if (TestNotNull(TEXT("몬스터 스킬 상세 겹"), Overlay)
					&& Viewport->IsWidgetAdded(Overlay)
					&& Viewport->IsWidgetAdded(MonsterTab))
				{
					TestEqual(TEXT("상세 겹 실제 Z-order"),
						Viewport->GetWidgetSlot(Overlay).ZOrder,
						HUD->GetDetailOverlayViewportZOrderForTest());
					TestEqual(TEXT("몬스터 탭 실제 Z-order"),
						Viewport->GetWidgetSlot(MonsterTab).ZOrder,
						HUD->GetMonsterTabViewportZOrderForTest());
				}
				else
				{
					AddInfo(TEXT("테스트 월드에 게임 viewport가 없어 실제 슬롯 Z 검증은 건너뜀"));
				}
			}
			TestEqual(TEXT("몬스터 스킬 상세 내용 왕복"),
				Model->GetSkillDetail().mDescription.ToString(),
				MonsterSkillDescription.ToString());
			const FSkillDetailUI& Rendered = HUD->GetRenderedSkillDetailForTest();
			TestEqual(TEXT("HUD가 소비한 몬스터 AP는 플레이어 레일 AP가 아님"),
				Rendered.mActionPointCost, 9);
			TestEqual(TEXT("HUD가 소비한 몬스터 AP 회복"),
				Rendered.mActionPointGain, 4);
			TestEqual(TEXT("HUD가 소비한 몬스터 피해 최소"),
				Rendered.mDamageMin, 41);
			TestEqual(TEXT("HUD가 소비한 몬스터 피해 최대"),
				Rendered.mDamageMax, 43);
			TestEqual(TEXT("HUD가 소비한 몬스터 쿨타임"),
				Rendered.mCooldownTurns, 7);
			TestEqual(TEXT("HUD가 소비한 몬스터 사거리"),
				Rendered.mTargeting.mSelectRange, 8.f);
			TestEqual(TEXT("HUD가 소비한 몬스터 치명 피해"),
				Rendered.mCriticalDamage, 65);
			TestEqual(TEXT("실제 몬스터 상세 AP 칩"),
				HUD->GetDetailChipValueForTest(0), FString(TEXT("9")));
			TestEqual(TEXT("실제 몬스터 상세 피해 칩"),
				HUD->GetDetailChipValueForTest(1), FString(TEXT("41~43")));
			TestEqual(TEXT("실제 몬스터 상세 쿨타임 칩"),
				HUD->GetDetailChipValueForTest(2), FString(TEXT("7턴")));
			TestEqual(TEXT("실제 몬스터 상세 치명 칩"),
				HUD->GetDetailChipValueForTest(4), FString(TEXT("65")));
			// 새 공용 스킬 WBP는 요약 띠를 숨기고 수치 메달과 전술 보드를 쓴다.
			// 위의 Rendered/칩 검증이 요청한 몬스터 DTO만 소비하는 계약이다.
			HUD->CloseDetailOverlayForTest();
			TestFalse(TEXT("몬스터 스킬 상세를 닫을 수 있음"),
				HUD->IsDetailOverlayShown());
		}
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
		TurnAPScale->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
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
	MercenarySkillButton1->OnClicked.Broadcast();
	TestEqual(TEXT("용병 스킬 한 번 탭은 카드 상세 경로를 사용한다"),
		DetailResponder->mLastType, ECombatInputType::LongPressSkill);
	TestEqual(TEXT("용병 첫 스킬 탭 payload"), DetailResponder->mLastPayload, 0);
	TestTrue(TEXT("용병 스킬 한 번 탭으로 상세가 열린다"),
		HUD->IsDetailOverlayShown());
	HUD->CloseDetailOverlayForTest();
	TestEqual(TEXT("용병 상세 면의 스킬 탭은 실제 터치 가능 상태"),
		MercenarySkillButton1->GetVisibility(), ESlateVisibility::Visible);
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
	UWidget* RoundPanel = Tree->FindWidget(TEXT("RoundPanel"));
	UTextBlock* RoundText = Cast<UTextBlock>(Tree->FindWidget(TEXT("RoundText")));
	UTextBlock* RoundNumberText = Cast<UTextBlock>(
		Tree->FindWidget(TEXT("RoundNumberText")));
	UImage* RoundPlate = Cast<UImage>(Tree->FindWidget(TEXT("RoundPlate")));
	UPanelWidget* RoundPlateMount =
		Cast<UPanelWidget>(Tree->FindWidget(TEXT("RoundPlateMount")));
	UImage* RoundNumberPlate = Cast<UImage>(
		Tree->FindWidget(TEXT("RoundNumberPlate")));
	UPanelWidget* RoundNumberPlateMount = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("RoundNumberPlateMount")));
	UPanelWidget* RoundNumberTextCenter = Cast<UPanelWidget>(
		Tree->FindWidget(TEXT("RoundNumberText_Center")));
	UWidget* ObjectiveTextCenter = Tree->FindWidget(TEXT("ObjectiveText_Center"));
	UCanvasPanelSlot* RoundPanelSlot = RoundPanel != nullptr
		? Cast<UCanvasPanelSlot>(RoundPanel->Slot) : nullptr;
	if (TestNotNull(TEXT("현재 라운드 독립 패널"), RoundPanel)
		&& TestNotNull(TEXT("현재 라운드 패널 슬롯"), RoundPanelSlot))
	{
		TestEqual(TEXT("라운드 패널 위치"), RoundPanelSlot->GetPosition(),
			FVector2D(18.f, 10.f));
		TestEqual(TEXT("라운드 패널 크기"), RoundPanelSlot->GetSize(),
			FVector2D(218.f, 136.f));
		TestEqual(TEXT("라운드 패널 좌상단 앵커 최소"),
			RoundPanelSlot->GetAnchors().Minimum, FVector2D::ZeroVector);
		TestEqual(TEXT("라운드 패널 좌상단 앵커 최대"),
			RoundPanelSlot->GetAnchors().Maximum, FVector2D::ZeroVector);
	}
	TestNotNull(TEXT("현재 라운드 독립 텍스트"), RoundText);
	if (TestNotNull(TEXT("현재 라운드 두 자리 숫자"), RoundNumberText))
	{
		UScaleBox* RoundNumberAutoFit = Cast<UScaleBox>(
			Tree->FindWidget(TEXT("RoundNumberText_AutoFit")));
		if (TestNotNull(TEXT("라운드 숫자 자동 축소 래퍼"), RoundNumberAutoFit))
		{
			TestEqual(TEXT("라운드 숫자 래퍼 부모"),
				RoundNumberAutoFit->GetParent(), RoundNumberTextCenter);
			TestEqual(TEXT("라운드 숫자 텍스트 부모"),
				RoundNumberText->GetParent(),
				static_cast<UPanelWidget*>(RoundNumberAutoFit));
		}
		TestEqual(TEXT("라운드 숫자는 두 자리 미리보기"),
			RoundNumberText->GetText().ToString(), FString(TEXT("01")));
	}
	if (TestNotNull(TEXT("라운드 배지 내부 래퍼"), RoundPlateMount))
	{
		TestEqual(TEXT("라운드 배지 래퍼 부모"), RoundPlateMount->GetParent(),
			Cast<UPanelWidget>(RoundPanel));
		if (UCanvasPanelSlot* PlateMountSlot =
			Cast<UCanvasPanelSlot>(RoundPlateMount->Slot))
		{
			TestEqual(TEXT("라운드 배지 래퍼 위치"),
				PlateMountSlot->GetPosition(), FVector2D::ZeroVector);
			TestEqual(TEXT("라운드 배지 래퍼 크기"),
				PlateMountSlot->GetSize(), FVector2D(218.f, 68.f));
		}
	}
	if (TestNotNull(TEXT("새 라운드 배지 그림"), RoundPlate))
	{
		TestEqual(TEXT("라운드 배지 그림 부모"), RoundPlate->GetParent(),
			RoundPlateMount);
		UObject* RoundResource = RoundPlate->GetBrush().GetResourceObject();
		if (TestNotNull(TEXT("새 라운드 배지 텍스처"), RoundResource))
		{
			TestEqual(TEXT("새 라운드 배지 텍스처 경로"),
				RoundResource->GetPathName(), FString(
					TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Combat/"
						"T_MB_RoundBadge_Frame.T_MB_RoundBadge_Frame")));
		}
	}
	if (TestNotNull(TEXT("라운드 숫자 배지 래퍼"), RoundNumberPlateMount))
	{
		TestEqual(TEXT("라운드 숫자 배지 래퍼 부모"),
			RoundNumberPlateMount->GetParent(), Cast<UPanelWidget>(RoundPanel));
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(
			RoundNumberPlateMount->Slot))
		{
			TestEqual(TEXT("라운드 숫자 배지는 ROUND 바로 아래"),
				Slot->GetPosition(), FVector2D(0.f, 68.f));
			TestEqual(TEXT("두 라운드 배지는 같은 크기"), Slot->GetSize(),
				FVector2D(218.f, 68.f));
		}
	}
	if (TestNotNull(TEXT("라운드 숫자 배지 그림"), RoundNumberPlate)
		&& RoundPlate != nullptr)
	{
		TestEqual(TEXT("ROUND와 숫자는 같은 에셋"),
			RoundNumberPlate->GetBrush().GetResourceObject(),
			RoundPlate->GetBrush().GetResourceObject());
	}
	if (ObjectiveTextCenter != nullptr)
	{
		TestEqual(TEXT("구형 임무 문구는 라운드 아래에 표시하지 않음"),
			ObjectiveTextCenter->GetVisibility(), ESlateVisibility::Collapsed);
	}
	TestNull(TEXT("구형 빌더의 검정 용병 바탕은 없어야 함"),
		Tree->FindWidget(TEXT("MercenaryContentWell")));
	UCanvasPanel* TurnPanel = Cast<UCanvasPanel>(
		Tree->FindWidget(TEXT("TurnPanel")));
	UCanvasPanelSlot* TurnPanelSlot = TurnPanel != nullptr
		? Cast<UCanvasPanelSlot>(TurnPanel->Slot) : nullptr;
	if (TestNotNull(TEXT("사용자 배치 턴바 슬롯"), TurnPanelSlot))
	{
		TestEqual(TEXT("턴바 사용자 배치 X"),
			double(TurnPanelSlot->GetPosition().X), 246.0);
		TestEqual(TEXT("턴바 사용자 배치 Y"),
			double(TurnPanelSlot->GetPosition().Y), 10.0);
		TestEqual(TEXT("턴바 사용자 배치 크기"),
			TurnPanelSlot->GetSize(), FVector2D(1090.f, 150.f));
		TestEqual(TEXT("턴바 좌상단 앵커 최소"),
			TurnPanelSlot->GetAnchors().Minimum, FVector2D::ZeroVector);
		TestEqual(TEXT("턴바 좌상단 앵커 최대"),
			TurnPanelSlot->GetAnchors().Maximum, FVector2D::ZeroVector);
	}

	for (int32 Index = 0; Index < UCombatLayoutHUDWidget::TurnSlotCount; ++Index)
	{
		const FString TokenName = FString::Printf(TEXT("TurnToken_%d"), Index);
		const FString CropName = FString::Printf(TEXT("TurnPortraitCrop_%d"), Index);
		const FString PortraitName = FString::Printf(TEXT("TurnPortrait_%d"), Index);
		const FString CurrentName = FString::Printf(TEXT("TurnCurrent_%d"), Index);
		const FString ButtonName =
			FString::Printf(TEXT("TurnTokenButton_%d"), Index);
		const FString DividerName =
			FString::Printf(TEXT("TurnRoundDivider_%d"), Index);
		const FString DividerMountName =
			FString::Printf(TEXT("TurnRoundDivider_%dMount"), Index);
		const FString LabelName =
			FString::Printf(TEXT("TurnRoundLabel_%d"), Index);
		const FString LabelCenterName =
			FString::Printf(TEXT("TurnRoundLabel_%d_Center"), Index);
		const FString FrameName =
			FString::Printf(TEXT("TurnFrame_%d"), Index);

		UPanelWidget* Token =
			Cast<UPanelWidget>(Tree->FindWidget(FName(*TokenName)));
		UScaleBox* Crop =
			Cast<UScaleBox>(Tree->FindWidget(FName(*CropName)));
		UWidget* Portrait = Tree->FindWidget(FName(*PortraitName));
		UWidget* Current = Tree->FindWidget(FName(*CurrentName));
		UButton* Button = Cast<UButton>(Tree->FindWidget(FName(*ButtonName)));
		UWidget* Divider = Tree->FindWidget(FName(*DividerName));
		UPanelWidget* DividerMount =
			Cast<UPanelWidget>(Tree->FindWidget(FName(*DividerMountName)));
		UTextBlock* RoundLabel =
			Cast<UTextBlock>(Tree->FindWidget(FName(*LabelName)));
		UPanelWidget* LabelCenter =
			Cast<UPanelWidget>(Tree->FindWidget(FName(*LabelCenterName)));
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
			if (UCanvasPanelSlot* TokenSlot = Cast<UCanvasPanelSlot>(Token->Slot))
			{
				TestEqual(*FString::Printf(TEXT("%s 위치"), *TokenName),
					TokenSlot->GetPosition(), FVector2D(5.f + 109.f * Index, 30.f));
				TestEqual(*FString::Printf(TEXT("%s 크기"), *TokenName),
					TokenSlot->GetSize(), FVector2D(108.f, 120.f));
			}
		}
		if (TestNotNull(*ButtonName, Button))
		{
			TestEqual(*FString::Printf(TEXT("%s는 턴바 직계 자식"), *ButtonName),
				Button->GetParent(), static_cast<UPanelWidget*>(TurnPanel));
			if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Button->Slot))
			{
				TestEqual(*FString::Printf(TEXT("%s 위치"), *ButtonName),
					ButtonSlot->GetPosition(), FVector2D(5.f + 109.f * Index, 30.f));
				TestEqual(*FString::Printf(TEXT("%s 크기"), *ButtonName),
					ButtonSlot->GetSize(), FVector2D(108.f, 120.f));
			}
		}
		if (TestNotNull(*DividerName, Divider))
		{
			TestEqual(*FString::Printf(TEXT("%s 기본 숨김"), *DividerName),
				Divider->GetVisibility(), ESlateVisibility::Collapsed);
			TestEqual(*FString::Printf(TEXT("%s 내부 배지 부모"), *DividerName),
				Divider->GetParent(), DividerMount);
		}
		if (TestNotNull(*DividerMountName, DividerMount))
		{
			TestEqual(*FString::Printf(TEXT("%s는 턴바 직계 자식"),
				*DividerMountName), DividerMount->GetParent(),
				static_cast<UPanelWidget*>(TurnPanel));
			if (UCanvasPanelSlot* MountSlot =
				Cast<UCanvasPanelSlot>(DividerMount->Slot))
			{
				TestEqual(*FString::Printf(TEXT("%s 위치"), *DividerMountName),
					MountSlot->GetPosition(), FVector2D(5.f + 109.f * Index, 0.f));
				TestEqual(*FString::Printf(TEXT("%s 크기"), *DividerMountName),
					MountSlot->GetSize(), FVector2D(108.f, 34.f));
			}
		}
		if (TestNotNull(*LabelName, RoundLabel))
		{
			TestEqual(*FString::Printf(TEXT("%s 기본 숨김"), *LabelName),
				RoundLabel->GetVisibility(), ESlateVisibility::Collapsed);
			UScaleBox* LabelAutoFit = Cast<UScaleBox>(Tree->FindWidget(
				FName(*FString::Printf(TEXT("%s_AutoFit"), *LabelName))));
			if (TestNotNull(*FString::Printf(TEXT("%s 자동 축소 래퍼"),
				*LabelName), LabelAutoFit))
			{
				TestEqual(*FString::Printf(TEXT("%s 중앙 영역"), *LabelName),
					LabelAutoFit->GetParent(), LabelCenter);
				TestEqual(*FString::Printf(TEXT("%s 자동 축소 내부"), *LabelName),
					RoundLabel->GetParent(),
					static_cast<UPanelWidget*>(LabelAutoFit));
			}
		}
		if (TestNotNull(*LabelCenterName, LabelCenter))
		{
			TestEqual(*FString::Printf(TEXT("%s 배지 래퍼"), *LabelCenterName),
				LabelCenter->GetParent(), DividerMount);
		}
		for (const FString& RemovedName : {
			FString::Printf(TEXT("TurnSpeed_%d"), Index),
			FString::Printf(TEXT("TurnSpeed_%d_Center"), Index),
			FString::Printf(TEXT("TurnSpeedIcon_%d"), Index),
			FString::Printf(TEXT("TurnSpeedPlate_%d"), Index) })
		{
			TestNull(*FString::Printf(TEXT("%s는 WBP에서 완전 제거"),
				*RemovedName), Tree->FindWidget(FName(*RemovedName)));
		}
		if (TestNotNull(*FrameName, Frame))
		{
			TestEqual(*FString::Printf(TEXT("%s 부모"), *FrameName),
				Frame->GetParent(), Token);
			UTexture2D* Texture =
				Cast<UTexture2D>(Frame->GetBrush().GetResourceObject());
			if (TestNotNull(*FString::Printf(TEXT("%s 텍스처"), *FrameName),
				Texture))
			{
				TestEqual(*FString::Printf(TEXT("%s 하단 없는 프레임 경로"),
					*FrameName), Texture->GetPathName(), FString(TEXT(
						"/Game/UI/Generated/CombatHUD/"
						"T_MB_TurnToken_Frame_NoSpeed_v1."
						"T_MB_TurnToken_Frame_NoSpeed_v1")));
			}
		}
	}

	TestNotNull(TEXT("왼쪽 넘김 버튼"),
		Tree->FindWidget(TEXT("TurnPageLeft")));
	TestNotNull(TEXT("오른쪽 넘김 버튼"),
		Tree->FindWidget(TEXT("TurnPageRight")));
	int32 CenterPairCount = 0;
	Tree->ForEachWidget([this, Tree, &CenterPairCount](UWidget* Widget)
	{
		UTextBlock* Text = Cast<UTextBlock>(Widget);
		if (Text == nullptr)
		{
			return;
		}
		UOverlay* Center = Cast<UOverlay>(Tree->FindWidget(FName(*FString::Printf(
			TEXT("%s_Center"), *Text->GetName()))));
		if (Center == nullptr)
		{
			return;
		}
		UWidget* CenterChild = Text;
		while (CenterChild != nullptr && CenterChild->GetParent() != Center)
		{
			CenterChild = CenterChild->GetParent();
		}
		if (!TestNotNull(*FString::Printf(TEXT("%s는 동명 Center 내부"),
			*Text->GetName()), CenterChild))
		{
			return;
		}
		UScaleBox* AutoFit = Cast<UScaleBox>(CenterChild);
		if (TestNotNull(*FString::Printf(TEXT("%s 자동 축소 래퍼"),
			*Text->GetName()), AutoFit))
		{
			TestEqual(*FString::Printf(TEXT("%s 가로 넘침만 축소"),
				*Text->GetName()), AutoFit->GetStretch(), EStretch::ScaleToFitX);
			TestEqual(*FString::Printf(TEXT("%s 원래 크기보다 확대하지 않음"),
				*Text->GetName()), AutoFit->GetStretchDirection(),
				EStretchDirection::DownOnly);
			UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Text->Slot);
			if (TestNotNull(*FString::Printf(TEXT("%s AutoFit 내부 슬롯"),
				*Text->GetName()), TextSlot))
			{
				TestEqual(*FString::Printf(TEXT("%s 글자 가로 중앙"),
					*Text->GetName()), TextSlot->GetHorizontalAlignment(),
					HAlign_Center);
				TestEqual(*FString::Printf(TEXT("%s 글자 세로 중앙"),
					*Text->GetName()), TextSlot->GetVerticalAlignment(),
					VAlign_Center);
			}
		}
		UOverlaySlot* Slot = Cast<UOverlaySlot>(CenterChild->Slot);
		if (TestNotNull(*FString::Printf(TEXT("%s Center 슬롯"),
			*Text->GetName()), Slot))
		{
			TestEqual(*FString::Printf(TEXT("%s Center와 같은 가로 영역"),
				*Text->GetName()), Slot->GetHorizontalAlignment(), HAlign_Fill);
			TestEqual(*FString::Printf(TEXT("%s Center와 같은 세로 영역"),
				*Text->GetName()), Slot->GetVerticalAlignment(), VAlign_Fill);
			TestEqual(*FString::Printf(TEXT("%s Center 내부 여백 없음"),
				*Text->GetName()), Slot->GetPadding(), FMargin(0.f));
		}
		TestEqual(*FString::Printf(TEXT("%s 개별 좌표 보정 없음"),
			*Text->GetName()), Text->GetRenderTransform().Translation,
			FVector2D::ZeroVector);
		++CenterPairCount;
	});
	TestTrue(TEXT("전투 HUD Center/Text 계약을 실제로 전체 순회함"),
		CenterPairCount >= 10);
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
		// 적 슬롯 클릭 시 대상 선택까지 실제로 왕복하는지 아래에서 확인한다.
		Unit.mIsPlayer = false;
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
		UButton* TokenButton = Cast<UButton>(HUD->WidgetTree->FindWidget(FName(
			*FString::Printf(TEXT("TurnTokenButton_%d"), Index))));
		if (TestNotNull(*FString::Printf(TEXT("턴 칸 %d"), Index), Token))
		{
			TestEqual(*FString::Printf(TEXT("턴 칸 %d 표시"), Index),
				Token->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
			TestEqual(*FString::Printf(TEXT("턴 칸 %d 라운드 투명도"), Index),
				Token->GetRenderOpacity(), 1.f);
		}
		if (TestNotNull(*FString::Printf(TEXT("턴 칸 버튼 %d"), Index),
			TokenButton))
		{
			TestEqual(*FString::Printf(TEXT("턴 칸 버튼 %d 표시"), Index),
				TokenButton->GetVisibility(), ESlateVisibility::Visible);
			TestTrue(*FString::Printf(TEXT("턴 칸 버튼 %d 클릭 배선"), Index),
				TokenButton->OnClicked.IsBound());
		}
	}
	if (UButton* SecondTokenButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("TurnTokenButton_1"))))
	{
		SecondTokenButton->OnClicked.Broadcast();
		TestTrue(TEXT("턴 칸 클릭은 해당 유닛을 대상으로 잡음"),
			Model->GetTarget().mIsValid);
		TestEqual(TEXT("턴 칸 클릭 대상 ID"), Model->GetTarget().mUnitId,
			Units[1].mUnitId);
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
	TestNull(TEXT("런타임에도 턴 속도 위젯 없음"),
		HUD->WidgetTree->FindWidget(TEXT("TurnSpeed_0")));
	UWidget* CurrentRoundDivider =
		HUD->WidgetTree->FindWidget(TEXT("TurnRoundDivider_0"));
	UWidget* CurrentRoundLabel =
		HUD->WidgetTree->FindWidget(TEXT("TurnRoundLabel_0"));
	if (TestNotNull(TEXT("현재 슬롯 라운드 배지"), CurrentRoundDivider))
	{
		TestEqual(TEXT("현재 라운드는 독립 패널만 사용"),
			CurrentRoundDivider->GetVisibility(), ESlateVisibility::Collapsed);
	}
	if (TestNotNull(TEXT("현재 슬롯 라운드 라벨"), CurrentRoundLabel))
	{
		TestEqual(TEXT("현재 슬롯 R# 중복 방지"),
			CurrentRoundLabel->GetVisibility(), ESlateVisibility::Collapsed);
	}
	UWidget* RoundPanel = HUD->WidgetTree->FindWidget(TEXT("RoundPanel"));
	UTextBlock* RoundText = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("RoundText")));
	if (TestNotNull(TEXT("독립 라운드 패널"), RoundPanel))
	{
		TestEqual(TEXT("독립 라운드 패널 표시"), RoundPanel->GetVisibility(),
			ESlateVisibility::SelfHitTestInvisible);
	}
	if (TestNotNull(TEXT("독립 라운드 텍스트"), RoundText))
	{
		TestEqual(TEXT("배지는 ROUND 글자만 표시"),
			RoundText->GetText().ToString(), FString(TEXT("ROUND")));
	}
	// 0823 확정: 배지 아래 두 자리 숫자 칸에도 같은 라운드가 나온다.
	UTextBlock* RoundNumber = Cast<UTextBlock>(
		HUD->WidgetTree->FindWidget(TEXT("RoundNumberText")));
	if (TestNotNull(TEXT("독립 라운드 숫자"), RoundNumber))
	{
		TestEqual(TEXT("라운드 두 자리 숫자 표시"),
			RoundNumber->GetText().ToString(), FString(TEXT("03")));
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

	// 클릭 받이는 토큰의 자식이 아니므로, 빈 라운드 칸과 범위 밖 칸도 별도로
	// 접어야 한다. 앞/뒤의 실제 유닛 칸만 입력을 받는다.
	Turn.mTurnOrderUnitIds = { Units[0].mUnitId };
	Turn.mNextRoundUnitIds = { Units[1].mUnitId };
	Turn.mCurrentUnitId = Units[0].mUnitId;
	Turn.mRound = 4;
	Turn.mCurrentRoundRemainingTurnCount = 1;
	Turn.mNextRoundOffset = 2;
	Model->SetTurnUI(Turn);
	const ESlateVisibility ExpectedButtonVisibility[] = {
		ESlateVisibility::Visible, ESlateVisibility::Collapsed,
		ESlateVisibility::Visible, ESlateVisibility::Collapsed,
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedButtonVisibility); ++Index)
	{
		UButton* Button = Cast<UButton>(HUD->WidgetTree->FindWidget(FName(
			*FString::Printf(TEXT("TurnTokenButton_%d"), Index))));
		if (TestNotNull(*FString::Printf(TEXT("희소 턴 버튼 %d"), Index), Button))
		{
			TestEqual(*FString::Printf(TEXT("희소 턴 버튼 %d 표시"), Index),
				Button->GetVisibility(), ExpectedButtonVisibility[Index]);
		}
	}
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
	UMercenaryDetailTestResponder* Responder =
		NewObject<UMercenaryDetailTestResponder>(HUD);
	Responder->Bind(Model);
	FVector2D LastFocusAnchor(-1.f, -1.f);
	int32 FocusAnchorCount = 0;
	const FDelegateHandle FocusAnchorHandle =
		Model->OnChangeFocusScreenAnchor.AddLambda(
			[&LastFocusAnchor, &FocusAnchorCount](const FVector2D& Anchor)
			{
				LastFocusAnchor = Anchor;
				++FocusAnchorCount;
			});
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

	// 아군 차례가 시작되면 해당 용병의 카드가 먼저 열린다. 아군 칸은
	// 뒤집기가 아니라 열린 카드를 그 용병 것으로 유지하는 손이다.
	//
	// 같은 아군 칸을 몇 번 눌러도 접히지 않는다 -- 스킬을 보러 눌렀는데
	// 접히면 아무 일도 안 일어난 것처럼 보인다.
	TestEqual(TEXT("아군 턴 시작에는 카드가 자동으로 펴진다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("아군 턴 시작은 현재 용병 포커스를 요청한다"),
		Responder->mLastType, ECombatInputType::FocusUnit);
	TestEqual(TEXT("아군 턴 포커스 대상은 현재 용병이다"),
		Responder->mLastPayload, PlayerUnit.mUnitId);
	TestEqual(TEXT("아군 턴 시작은 화면 앵커를 한 번만 보낸다"),
		FocusAnchorCount, 1);
	TestEqual(TEXT("아군 턴 포커스는 커맨드 고리 중심을 쓴다"),
		LastFocusAnchor, HUD->GetCommandRingAnchorForTest());
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("누르면 카드가 펴진다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	PartyButton->OnClicked.Broadcast();
	TestEqual(TEXT("다시 눌러도 접히지 않는다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	Model->OnEndAnyTurn.Broadcast(nullptr);
	Model->OnChangeFocusScreenAnchor.Remove(FocusAnchorHandle);
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
	UMercenaryDetailTestResponder* Responder =
		NewObject<UMercenaryDetailTestResponder>(HUD);
	Responder->Bind(Model);
	FVector2D LastFocusAnchor(-1.f, -1.f);
	int32 FocusAnchorCount = 0;
	const FDelegateHandle FocusAnchorHandle =
		Model->OnChangeFocusScreenAnchor.AddLambda(
			[&LastFocusAnchor, &FocusAnchorCount](const FVector2D& Anchor)
			{
				LastFocusAnchor = Anchor;
				++FocusAnchorCount;
			});

	FUnitUI PlayerUnit;
	PlayerUnit.mUnitId = 101;
	PlayerUnit.mIsPlayer = true;
	FUnitUI EnemyUnit;
	EnemyUnit.mUnitId = 202;
	EnemyUnit.mIsPlayer = false;
	Model->SetUnitUIs({ PlayerUnit, EnemyUnit });

	FTurnUI PlayerTurn;
	PlayerTurn.mCurrentUnitId = PlayerUnit.mUnitId;
	PlayerTurn.mTurnOrderUnitIds.Add(PlayerUnit.mUnitId);
	Model->SetTurnUI(PlayerTurn);
	Model->OnBeginAnyTurn.Broadcast(nullptr);

	UWidget* Card = HUD->WidgetTree->FindWidget(TEXT("CommandCard_0"));
	UButton* SkillButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("SkillToggleButton")));
	UButton* ConfirmButton = Cast<UButton>(
		HUD->WidgetTree->FindWidget(TEXT("ConfirmButton")));
	if (!TestNotNull(TEXT("전투 UI 모델"), Model)
		|| !TestNotNull(TEXT("명령 카드"), Card)
		|| !TestNotNull(TEXT("스킬 단추"), SkillButton)
		|| !TestNotNull(TEXT("확정 단추"), ConfirmButton))
	{
		return false;
	}

	// 아군 차례가 열리면 현재 용병의 카드를 자동으로 보여 준다.
	TestEqual(TEXT("플레이어 턴 시작에 카드가 자동으로 펴진다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("플레이어 턴 시작은 현재 용병 포커스를 요청한다"),
		Responder->mLastType, ECombatInputType::FocusUnit);
	TestEqual(TEXT("플레이어 턴 포커스 대상"),
		Responder->mLastPayload, PlayerUnit.mUnitId);
	TestEqual(TEXT("플레이어 턴 시작은 화면 앵커를 한 번만 보낸다"),
		FocusAnchorCount, 1);
	TestEqual(TEXT("플레이어 턴은 커맨드 고리 중심에 포커스한다"),
		LastFocusAnchor, HUD->GetCommandRingAnchorForTest());

	// 확정 버튼은 조준 중 표시 여부와 별개로 클릭 델리게이트가 모델의
	// Confirm 의도로 이어져야 한다. 바인딩이 빠지면 실제 전투에서 무반응이다.
	Responder->mLastType = ECombatInputType::Cancel;
	ConfirmButton->OnClicked.Broadcast();
	TestEqual(TEXT("확정 단추가 전투 Confirm 명령을 보낸다"),
		Responder->mLastType, ECombatInputType::Confirm);

	// 자동 표시 뒤에도 스킬 단추는 수동 접기/다시 열기의 역할을 유지한다.
	SkillButton->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 단추로 자동 카드를 접을 수 있다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);
	SkillButton->OnClicked.Broadcast();
	TestEqual(TEXT("스킬 단추로 다시 편다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);

	Model->OnEndAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("턴 종료 알림 즉시 카드를 내린다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);

	Model->OnBeginAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("다음 플레이어 턴 시작에도 다시 편다"), Card->GetVisibility(),
		ESlateVisibility::SelfHitTestInvisible);

	Model->OnBeginAnyTurnAction.Broadcast(nullptr);
	TestEqual(TEXT("행동 시작부터 카드를 감춘다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);

	Model->OnEndAnyTurnAction.Broadcast(nullptr);
	TestEqual(TEXT("행동 종료와 후속 행동 사이에는 즉시 다시 보이지 않는다"),
		Card->GetVisibility(), ESlateVisibility::Collapsed);

	// 다음 틱 예약이 시험 뒤 다른 상태를 건드리지 않게 턴 종료로 무효화한다.
	Model->OnEndAnyTurn.Broadcast(nullptr);

	// 적 턴은 PR #511의 전투 서브시스템이 월드 카메라를 맡는다. HUD는 아군
	// 커맨드와 카드 고리 앵커/FocusUnit 요청을 내보내면 안 된다.
	Responder->mLastType = ECombatInputType::Cancel;
	Responder->mLastPayload = INDEX_NONE;
	FocusAnchorCount = 0;
	FTurnUI EnemyTurn;
	EnemyTurn.mCurrentUnitId = EnemyUnit.mUnitId;
	EnemyTurn.mTurnOrderUnitIds.Add(EnemyUnit.mUnitId);
	Model->SetTurnUI(EnemyTurn);
	Model->OnBeginAnyTurn.Broadcast(nullptr);
	TestEqual(TEXT("몬스터 턴에는 아군 카드를 접는다"), Card->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("몬스터 턴에는 HUD FocusUnit 요청을 보내지 않는다"),
		Responder->mLastType, ECombatInputType::Cancel);
	TestEqual(TEXT("몬스터 턴에는 카드 고리 앵커를 보내지 않는다"),
		FocusAnchorCount, 0);
	Model->OnEndAnyTurn.Broadcast(nullptr);
	Model->OnChangeFocusScreenAnchor.Remove(FocusAnchorHandle);
	return true;
}

#endif
