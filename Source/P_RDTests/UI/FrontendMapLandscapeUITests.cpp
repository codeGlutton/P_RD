#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include "UI/FrontendMapLandscapeWidget.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/RunOptionsRailWidget.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFrontendMapLandscapeStructureTest,
	"P_RD.UI.FrontendMap.LandscapeStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFrontendMapConnectionLineModeTest,
	"P_RD.UI.FrontendMap.ConnectionLineModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFrontendMapLandscapeStructureTest::RunTest(const FString& Parameters)
{
	UClass* MapClass = LoadClass<UFrontendMapLandscapeWidget>(nullptr,
		TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape."
			"WBP_FrontendMapLandscape_C"));
	if (!TestNotNull(TEXT("가로형 월드맵 WBP 클래스"), MapClass))
	{
		return false;
	}
	TestTrue(TEXT("가로형 전용 C++ 부모"),
		MapClass->IsChildOf(UFrontendMapLandscapeWidget::StaticClass()));
	const UFrontendMapLandscapeWidget* MapDefaults =
		Cast<UFrontendMapLandscapeWidget>(MapClass->GetDefaultObject());
	if (!TestNotNull(TEXT("가로형 월드맵 CDO"), MapDefaults))
	{
		return false;
	}
	UClass* LandscapeLineClass = MapDefaults->GetLandscapeLineWidgetClass().Get();
	UClass* LandscapeNodeClass = MapDefaults->GetLandscapeNodeWidgetClass().Get();
	if (!TestNotNull(TEXT("메인 WBP의 연결선 하드 클래스 참조"), LandscapeLineClass)
		|| !TestNotNull(TEXT("메인 WBP의 노드 하드 클래스 참조"), LandscapeNodeClass))
	{
		return false;
	}
	TestEqual(TEXT("메인 WBP의 연결선 클래스 경로"), LandscapeLineClass->GetPathName(),
		FString(TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscapeLine."
			"WBP_FrontendMapLandscapeLine_C")));
	TestEqual(TEXT("메인 WBP의 노드 클래스 경로"), LandscapeNodeClass->GetPathName(),
		FString(TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscapeNode."
			"WBP_FrontendMapLandscapeNode_C")));
	TestTrue(TEXT("연결선 하드 참조의 C++ 부모"),
		LandscapeLineClass->IsChildOf(UFrontendMapLandscapeLineWidget::StaticClass()));
	TestTrue(TEXT("노드 하드 참조의 C++ 부모"),
		LandscapeNodeClass->IsChildOf(UFrontendMapLandscapeNodeWidget::StaticClass()));

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	UFrontendMapLandscapeWidget* Map = CreateWidget<UFrontendMapLandscapeWidget>(World, MapClass);
	if (!TestNotNull(TEXT("가로형 월드맵 인스턴스"), Map))
	{
		return false;
	}
	Map->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TSharedPtr<SWidget> MapSlate = Map->TakeWidget();
	if (!TestTrue(TEXT("월드맵 Slate 생명주기 유지"), MapSlate.IsValid()))
	{
		return false;
	}
	URunOptionsRailWidget* OptionsRail = Map->GetRunOptionsRailForTest();
	TestNotNull(TEXT("지도에 공용 설정바 WBP 생성"), OptionsRail);
	if (OptionsRail != nullptr)
	{
		OptionsRail->TakeWidget();
		TestTrue(TEXT("지도 버튼은 현재 화면으로 표시"),
			OptionsRail->IsMapContextForTest());
		TestEqual(TEXT("열린 지도에서는 별도 설정바 자식만 입력 허용"),
			OptionsRail->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		UWidget* RailRoot = OptionsRail->GetWidgetFromName(TEXT("RunOptionsRailRoot"));
		UWidget* RailCanvas = OptionsRail->GetWidgetFromName(TEXT("RunOptionsRailCanvas"));
		TestNotNull(TEXT("설정바 전체 화면 루트"), RailRoot);
		TestNotNull(TEXT("설정바 우상단 Canvas"), RailCanvas);
		if (RailRoot != nullptr)
		{
			TestEqual(TEXT("전체 화면 루트는 지도 입력을 통과"),
				RailRoot->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		}
		if (RailCanvas != nullptr)
		{
			TestEqual(TEXT("설정바 Canvas는 버튼 자식만 입력"),
				RailCanvas->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		}
		Map->SetVisibility(ESlateVisibility::Collapsed);
		TestEqual(TEXT("BACK으로 지도 본체를 닫으면 별도 설정바도 숨김"),
			OptionsRail->GetVisibility(), ESlateVisibility::Collapsed);
		Map->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TestEqual(TEXT("지도를 다시 열면 별도 설정바도 복구"),
			OptionsRail->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		TestNotNull(TEXT("설정바 용병/인벤토리 버튼"),
			OptionsRail->GetWidgetFromName(TEXT("MenuButton_1")));
		TestNotNull(TEXT("설정바 설정 버튼"),
			OptionsRail->GetWidgetFromName(TEXT("MenuButton_3")));
	}

	UWidgetTree* Tree = Map->WidgetTree;
	if (!TestNotNull(TEXT("월드맵 위젯 나무"), Tree))
	{
		return false;
	}

	USizeBox* GraphSize = Cast<USizeBox>(Tree->FindWidget(TEXT("MapGraphSize")));
	UCanvasPanel* GraphCanvas = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("MapGraphCanvas")));
	UBorder* Backdrop = Cast<UBorder>(Tree->FindWidget(TEXT("LandscapeMapBackdrop")));
	UImage* Background = Cast<UImage>(Tree->FindWidget(TEXT("Map_ParchmentBody")));
	UOverlay* TitleArea = Cast<UOverlay>(Tree->FindWidget(TEXT("MapTitleArea")));
	UTextBlock* TitleText = Cast<UTextBlock>(Tree->FindWidget(TEXT("MapTitleText")));
	UButton* EnterButton = Cast<UButton>(Tree->FindWidget(TEXT("EnterRoomButton")));
	UButton* CloseButton = Cast<UButton>(Tree->FindWidget(TEXT("CloseButton")));
	UWidget* Legend = Tree->FindWidget(TEXT("Map_LegendGroup"));
	UWidget* NodeArea = Tree->FindWidget(TEXT("Map_NodeArea"));

	TestNotNull(TEXT("고정 디자인 크기"), GraphSize);
	TestNotNull(TEXT("가로 그래프 Canvas"), GraphCanvas);
	TestNotNull(TEXT("화면비 여백 스크림"), Backdrop);
	TestNotNull(TEXT("가로 지도 배경"), Background);
	TestNotNull(TEXT("제목 중앙정렬 영역"), TitleArea);
	TestNotNull(TEXT("지도 제목"), TitleText);
	TestNotNull(TEXT("입장 버튼"), EnterButton);
	TestNotNull(TEXT("닫기 버튼"), CloseButton);
	TestNotNull(TEXT("좌측 범례"), Legend);
	TestNotNull(TEXT("가로 노드 배치 영역"), NodeArea);
	if (GraphSize == nullptr || GraphCanvas == nullptr || Backdrop == nullptr
		|| Background == nullptr || TitleArea == nullptr || TitleText == nullptr
		|| EnterButton == nullptr || CloseButton == nullptr || Legend == nullptr
		|| NodeArea == nullptr)
	{
		return false;
	}

	TestEqual(TEXT("디자인 폭"), GraphSize->GetWidthOverride(), 1672.f);
	TestEqual(TEXT("디자인 높이"), GraphSize->GetHeightOverride(), 941.f);
	TestEqual(TEXT("불투명 배경은 뒤 게임 입력을 차단"), Backdrop->GetVisibility(),
		ESlateVisibility::Visible);
	TestEqual(TEXT("불투명 배경 알파"), Backdrop->GetBrushColor().A, 1.f);
	TestTrue(TEXT("불투명 배경색 #05080C"), Backdrop->GetBrushColor().Equals(
		FLinearColor::FromSRGBColor(FColor(0x05, 0x08, 0x0C, 0xFF))));
	TestTrue(TEXT("그래프 Canvas는 디자인 SizeBox 자식"), GraphCanvas->GetParent() == GraphSize);
	TestTrue(TEXT("배경은 동적 노드와 같은 가로 Canvas에 존재"),
		Background->GetParent() == GraphCanvas);
	TestTrue(TEXT("지도 제목은 중앙정렬 Overlay의 자식"), TitleText->GetParent() == TitleArea);
	const UOverlaySlot* TitleSlot = Cast<UOverlaySlot>(TitleText->Slot);
	TestNotNull(TEXT("지도 제목 Overlay 슬롯"), TitleSlot);
	if (TitleSlot != nullptr)
	{
		TestEqual(TEXT("지도 제목 가로 중앙정렬"),
			TitleSlot->GetHorizontalAlignment(), HAlign_Center);
		TestEqual(TEXT("지도 제목 세로 중앙정렬"),
			TitleSlot->GetVerticalAlignment(), VAlign_Center);
	}
	TestEqual(TEXT("지도 제목 폰트 베이스라인 시각 보정"),
		TitleText->GetRenderTransform().Translation, FVector2D(0.f, -34.f));
	TestTrue(TEXT("닫기 이벤트 연결"), CloseButton->OnClicked.IsBound());
	TestTrue(TEXT("입장 이벤트 연결"), EnterButton->OnClicked.IsBound());
	TestNull(TEXT("새 지도에는 세로 ScrollBox가 없음"), Tree->FindWidget(TEXT("MapScrollBox")));

	UTexture2D* Texture = Cast<UTexture2D>(Background->GetBrush().GetResourceObject());
	if (!TestNotNull(TEXT("가로 지도 배경 텍스처"), Texture))
	{
		return false;
	}
	TestEqual(TEXT("가로 지도 배경 경로"), Texture->GetPathName(),
		FString(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
			"T_WorldMapLandscape_Base_20260811.T_WorldMapLandscape_Base_20260811")));
	const FIntPoint ImportedSize = Texture->GetImportedSize();
	TestEqual(TEXT("배경 원본 폭"), ImportedSize.X, 1672);
	TestEqual(TEXT("배경 원본 높이"), ImportedSize.Y, 941);
	TestEqual(TEXT("UI 텍스처 그룹"), Texture->LODGroup,
		TEnumAsByte<TextureGroup>(TEXTUREGROUP_UI));
	TestEqual(TEXT("밉 없음"), Texture->MipGenSettings,
		TEnumAsByte<TextureMipGenSettings>(TMGS_NoMipmaps));
	TestTrue(TEXT("지도 본문은 스트리밍하지 않음"), Texture->NeverStream);
	// 왜곡 후 프레임 경계가 찢어지지 않도록 통짜 Image 로만 그린다.
	TestEqual(TEXT("본문은 통짜 Image"), Background->GetBrush().DrawAs,
		ESlateBrushDrawType::Image);
	TestTrue(TEXT("9-slice 마진 없음"),
		Background->GetBrush().Margin.Left == 0.f
		&& Background->GetBrush().Margin.Top == 0.f);

	// 열 번호가 듬성듬성한 실제 Stage에서도 사용 레인 수로 계산해 노드가 48px로 축소되지 않아야 한다.
	TArray<FMapRoomView> SparseRooms;
	const int32 SparseColumns[] = { 6, 0, 12, 3, 9 };
	for (int32 Row = 0; Row < UE_ARRAY_COUNT(SparseColumns); ++Row)
	{
		FMapRoomView& Room = SparseRooms.AddDefaulted_GetRef();
		Room.mRow = Row;
		Room.mColumn = SparseColumns[Row];
		Room.mType = Row + 1 == UE_ARRAY_COUNT(SparseColumns)
			? ERoomType::BossMonster : ERoomType::Monster;
		Room.mState = Row == 0 ? EMapRoomState::Cleared : EMapRoomState::Locked;
		Room.mIsStartPoint = Row == 0;
		if (Row + 1 < UE_ARRAY_COUNT(SparseColumns))
		{
			Room.mNextRoomColumns.Add(SparseColumns[Row + 1]);
		}
	}
	Map->SetPreviewRoomsForDebug(SparseRooms, true);
	Map->SetRoomSelectionEnabled(false);
	TestTrue(TEXT("희소 열 테스트 지도 갱신"), Map->RefreshMap());
	TestEqual(TEXT("조회 모드에서는 입장 판 숨김"), EnterButton->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("조회 모드 입장 비활성"), EnterButton->GetIsEnabled());

	int32 VisibleNodeCount = 0;
	for (int32 Index = 0; Index < GraphCanvas->GetChildrenCount(); ++Index)
	{
		UFrontendMapNodeWidget* Node = Cast<UFrontendMapNodeWidget>(GraphCanvas->GetChildAt(Index));
		if (Node == nullptr || Node->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}
		++VisibleNodeCount;
		const UCanvasPanelSlot* NodeSlot = Cast<UCanvasPanelSlot>(Node->Slot);
		TestNotNull(TEXT("동적 노드 Canvas 슬롯"), NodeSlot);
		if (NodeSlot != nullptr)
		{
			TestTrue(TEXT("희소 열에서도 노드 가독 크기 유지"), NodeSlot->GetSize().X >= 80.f);
		}
	}
	TestEqual(TEXT("희소 열 방 수와 표시 노드 수"), VisibleNodeCount, SparseRooms.Num());
	return true;
}

