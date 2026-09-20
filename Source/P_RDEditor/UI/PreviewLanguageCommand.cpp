// 에디터에서 게임 텍스트의 미리보기 언어를 바꾸는 콘솔 커맨드.
//
// 에디터 자동화(-ExecCmds="Automation RunTests ...")는 -culture= 를 무시하고
// 에디터의 게임 로컬라이제이션 미리보기 언어를 따른다. 언어별 WBP 캡처를
// 헤드리스로 뽑으려면 테스트 실행 전에 이 커맨드로 언어를 바꿔야 한다.
//
//   UnrealEditor-Cmd P_RD.uproject -ExecCmds="RD.Editor.SetPreviewLanguage en,
//       Automation RunTests P_RD.UI.CombatLayout.Capture; Quit" ...

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"

static FAutoConsoleCommand GSetPreviewLanguageCommand(
	TEXT("RD.Editor.SetPreviewLanguage"),
	TEXT("게임 텍스트 미리보기 언어를 바꾼다. 사용: RD.Editor.SetPreviewLanguage <en|ko>"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("RD.Editor.SetPreviewLanguage: 문화 코드가 필요하다 (예: en, ko)"));
			return;
		}
		const FString& Culture = Args[0];
		FInternationalization::Get().SetCurrentCulture(Culture);
		FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
		// 에디터는 부팅 언어의 locres 만 라이브 테이블에 올려 둔다. 다른
		// 언어로 바꿀 때는 리소스를 강제로 다시 읽어야 한다(런타임 경로와 동일).
		FTextLocalizationManager::Get().RefreshResources();
		FTextLocalizationManager::Get().WaitForAsyncTasks();
		// 진단: 알려진 키가 이 시점에 어떤 언어로 리졸브되는지 찍는다.
		FText Probe;
		FText::FindTextInLiveTable_Advanced(
			TEXT("CombatLayoutHUD"), TEXT("StatusWeakness"), Probe);
		UE_LOG(LogTemp, Display,
			TEXT("RD.Editor.SetPreviewLanguage: '%s' 적용됨. Probe(StatusWeakness)='%s' Culture=%s"),
			*Culture, *Probe.ToString(),
			*FInternationalization::Get().GetCurrentCulture()->GetName());
	}));
