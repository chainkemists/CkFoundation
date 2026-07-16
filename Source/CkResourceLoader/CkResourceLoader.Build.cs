using System.IO;
using UnrealBuildTool;

public class CkResourceLoader : CkModuleRules
{
    public CkResourceLoader(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
        });
    }
}
