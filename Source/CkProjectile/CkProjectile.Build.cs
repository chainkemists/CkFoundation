using System.IO;
using UnrealBuildTool;

public class CkProjectile : CkModuleRules
{
    public CkProjectile(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseSharedPCHs;
        PrivatePCHHeaderFile = "../CkEcs_PCH.h";
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

            "CkPhysics",
            "CkLog",
            "CkSignal",
            "CkVariables",
        });
    }
}
