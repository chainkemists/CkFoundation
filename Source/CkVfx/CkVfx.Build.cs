using System.IO;
using UnrealBuildTool;

public class CkVfx : CkModuleRules
{
    public CkVfx(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "Niagara",

            "CkActorRelay",
            "CkCore",
            "CkCue",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkTimer"
        });
    }
}
