using System.IO;
using UnrealBuildTool;

public class CkAntBridge : CkModuleRules
{
    public CkAntBridge(ReadOnlyTargetRules Target) : base(Target)
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

            "Ant",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkIsmRenderer",
            "CkLabel",
            "CkLog",
            "CkNet",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
