using System.IO;
using UnrealBuildTool;

public class CkGameSession : CkModuleRules
{
    public CkGameSession(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkCore",
            "CkEcs",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
        });
    }
}
