using System.IO;
using UnrealBuildTool;

public class CkNet : CkModuleRules
{
    public CkNet(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",

            "NetworkTimeSync",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
        });
    }
}
