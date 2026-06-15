using UnrealBuildTool;

public class P_RD : ModuleRules
{
	public P_RD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[] {
            /* Engine Core Modules */
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "MediaAssets",              // 인트로 시네마틱 MP4 재생(MediaPlayer/MediaSource)에 필요
            "ImageWrapper",             // 런타임 PNG 디코딩(UITextureLoader)에 필요

            /*
             * WBP 자체는 UMG 모듈만으로 다루지만, 이번 UI 패널은 런타임에서 버튼 입력 방식,
             * Slate Visibility, FReply, Widget Transform 같은 Slate 타입을 직접 사용한다.
             * 그래서 Carousel/Dice/TopMenuBar 쪽 C++ 위젯 컴파일을 위해 Slate/SlateCore 의존성을 명시한다.
             */
            "Slate",
            "SlateCore",

            /* Gameplay Tag Modules */
            "GameplayTags",				// 게임플레이 태그 시스템

            /* GAS Plugin Modules */
			"GameplayTasks",			// GAS에서 비동기적인 작업을 생성하고 관리하는 모듈
			"GameplayAbilities",		// GAS 프레임워크

            /* AI Plugin Modules */
            "AIModule",                 // 기본 AI 연관 도구 사용
            "StateTreeModule",          // StateTree 사용
            "GameplayStateTreeModule",  // StateTree AI Comp 사용
        });

        // 캐릭터 선택 WBP를 코드로 생성·구성하는 에디터 커맨드렛(ClassSelectWBPSetupCommandlet) 전용 의존성.
        // 에디터 빌드에서만 필요하므로 런타임 패키지에 끌려들어가지 않게 bBuildEditor로 감싼다.
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",          // 커맨드렛/에셋 편집 기반
                "UMGEditor",         // WBP(WidgetBlueprint) 에디터 조작
                "AssetTools",        // 텍스처 임포트·에셋 생성
                "KismetCompiler",    // 블루프린트 컴파일
                "BlueprintGraph",    // 블루프린트 그래프 노드 구성
            });
        }

        PrivateIncludePaths.AddRange(new string[] {
            "P_RD",
        });

        // 온라인 기능을 사용할 때만 OnlineSubsystem을 추가한다.
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // OnlineSubsystemSteam을 쓸 경우 uproject의 plugins 항목에서도 Enabled=true로 켜야 한다.
    }
}
