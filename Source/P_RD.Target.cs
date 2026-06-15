// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/**
 * @brief 게임 실행 빌드 설정입니다.
 */
public class P_RDTarget : TargetRules
{
	public P_RDTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// 윈도우 에디터에서 C++ 재로드를 쓸 수 있게 Win64에만 켠다.
		// 모바일 빌드에는 필요 없으므로 적용하지 않는다.
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			bOverrideBuildEnvironment = true;
			GlobalDefinitions.Add("WITH_HOT_RELOAD=1");
		}
		ExtraModuleNames.Add("P_RD");
	}
}
