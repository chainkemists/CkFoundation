using System.IO;
using UnrealBuildTool;

public class CkLoadingScreen : CkModuleRules
{
    public CkLoadingScreen(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "InputCore",
            "MoviePlayer",
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