bool FFrontendMapConnectionLineModeTest::RunTest(const FString& Parameters)
{
	UClass* MapClass = LoadClass<UFrontendMapLandscapeWidget>(nullptr,
		TEXT("/Game/UI/WorldMapLandscape/WBP_FrontendMapLandscape."
			"WBP_FrontendMapLandscape_C"));
	if (!TestNotNull(TEXT("가로형 월드맵 WBP 클래스"), MapClass))
	{
		return false;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	UFrontendMapLandscapeWidget* Map = CreateWidget<UFrontendMapLandscapeWidget>(World, MapClass);
	if (!TestNotNull(TEXT("가로형 월드맵 인스턴스"), Map))
	{
		return false;
	}
	Map->TakeWidget();

	UCanvasPanel* GraphCanvas = Map->WidgetTree != nullptr
		? Cast<UCanvasPanel>(Map->WidgetTree->FindWidget(TEXT("MapGraphCanvas"))) : nullptr;
	if (!TestNotNull(TEXT("가로 그래프 Canvas"), GraphCanvas))
	{
		return false;
	}

	// 0 -> 1은 실제 지나온 길, 1 -> 2는 현재 방에서 갈 수 있는 다음 후보 길이다.
	TArray<FMapRoomView> Rooms;
	for (int32 Row = 0; Row < 3; ++Row)
	{
		FMapRoomView& Room = Rooms.AddDefaulted_GetRef();
		Room.mRow = Row;
		Room.mColumn = 0;
		Room.mType = ERoomType::Monster;
		Room.mState = Row < 2 ? EMapRoomState::Cleared : EMapRoomState::Ready;
		Room.mVisited = Row < 2;
		Room.mSelectable = Row == 2;
		if (Row < 2)
		{
			Room.mNextRoomColumns.Add(0);
		}
	}
	Map->SetPreviewRoomsForDebug(Rooms, false);

	auto CollectVisibleLines = [GraphCanvas]()
	{
		TArray<UFrontendMapLineWidget*> Lines;
		for (int32 Index = 0; Index < GraphCanvas->GetChildrenCount(); ++Index)
		{
			UFrontendMapLineWidget* Line =
				Cast<UFrontendMapLineWidget>(GraphCanvas->GetChildAt(Index));
			if (Line != nullptr && Line->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Lines.Add(Line);
			}
		}
		return Lines;
	};

	Map->SetRoomSelectionEnabled(false);
	TestTrue(TEXT("전투 중 조회 지도 갱신"), Map->RefreshMap());
	TArray<UFrontendMapLineWidget*> Lines = CollectVisibleLines();
	if (!TestEqual(TEXT("조회 지도 연결선 수"), Lines.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("조회 지도에서도 지나온 길은 밝음"), Lines[0]->IsOpenPath());
	TestTrue(TEXT("첫 연결은 지나온 길"), Lines[0]->IsTraversedPath());
	TestFalse(TEXT("전투 중에는 다음 후보 길을 밝히지 않음"), Lines[1]->IsOpenPath());
	TestFalse(TEXT("다음 후보 길은 지나온 길이 아님"), Lines[1]->IsTraversedPath());

	Map->SetRoomSelectionEnabled(true);
	TestTrue(TEXT("클리어 후 선택 지도 갱신"), Map->RefreshMap());
	Lines = CollectVisibleLines();
	if (!TestEqual(TEXT("선택 지도 연결선 수"), Lines.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("클리어 후에는 다음 후보 길을 밝힘"), Lines[1]->IsOpenPath());
	TestFalse(TEXT("밝힌 다음 후보 길은 아직 지나온 길이 아님"), Lines[1]->IsTraversedPath());
	return true;
}

#endif
