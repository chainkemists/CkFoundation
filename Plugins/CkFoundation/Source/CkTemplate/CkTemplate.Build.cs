using System.IO;
using UnrealBuildTool;

public class CkTemplate : CkModuleRules
{
    public CkTemplate(ReadOnlyTargetRules Target) : base(Target)
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
            "CkLog"
        });
    }
}
