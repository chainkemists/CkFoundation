using System.IO;
using UnrealBuildTool;

public class CkSpatialQuery : CkModuleRules
{
    public CkSpatialQuery(ReadOnlyTargetRules Target) : base(Target)
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

            "CkThirdParty",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkNet",
            "CkPhysics",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
