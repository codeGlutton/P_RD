using UnrealBuildTool;

public class P_RDEditor : ModuleRules
{
	public P_RDEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateDependencyModuleNames.AddRange(new string[] {
            /* Game Modules */
            "P_RD",

            /* Engine Core Modules */
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",

            /* Editor Core Modules */
            "UnrealEd",
            "EditorFramework", 
            "AssetRegistry",
            "UMGEditor",
            "AssetTools",
            "KismetCompiler",
            "BlueprintGraph",
            "ImageWrapper",

            /* Asset Validator Modules */
            "DataValidation",
        });

        PrivateIncludePaths.AddRange(new string[] {
            "P_RD",
            "P_RDEditor",
        });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
