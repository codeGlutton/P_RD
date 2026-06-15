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

            /*
             * 인트로 UI에서 영상을 직접 재생하고 상태를 확인한다.
             * 그래서 MediaPlayer 관련 타입을 쓰기 위해 필요하다.
             */
            "MediaAssets",

            /*
             * 런타임에 이미지 파일을 읽어 Texture2D로 바꿀 때 사용한다.
             * 버튼 이미지, 배경, 프리뷰 이미지를 코드에서 붙이기 위한 모듈이다.
             */
            "ImageWrapper",

            /*
             * C++ UI 위젯에서 버튼 입력, 표시 상태, 위젯 위치 값을 직접 다룬다.
             * Carousel, DicePanel, TopMenuBar 쪽 코드가 Slate 타입을 쓰기 때문에 필요하다.
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
         * WBP를 자동으로 맞추는 commandlet에서 에디터 API를 사용한다.
         * 게임 실행 빌드에는 필요 없으므로 에디터 빌드에서만 추가한다.
         */
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
                "UMGEditor",
                "AssetTools",
                "KismetCompiler",
                "BlueprintGraph",
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
