using System.IO;
using UnrealBuildTool;

public class CkPhysics : CkModuleRules
{
    public CkPhysics(ReadOnlyTargetRules Target) : base(Target)
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
            "GameplayTags",
            "GeometryCollectionEngine",
            "FieldSystemEngine",
            "PhysicsCore",

            "IrisCore",
            "NetCore",

            "CkActor",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkEntityBridge",
            "CkLabel",
            "CkLog",

            "CkRecord",
        });
    }
}
