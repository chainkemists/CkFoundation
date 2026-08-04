using System.IO;
using UnrealBuildTool;

public class CkVoxelNav : CkModuleRules
{
    public CkVoxelNav(ReadOnlyTargetRules Target) : base(Target)
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

            "CkAStar",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkJolt",
            "CkLabel",
            "CkLog",
            "CkNavigation",
            "CkProfile",
            "CkRecord",
            "CkSettings",
            "CkThirdParty",
        });
    }
}
