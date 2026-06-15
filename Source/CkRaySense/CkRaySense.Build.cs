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
                        "GameplayTags",
            "PhysicsCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkShapes",
            "CkIsmRenderer",    // RaySense PostTransform processors RunAfter the IsmProxy PostTransform processors (shared FTag_Transform_Updated marker)
        });
    }
}
