using UnrealBuildTool;

/**
 * @brief P_RD 런타임 모듈의 빌드 의존성을 선언합니다.
 *
 * @details
 * 이 파일은 모듈 전체가 쓰는 의존성을 한곳에 모은 것이라, 뒤따르는 UI 작업이 쓰는 모듈(미디어 재생,
 * Slate 타입, PNG 디코딩 등)도 여기서 미리 선언됩니다. 각 의존성에 "무엇 때문에 필요한지"를 적어,
 * 리뷰 시 모듈이 왜 그 의존성을 끌어오는지 바로 알 수 있게 했습니다.
 *
 * 이 파일에서 가장 중요한 경계는 마지막의 bBuildEditor 블록입니다. 에디터 전용 모듈(UnrealEd 계열)을
 * 그 안에만 두어, 런타임 패키지/APK에 에디터 모듈이 끌려 들어가지 않게 분리합니다.
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
             * WBP 표시 자체는 UMG만으로 되지만, UI 위젯을 C++에서 다루면 SlateVisibility, FReply,
             * Widget Transform 같은 Slate 타입을 직접 쓰게 된다. 그 위젯들의 컴파일을 위해 Slate/SlateCore를 명시한다.
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
         * WBP/텍스처를 코드로 생성·편집하는 에디터 전용 도구(커맨드렛 등)가 쓰는 의존성.
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
