using System.IO;
using UnrealBuildTool;

public class CkMinimap : CkModuleRules
{
    public CkMinimap(ReadOnlyTargetRules Target) : base(Target)
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

            "UMG",
            "Slate",
            "SlateCore",
            "CommonUI",

            "CkCamera",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkPoi",
            "CkRecord",
            "CkUI",
        });
    }
}
