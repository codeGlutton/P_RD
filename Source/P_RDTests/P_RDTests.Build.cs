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

        PublicDependencyModuleNames.AddRange(new string[]
        {
            /* Engine Core Modules */
            "Core",
            "CoreUObject",
            "Engine",

            /* 테스트 대상 게임 모듈 */
            "P_RD",

            /* GAS Plugin Modules */
            "GameplayAbilities",
        });

        // 모듈 내 하위 폴더에서 루트 헤더를 참조할 수 있도록 경로 추가
        PrivateIncludePaths.AddRange(new string[]
        {
            "P_RDTests",
            "P_RD",  // P_RD 모듈 헤더 참조를 위한 경로
        });
    }
}
