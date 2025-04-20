using System.IO;
using UnrealBuildTool;

public class CkTargeting : CkModuleRules
{
    public CkTargeting(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "StructUtils",
            "GameplayTags",

            "CkActor",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
