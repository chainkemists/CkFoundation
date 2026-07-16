using System.IO;
using UnrealBuildTool;

public class CkAudio : CkModuleRules
{
    public CkAudio(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

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
