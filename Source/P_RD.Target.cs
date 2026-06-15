// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class P_RDTarget : TargetRules
{
	public P_RDTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		/*
		 * 게임 실행용 Win64 빌드에서도 Hot Reload 코드를 같은 조건으로 보게 한다.
		 * UI 위젯 코드를 에디터와 게임 빌드에서 다르게 컴파일하지 않기 위한 설정이다.
		 */
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			bOverrideBuildEnvironment = true;
			GlobalDefinitions.Add("WITH_HOT_RELOAD=1");
		}
		ExtraModuleNames.Add("P_RD");
	}
}
