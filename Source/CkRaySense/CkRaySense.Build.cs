using System.IO;
using UnrealBuildTool;

public class CkRaySense : CkModuleRules
{
    public CkRaySense(ReadOnlyTargetRules Target) : base(Target)
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
            "PhysicsCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkNet",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkShapes",
            "CkSignal",
        });
    }
}
