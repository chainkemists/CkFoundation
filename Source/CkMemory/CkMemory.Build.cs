using System.IO;
using UnrealBuildTool;

public class CkMemory : CkModuleRules
{
    public CkMemory(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkCore",
            "CkLog",
        });
    }
}
