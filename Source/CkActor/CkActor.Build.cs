using System.IO;
using UnrealBuildTool;

public class CkActor : CkModuleRules
{
    public CkActor(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSignal",
            "CkVariables"
        });
    }
}
