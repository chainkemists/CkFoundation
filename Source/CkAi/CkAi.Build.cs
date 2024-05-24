using System.IO;
using UnrealBuildTool;

public class CkAi : CkModuleRules
{
    public CkAi(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "AIModule",

            "CkCore",
            "CkEcs",
            "CkLog"
        });
    }
}
