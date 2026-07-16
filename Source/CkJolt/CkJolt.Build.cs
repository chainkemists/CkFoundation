using System.IO;
using UnrealBuildTool;

public class CkJolt : CkModuleRules
{
    public CkJolt(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "TraceLog",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",

            "CkThirdParty",
            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
        });
    }
}
