using System.IO;
using UnrealBuildTool;

public class CkInput : CkModuleRules
{
    public CkInput(ReadOnlyTargetRules Target) : base(Target, UseUnityBuild: false)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "InputCore",
            "EnhancedInput",
            "GameplayTags",
            "AssetRegistry",
            "DeveloperSettings",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings"
        });
    }
}
