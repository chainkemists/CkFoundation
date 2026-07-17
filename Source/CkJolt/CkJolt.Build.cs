using System.IO;
using UnrealBuildTool;

public class CkJolt : CkModuleRules
{
    public CkJolt(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "TraceLog",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",

            // Static-world baking reads Chaos's own cooked collision (BodySetup AggGeom +
            // TriMeshGeometries) and landscape heightmaps — parity with Chaos by construction.
            "PhysicsCore",
            "Chaos",
            "ChaosCore",
            "Landscape",

            "CkThirdParty",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLog",
            "CkSettings",
        });
    }
}
