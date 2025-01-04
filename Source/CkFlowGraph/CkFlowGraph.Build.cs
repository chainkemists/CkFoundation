using System.IO;
using UnrealBuildTool;

public class CkFlowGraph : CkModuleRules
{
    public CkFlowGraph(ReadOnlyTargetRules Target) : base(Target)
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

            "Flow",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
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
