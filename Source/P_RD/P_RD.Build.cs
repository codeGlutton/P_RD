using UnrealBuildTool;

/**
 * @brief P_RD 모듈이 사용하는 엔진 모듈 목록입니다.
 *
 * @details
 * UI 코드에서 영상 재생, PNG 읽기, Slate 타입을 쓰기 때문에 관련 모듈을 여기에 추가합니다.
 * 새 모듈을 넣을 때는 옆에 이유를 짧게 적어 두면 나중에 지워도 되는 의존성인지 판단하기 쉽습니다.
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

        PrivateIncludePaths.AddRange(new string[] {
            "P_RD",
        });

        // 온라인 기능을 사용할 때만 OnlineSubsystem을 추가한다.
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // OnlineSubsystemSteam을 쓸 경우 uproject의 plugins 항목에서도 Enabled=true로 켜야 한다.
    }
}
