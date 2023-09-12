using System.IO;
using UnrealBuildTool;

public class CkTimer : CkModuleRules
{
    public CkTimer(ReadOnlyTargetRules Target) : base(Target)
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
            "CkLog",
            "CkNet",
            "CkSignal"
        });
    }
}
