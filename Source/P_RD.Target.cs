// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/**
 * @brief P_RD 게임 타깃의 빌드 규칙입니다.
 */
public class P_RDTarget : TargetRules
{
	public P_RDTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// Win64 개발 빌드에만 WITH_HOT_RELOAD 정의를 강제로 켠다.
		// 이 정의는 기본 빌드 환경에 없어서 bOverrideBuildEnvironment로 환경을 열어야 추가할 수 있다.
		// (윈도우 에디터에서 C++ 재로드를 쓰기 위한 설정이며, 모바일/패키징 타깃에는 적용하지 않는다.)
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			bOverrideBuildEnvironment = true;
			GlobalDefinitions.Add("WITH_HOT_RELOAD=1");
		}
		ExtraModuleNames.Add("P_RD");
	}
}
