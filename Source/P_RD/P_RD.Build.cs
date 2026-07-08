using UnrealBuildTool;

using System.IO;

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

            /* Media Modules */
            "MediaAssets",              // 인트로 시네마틱 MP4 재생(MediaPlayer/MediaSource)에 필요
            "ImageWrapper",             // 실행 중 PNG 파일을 읽는 UITextureLoader에 필요

            /* Native SWidget Modules */
            "UMG",
            "Slate",
            "SlateCore",

            /* VFX */
            "Niagara",

            /* Gameplay Tag Modules */
            "GameplayTags",				// 게임플레이 태그 시스템

            /* GAS Plugin Modules */
			"GameplayTasks",			// GAS에서 비동기적인 작업을 생성하고 관리하는 모듈
			"GameplayAbilities",		// GAS 프레임워크

            /* AI Plugin Modules */
            "AIModule",                 // 기본 AI 연관 도구 사용

            /* Procedural Mesh */
            "ProceduralMeshComponent",  // 전투 3D 주사위 프리뷰 액터(CombatDicePreviewActor) 면 텍스처 쿼드에 필요
        });

        PrivateIncludePaths.AddRange(new string[] {
            "P_RD",
        });

        // MediaPlayer가 OS 파일 경로로 직접 여는 mp4는 pak/ucas 안이 아니라 loose 파일로 스테이징한다.
        // Content/SVN 전체를 패키징하면 SVN 원본/시안/.svn 메타데이터까지 들어가 APK가 크게 불어난다.
        StageContentMedia(Target, "mTitleBackgroundVideoPath", "SVN/OutSideAsset/AICreation/campfire_titleloop_idle_x3preview.mp4");
        StageContentMedia(Target, "mIntroCinematicVideoPath", "SVN/OutSideAsset/AICreation/hero_loading_intro4_1280_3s.mp4");

        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
            });
        }

        // 온라인 기능을 사용할 때만 OnlineSubsystem을 추가한다.
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // OnlineSubsystemSteam을 쓸 경우 uproject의 plugins 항목에서도 Enabled=true로 켜야 한다.
    }

    private void StageContentMedia(ReadOnlyTargetRules Target, string ConfigKey, string FallbackPath)
    {
        string MediaPath = ReadDefaultGameConfigValue(Target, ConfigKey, FallbackPath);
        if (string.IsNullOrWhiteSpace(MediaPath))
        {
            return;
        }

        string DependencyPath = Path.IsPathRooted(MediaPath)
            ? MediaPath
            : "$(ProjectDir)/Content/" + MediaPath.Replace("\\", "/");
        RuntimeDependencies.Add(DependencyPath, StagedFileType.NonUFS);
    }

    private static string ReadDefaultGameConfigValue(ReadOnlyTargetRules Target, string ConfigKey, string FallbackValue)
    {
        if (Target.ProjectFile == null)
        {
            return FallbackValue;
        }

        string IniPath = Path.Combine(Target.ProjectFile.Directory.FullName, "Config", "DefaultGame.ini");
        if (!File.Exists(IniPath))
        {
            return FallbackValue;
        }

        foreach (string RawLine in File.ReadLines(IniPath))
        {
            string Line = RawLine.Trim();
            if (Line.StartsWith(ConfigKey + "="))
            {
                return Line.Substring(ConfigKey.Length + 1).Trim().Trim('"');
            }
        }

        return FallbackValue;
    }
}
