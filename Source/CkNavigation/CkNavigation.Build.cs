using System.IO;
using UnrealBuildTool;

public class CkNavigation : CkModuleRules
{
    public CkNavigation(ReadOnlyTargetRules Target) : base(Target)
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
            "DeveloperSettings",
            "AIModule",
            "NavigationSystem",
            "Navmesh",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLog",
            "CkSettings",
            "CkThirdParty",
            "CkProfile",
        });
    }
}
