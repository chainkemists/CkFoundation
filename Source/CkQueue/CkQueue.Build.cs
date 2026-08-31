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
            "AIModule",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkEntityTag",
            "CkLabel",
            "CkLog",
            "CkNavigation",
        });
    }
}
