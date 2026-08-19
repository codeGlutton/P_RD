/*****************************************************************//**
 * @file   P_RDTests.Build.cs
 * @brief  P_RD 프로젝트 자동화 테스트 모듈 빌드 설정
 * @details
 * 에디터 전용 모듈로, 게임 배포 빌드에는 포함되지 않는다.
 * @author 이문환
 * @date   2026-04-30
 *********************************************************************/

using UnrealBuildTool;

/// <summary>P_RD 자동화 테스트 모듈 빌드 규칙</summary>
public class P_RDTests : ModuleRules
{
    public P_RDTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // 여러 테스트 파일이 같은 이름의 익명-namespace 도우미를 갖는다.
        // Unity translation unit으로 합치면 이들이 한 파일에서 재정의되므로 테스트 모듈은 개별 컴파일한다.
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            /* Engine Core Modules */
            "Core",
            "CoreUObject",
            "Engine",

            /* 테스트 대상 게임 모듈 */
            "P_RD",

            /* Gameplay Tag Modules */
            "GameplayTags",

            /* AI Module (BoardCombatTarget가 참조하는 GenericTeamAgentInterface 등) */
            "AIModule",

            /* UI 배치안을 오프스크린 렌더해서 PNG로 남기는 캡처 테스트용 */
            "UMG",
            "Slate",
            "SlateCore",
            "RenderCore",
            "RHI",
            "ImageCore",
        });

        // 캡처 테스트는 에디터 월드에서만 돈다.
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.Add("P_RDEditor");
        }

        // 모듈 내 하위 폴더에서 루트 헤더를 참조할 수 있도록 경로 추가
        PrivateIncludePaths.AddRange(new string[]
        {
            "P_RDTests",
            "P_RD",  // P_RD 모듈 헤더 참조를 위한 경로
            "P_RDEditor",
        });
    }
}
