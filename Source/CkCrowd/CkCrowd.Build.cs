using System.IO;
using UnrealBuildTool;

public class CkCrowd : CkModuleRules
{
    public CkCrowd(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "DeveloperSettings",

            "GameplayTags",

            "NavigationSystem",   // for ProjectPointToNavigation in DrawNavProjection processor

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",

            "CkPhysics",
            "CkPmg",            // for FProcessor_CrowdAgent_DrawBody_Setup body capsule + cone
            "CkProjectile",     // FProcessor_CrowdAgent_ApplyOffset RunAfter FProcessor_Projectile_Update (shared FTag_EulerIntegrator_NeedsUpdate marker)
            "CkShapes",
            "CkSpatialQuery",

            "CkNavigation",
            "CkQueue",
            "CkPathNetwork",   // follower agents route MoveTo through the path network (corridor → nav-path install seam)
            "CkVoxelNav",      // volumetric agents route MoveTo through a baked volume (voxel path → nav-path install seam)
        });
    }
}
