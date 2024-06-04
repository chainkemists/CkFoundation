using System.IO;
using UnrealBuildTool;

public class CkProjectile : CkModuleRules
{
    public CkProjectile(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkNet",
            "CkPhysics",
            "CkLog",
            "CkSignal",
            "CkVariables",
        });
    }
}
