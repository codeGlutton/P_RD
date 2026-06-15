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
		// Win64 에디터 반복 작업에서 C++ Hot Reload를 켜기 위한 설정.
		// 기본 빌드 환경에는 WITH_HOT_RELOAD가 꺼져 있어, 환경을 오버라이드해 정의를 강제로 추가한다.
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			bOverrideBuildEnvironment = true;
			GlobalDefinitions.Add("WITH_HOT_RELOAD=1");
		}
		ExtraModuleNames.Add("P_RD");
	}
}
