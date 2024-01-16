using System.IO;
using UnrealBuildTool;

public class CkVariables : CkModuleRules
{
    public CkVariables(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "StructUtils",

            "CkCore",
            "CkEcs",
            "CkLog",
        });
    }
}
