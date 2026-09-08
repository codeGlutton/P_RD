// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Effekseer : ModuleRules
	{
		public Effekseer(ReadOnlyTargetRules Target)
			: base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
#if !UE_5_0_OR_LATER
            CppStandard = CppStandardVersion.Cpp17;
#endif

            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Effekseer/Private",
					"Effekseer/Private/Effekseer",
					"Effekseer/Private/EffekseerRendererCommon"
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"RHI",
					"RenderCore",
					"Projects",
					//"ShaderCore",
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"RHI",
					"RenderCore",
					//"ShaderCore",
					"Slate",
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

			// Live editing/profiling over the network is not used by this project.
			// Leave __EFFEKSEER_NETWORK_ENABLED__ undefined on every platform:
			// the vendored runtime does not include its optional FlatBuffers dependency.
			// In particular, Android must use the same offline runtime as Win64.

			PrivateDefinitions.Add("__EFFEKSEER_FOR_UE4__");
		}
	}
}
