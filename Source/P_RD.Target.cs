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
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			bOverrideBuildEnvironment = true;
			GlobalDefinitions.Add("WITH_HOT_RELOAD=1");
		}
		ExtraModuleNames.Add("P_RD");
	}
}
