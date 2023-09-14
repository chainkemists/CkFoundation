using System.IO;
using UnrealBuildTool;

public class CkUnreal : CkModuleRules
{
    public CkUnreal(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "NetworkTimeSync",

            "CkActor",
            "CkAttribute",
            "CkCore",
            "CkEcs",
            "CkEcsBasics",
            "CkIntent",
            "CkNet",
            "CkLog",
            "CkOverlapBody",
            "CkPhysics",
            "CkProjectile",
            "CkRecord"
        });
    }
}
