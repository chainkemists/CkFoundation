using System.IO;
using UnrealBuildTool;

public class CkChaos : CkModuleRules
{
    public CkChaos(ReadOnlyTargetRules Target) : base(Target)
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

            "NetCore",

            "Chaos",
            "GeometryCollectionEngine",

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
            "CkTargeting",
        });
    }
}
