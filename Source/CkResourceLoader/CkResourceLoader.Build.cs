using System.IO;
using UnrealBuildTool;

public class CkResourceLoader : CkModuleRules
{
    public CkResourceLoader(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "../CkEcs_PCH.h";
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
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
