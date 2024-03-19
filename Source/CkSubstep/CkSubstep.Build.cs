using System.IO;
using UnrealBuildTool;

public class CkSubstep : CkModuleRules
{
    public CkSubstep(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkCore",
            "CkEcs",
            "CkEcsBasics",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
