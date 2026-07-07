using System.IO;
using UnrealBuildTool;

public class CkPool : CkModuleRules
{
    public CkPool(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "GameplayTags",

            "CkCore",
            "CkEcs",
            "CkLabel",
            "CkLog",
            "CkSettings",
        });
    }
}
