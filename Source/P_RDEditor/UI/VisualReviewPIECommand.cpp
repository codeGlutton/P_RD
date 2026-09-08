#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"

static FAutoConsoleCommand StartVisualReviewPIE(TEXT("RD.Editor.StartVisualReviewPIE"),
	TEXT("Start a 1280x720 PIE window for visual review."), FConsoleCommandDelegate::CreateLambda([]()
	{
		if (!GEditor || GEditor->PlayWorld) return;
		ULevelEditorPlaySettings* Settings = DuplicateObject<ULevelEditorPlaySettings>(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
		Settings->NewWindowWidth = 1280;
		Settings->NewWindowHeight = 720;
		FRequestPlaySessionParams Params;
		Params.EditorPlaySettings = Settings;
		GEditor->RequestPlaySession(Params);
	}));
