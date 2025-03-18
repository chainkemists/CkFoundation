using System.IO;
using UnrealBuildTool;

public class CkFeedback : CkModuleRules
{
    public CkFeedback(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "StructUtils",
            "GameplayTags",

            "CkAI",
            "CkCamera",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkFx",
            "CkLabel",
            "CkLog",
            "CkNet",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
