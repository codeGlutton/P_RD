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

            /* Editor Core Modules */
            "UnrealEd",
            "EditorFramework", 
            "AssetRegistry",

            /* Asset Validator Modules */
            "DataValidation",
        });

        PrivateIncludePaths.AddRange(new string[] {
            "P_RD",
            "P_RDEditor",
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
