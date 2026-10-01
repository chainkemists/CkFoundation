using System.IO;
using UnrealBuildTool;

public class CkPso : CkModuleRules
{
    public CkPso(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "DeveloperSettings",

            "CkCore",
            "CkLog",
            "CkSettings",
            "CkLoadingScreen",
        });
    }
}
