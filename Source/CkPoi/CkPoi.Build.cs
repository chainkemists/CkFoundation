using System.IO;
using UnrealBuildTool;

public class CkPoi : CkModuleRules
{
    public CkPoi(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkRecord",
            "CkSettings",
            "CkTimer",
        });
    }
}
