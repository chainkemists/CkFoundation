using System.IO;
using UnrealBuildTool;

public class CkSpatialQuery : CkModuleRules
{
    public CkSpatialQuery(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "TraceLog",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",
            "PhysicsCore",

            "CkThirdParty",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkJolt",
            "CkLabel",
            "CkLog",

            "CkPhysics",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkShapes",
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
            });
        }
    }
}
