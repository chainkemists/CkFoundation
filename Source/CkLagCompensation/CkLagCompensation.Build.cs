using System.IO;
using UnrealBuildTool;

public class CkLagCompensation : CkModuleRules
{
    public CkLagCompensation(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLog",
            "CkProjectile",
            "CkShapes",
        });
    }
}
