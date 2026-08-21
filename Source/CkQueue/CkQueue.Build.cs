using System.IO;
using UnrealBuildTool;

public class CkQueue : CkModuleRules
{
    public CkQueue(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "NavigationSystem",
            "AIModule",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkNavigation",
        });
    }
}
