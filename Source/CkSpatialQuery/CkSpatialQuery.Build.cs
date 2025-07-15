using System.IO;
using UnrealBuildTool;

public class CkSpatialQuery : CkModuleRules
{
    public CkSpatialQuery(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        SharedPCHHeaderFile = "../CkEcs_PCH.h";
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
                        "GameplayTags",
            "DeveloperSettings",
            "PhysicsCore",

            "CkThirdParty",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkPhysics",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkShapes",
        });
    }
}
