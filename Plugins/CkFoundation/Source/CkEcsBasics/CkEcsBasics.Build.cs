using System.IO;
using UnrealBuildTool;

public class CkEcsBasics : CkModuleRules
{
    public CkEcsBasics(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkActor",
            "CkCore",
            "CkEcs",
            "CkNet",
            "CkLog"
        });
    }
}
