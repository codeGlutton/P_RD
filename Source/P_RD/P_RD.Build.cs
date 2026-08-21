using UnrealBuildTool;

using System.IO;

/**
 * @brief P_RD 모듈이 사용하는 엔진 모듈 목록입니다.
 *
 * @details
 * UI 코드에서 영상 재생, Slate 타입을 쓰기 때문에 관련 모듈을 여기에 추가합니다.
 * 새 모듈을 넣을 때는 옆에 이유를 짧게 적어 두면 나중에 지워도 되는 의존성인지 판단하기 쉽습니다.
 */
public class P_RD : ModuleRules
{
	public P_RD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 공개 헤더(GamePlayType.h)가 NiagaraSystem.h를 노출하므로 소비 모듈에 전파한다.
        PublicDependencyModuleNames.Add("Niagara");

        PrivateDependencyModuleNames.AddRange(new string[] {
            /* Engine Core Modules */
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",

            /* Media Modules */
            "MediaAssets",
            "LevelSequence",
            "MovieScene",
            "MovieSceneTracks",

            /* Native SWidget Modules */
            "UMG",
            "Slate",
            "SlateCore",

            /* VFX */
            "NiagaraAnimNotifies",

            /* Gameplay Tag Modules */
            "GameplayTags",

            /* AI Plugin Modules */
            "AIModule",

            /* Camera Shaeks Modules*/
            "EngineCameras"
        });

        PrivateIncludePaths.AddRange(new string[] {
            "P_RD",
        });

        // MediaPlayer가 OS 파일 경로로 직접 여는 mp4는 pak/ucas 안이 아니라 loose 파일로 스테이징한다.
        // Content/SVN 전체를 패키징하면 SVN 원본/시안/.svn 메타데이터까지 들어가 APK가 크게 불어난다.
        StageContentMedia(Target, "mTitleBackgroundVideoPath", "SVN/OutSideAsset/AICreation/UI/Title/Video/Random30_16x9/Title_All6_16x9_combo01_5s_mobile.mp4");
        StageContentMediaArray(Target, "mTitleBackgroundVideoPaths");
        StageContentMedia(Target, "mIntroCinematicVideoPath", "SVN/OutSideAsset/AICreation/UI/Title/Video/Intro/H3_Intro_Crystal_v01_15s.mp4");
        StageContentMedia(Target, "mIntroCinematicAcceleratedVideoPath", "SVN/OutSideAsset/AICreation/UI/Title/Video/Intro/H3_Intro_Crystal_v01_15s_3x.mp4");
        StageContentMedia(Target, "mCombatVictoryVideoPath", "SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Victory_01.mp4");
        StageContentMedia(Target, "mCombatDefeatVideoPath", "SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Defeat_01.mp4");

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

    private void StageContentMediaArray(ReadOnlyTargetRules Target, string ConfigKey)
    {
        foreach (string MediaPath in ReadDefaultGameConfigValues(Target, ConfigKey))
        {
            string DependencyPath = Path.IsPathRooted(MediaPath)
                ? MediaPath
                : "$(ProjectDir)/Content/" + MediaPath.Replace("\\", "/");
            RuntimeDependencies.Add(DependencyPath, StagedFileType.NonUFS);
        }
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

    private static string[] ReadDefaultGameConfigValues(ReadOnlyTargetRules Target, string ConfigKey)
    {
        if (Target.ProjectFile == null)
        {
            return new string[0];
        }

        string IniPath = Path.Combine(Target.ProjectFile.Directory.FullName, "Config", "DefaultGame.ini");
        if (!File.Exists(IniPath))
        {
            return new string[0];
        }

        var Values = new System.Collections.Generic.List<string>();
        string PlainPrefix = ConfigKey + "=";
        string AddPrefix = "+" + ConfigKey + "=";
        foreach (string RawLine in File.ReadLines(IniPath))
        {
            string Line = RawLine.Trim();
            if (Line.StartsWith(PlainPrefix) || Line.StartsWith(AddPrefix))
            {
                int EqualsIndex = Line.IndexOf('=');
                string Value = Line.Substring(EqualsIndex + 1).Trim().Trim('"');
                if (!string.IsNullOrWhiteSpace(Value))
                {
                    Values.Add(Value);
                }
            }
        }
        return Values.ToArray();
    }
}
