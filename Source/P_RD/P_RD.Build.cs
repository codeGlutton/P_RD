using UnrealBuildTool;

/**
 * @brief P_RD 런타임 모듈의 빌드 의존성을 선언합니다.
 *
 * @details
 * 이번 UI foundation PR은 WBP를 단순히 참조하는 수준을 넘어, C++에서 버튼 입력, Slate 타입,
 * 런타임 PNG 디코딩, 인트로 미디어 재생을 직접 다룹니다. 그래서 관련 모듈 의존성을 Build.cs에
 * 명시해 "어떤 UI 기능 때문에 어떤 모듈이 필요한지"를 리뷰 시 바로 추적할 수 있게 합니다.
 *
 * 에디터 전용 WBP 생성/편집 의존성은 Target.bBuildEditor 안에만 둡니다. 런타임 APK/패키지에
 * UnrealEd 계열 모듈이 끌려가지 않게 분리하는 것이 이 파일에서 가장 중요한 경계입니다.
 */
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

        /*
         * 캐릭터 선택 WBP를 코드로 생성·구성하는 에디터 커맨드렛(ClassSelectWBPSetupCommandlet) 전용 의존성.
         * 에디터 빌드에서만 필요하므로 런타임 패키지에 끌려들어가지 않게 bBuildEditor로 감싼다.
         */
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
