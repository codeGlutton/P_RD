#include "Misc/AutomationTest.h"
#include "Actor/TileMap/TileMap.h"
#include "Editor.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTileHighlightPulseMembershipTest, "P_RD.TileMap.HighlightPulseMembership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTileHighlightPulseMembershipTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("Editor world"), World)) return false;
	ATileMap* Map = World->SpawnActor<ATileMap>();
	if (!TestNotNull(TEXT("Tile map"), Map)) return false;
	Map->SetTileHighlight({FTileIndex(0, 0), FTileIndex(1, 0)}, ETileHighlightFlag::Effect);
	TestEqual(TEXT("Only animated tiles are scheduled"), Map->mPulsingHighlights.Num(), 2);
	Map->Tick(1.0f / 60.0f);
	TestEqual(TEXT("Tick retains membership"), Map->mPulsingHighlights.Num(), 2);
	Map->SetTileHighlight({FTileIndex(0, 0)}, ETileHighlightFlag::Select);
	TestEqual(TEXT("Selection invalidates the effect layer"), Map->mPulsingHighlights.Num(), 0);
	Map->SetTileHighlight({FTileIndex(1, 0)}, ETileHighlightFlag::Effect);
	TestEqual(TEXT("New effect becomes animated"), Map->mPulsingHighlights.Num(), 1);
	Map->ClearTileHighlight(ETileHighlightFlag::Effect);
	TestEqual(TEXT("Cleared effects stop updating"), Map->mPulsingHighlights.Num(), 0);
	Map->Destroy();
	return !HasAnyErrors();
}
#endif
