using UnrealBuildTool;

/**
 * @brief P_RD 모듈이 사용하는 엔진 모듈 목록입니다.
 *
 * @details
 * UI 코드에서 영상 재생, PNG 읽기, Slate 타입을 쓰기 때문에 관련 모듈을 여기에 추가합니다.
 * 새 모듈을 넣을 때는 옆에 이유를 짧게 적어 두면 나중에 지워도 되는 의존성인지 판단하기 쉽습니다.
 *
 * UnrealEd 같은 에디터 전용 모듈은 bBuildEditor 안에만 둡니다.
 * 그래야 APK 같은 실제 실행 빌드에 에디터 모듈이 섞이지 않습니다.
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
            "ImageWrapper",             // 실행 중 PNG 파일을 읽는 UITextureLoader에 필요

            /*
             * UI 위젯을 C++에서 다룰 때 SlateVisibility, FReply 같은 Slate 타입을 쓴다.
             * 그래서 UMG와 함께 Slate/SlateCore도 필요하다.
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
         * WBP나 텍스처를 코드로 만들고 고치는 에디터 전용 도구가 쓰는 모듈.
         * 게임 실행에는 필요 없으니 에디터 빌드에서만 추가한다.
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
