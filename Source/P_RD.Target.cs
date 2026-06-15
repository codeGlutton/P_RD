// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/**
 * @brief P_RD 게임 타깃의 빌드 규칙입니다.
 *
 * @details
 * 이번 UI 작업은 에디터에서 WBP와 C++ 위젯을 반복 수정하는 흐름이 많습니다. Win64 개발 환경에서
 * Hot Reload가 꺼진 상태면 작은 UI helper 수정도 에디터 재시작 비용으로 이어져, Win64 타깃에만
 * WITH_HOT_RELOAD를 명시적으로 켭니다.
 *
 * Android/패키징 타깃까지 이 override를 넓히지 않는 것이 의도입니다. 모바일 빌드 검증은 기본 UBT
 * 환경을 유지하고, 에디터 반복 작업 편의만 좁게 보완합니다.
 */
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
