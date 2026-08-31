using System.IO;
using UnrealBuildTool;

public class CkEqs : CkModuleRules
{
    public CkEqs(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "GameplayTags",
            "DeveloperSettings",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkEntityTag",
            "CkLabel",
            "CkLog",
            "CkNavigation",
            "CkRecord",
            "CkSettings",
            "CkShapes",
            "CkSpatialQuery",
            "CkThirdParty",
        });
    }
}
