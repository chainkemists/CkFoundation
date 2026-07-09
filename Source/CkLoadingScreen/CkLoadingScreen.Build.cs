using System.IO;
using UnrealBuildTool;

public class CkLoadingScreen : CkModuleRules
{
    public CkLoadingScreen(ReadOnlyTargetRules Target) : base(Target)
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
            "PreLoadScreen",
            "RenderCore",
            "DeveloperSettings",
            "UMG",

            "CkCore",
            "CkLog",
            "CkSettings",
        });
    }
}
