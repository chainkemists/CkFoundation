using System.IO;
using UnrealBuildTool;

public class CkVisibleRange : CkModuleRules
{
    public CkVisibleRange(ReadOnlyTargetRules Target) : base(Target)
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
            "CkEcsExt",
            "CkLog",
        });
    }
}
