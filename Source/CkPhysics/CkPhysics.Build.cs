using System.IO;
using UnrealBuildTool;

public class CkPhysics : CkModuleRules
{
    public CkPhysics(ReadOnlyTargetRules Target) : base(Target)
    {
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
            "CkChaos",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkRecord",
        });
    }
}
