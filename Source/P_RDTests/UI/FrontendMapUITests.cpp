#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "UI/FrontendMapWidget.h"
#include "Widgets/SWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFrontendMapScrollBackgroundBTest,
	"P_RD.UI.FrontendMap.ScrollBackgroundB",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief 선택한 B안이 세로 스크롤 WBP의 실제 본문 텍스처로 연결됐는지 확인한다.
 *
 * 배경 PNG만 바꾸고 ScrollBox를 잃거나, WBP는 그대로인데 다른 텍스처를
 * 가져오면 에디터에서는 그럴듯해 보여도 런타임 지도 계약이 깨진다.
 */
bool FFrontendMapScrollBackgroundBTest::RunTest(const FString& Parameters)
{
	UClass* MapClass = LoadClass<UFrontendMapWidget>(nullptr,
		TEXT("/Game/UI/WBP_FrontendMap.WBP_FrontendMap_C"));
	if (!TestNotNull(TEXT("월드맵 WBP 클래스"), MapClass))
	{
		return false;
	}

	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("에디터 월드"), World))
	{
		return false;
	}

	UFrontendMapWidget* Map = CreateWidget<UFrontendMapWidget>(World, MapClass);
	if (!TestNotNull(TEXT("런타임 월드맵 인스턴스"), Map))
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

	UScrollBox* Scroll = Cast<UScrollBox>(
		Tree->FindWidget(TEXT("MapScrollBox")));
	USizeBox* GraphSize = Cast<USizeBox>(
		Tree->FindWidget(TEXT("MapGraphSize")));
	UCanvasPanel* GraphCanvas = Cast<UCanvasPanel>(
		Tree->FindWidget(TEXT("MapGraphCanvas")));
	UImage* Background = Cast<UImage>(
		Tree->FindWidget(TEXT("Map_ParchmentBody")));
	UButton* CloseButton = Cast<UButton>(
		Tree->FindWidget(TEXT("CloseButton")));
	if (CloseButton == nullptr)
	{
		CloseButton = Cast<UButton>(
			Tree->FindWidget(TEXT("RuntimeCloseButton")));
	}
	if (CloseButton == nullptr)
	{
		TArray<UWidget*> Widgets;
		Tree->GetAllWidgets(Widgets);
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

	TestNotNull(TEXT("세로 지도 ScrollBox"), Scroll);
	TestNotNull(TEXT("동적 지도 크기 상자"), GraphSize);
	TestNotNull(TEXT("노드/선 그래프 Canvas"), GraphCanvas);
	TestNotNull(TEXT("B안 지도 본문"), Background);
	TestNotNull(TEXT("지도 닫기 단추"), CloseButton);
	if (Scroll == nullptr || GraphSize == nullptr
		|| GraphCanvas == nullptr || Background == nullptr
		|| CloseButton == nullptr)
	{
		return false;
	}
	TestTrue(TEXT("지도 닫기 동작이 연결됨"),
		CloseButton->OnClicked.IsBound());

	TestEqual(TEXT("지도 스크롤 방향"), Scroll->GetOrientation(),
		Orient_Vertical);
	TestEqual(TEXT("기본 스크롤바는 숨김"), Scroll->GetScrollBarVisibility(),
		ESlateVisibility::Collapsed);
	TestTrue(TEXT("지도 크기 상자는 ScrollBox 자식"),
		GraphSize->GetParent() == Scroll);
	TestTrue(TEXT("그래프 Canvas는 크기 상자 자식"),
		GraphCanvas->GetParent() == GraphSize);
	TestTrue(TEXT("B안 본문은 노드와 함께 스크롤"),
		Background->GetParent() == GraphCanvas);

	UTexture2D* Texture = Cast<UTexture2D>(
		Background->GetBrush().GetResourceObject());
	if (!TestNotNull(TEXT("B안 런타임 텍스처"), Texture))
	{
		return false;
	}

	// 원근은 이미지에 굽지 않는다 — 본문은 평평한 지도, 눕힘은 리테이너
	// 머티리얼(M_MapPerspective)이 화면에서 건다.
	// 아트 원본은 SVN(Content/SVN 정션)이 정본이다. git 폴더에는 두지 않는다.
	TestEqual(TEXT("평평한 지도 텍스처 경로"), Texture->GetPathName(),
		FString(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/"
			"T_StageMap_Scroll_Flat.T_StageMap_Scroll_Flat")));
	const FIntPoint ImportedSize = Texture->GetImportedSize();
	TestEqual(TEXT("지도 폭"), ImportedSize.X, 1024);
	TestEqual(TEXT("지도 높이"), ImportedSize.Y, 3072);
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

	/*
	 * 원근 리테이너는 기본 꺼짐이다(mUseMapPerspective). 책상 배경을 버리고
	 * 팝업으로 바꾸면서 기울임의 근거가 약해졌고, 탭 좌표 역변환 복잡도도 함께
	 * 빠진다. 켜져 있다면 스크롤 레이어를 감싸고 이펙트 머티리얼이 붙어 있어야
	 * 한다는 계약만 확인한다.
	 */
	URetainerBox* Retainer = Cast<URetainerBox>(
		Tree->FindWidget(TEXT("MapPerspectiveRetainer")));
	if (Retainer != nullptr)
	{
		TestTrue(TEXT("스크롤 박스는 리테이너 자식"),
			Scroll->GetParent() == Retainer);
		// Slate 미생성 상태에서도 확인 가능한 프로퍼티 getter 를 쓴다(MID 는 페인트 시 생성).
		TestNotNull(TEXT("원근 이펙트 머티리얼"),
			Retainer->GetEffectMaterialInterface());
	}
	return true;
}

#endif
