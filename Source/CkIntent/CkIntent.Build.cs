using System.IO;
using UnrealBuildTool;

public class CkIntent : CkModuleRules
{
    public CkIntent(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkActor"
        });
    }
}
