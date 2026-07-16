using System.IO;
using UnrealBuildTool;

public class CkAi : CkModuleRules
{
    public CkAi(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkLog"
        });
    }
}
