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
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFrontendMapLandscapeStructureTest,
	"P_RD.UI.FrontendMap.LandscapeStructure",
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

#endif
