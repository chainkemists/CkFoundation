using System.IO;
using UnrealBuildTool;

public class CkObjective : CkModuleRules
{
    public CkObjective(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            "CkAttribute",
            "CkActorRelay",
            "CkCore",
            "CkCue",
            "CkEcs",
            "CkEcsExt",
            "CkEntityCollection",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
        });
    }
}
