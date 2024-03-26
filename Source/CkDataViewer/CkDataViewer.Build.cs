using System.IO;
using UnrealBuildTool;

public class CkDataViewer : CkModuleRules
{
    public CkDataViewer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "BlueprintGraph",
            "Slate",
            "Kismet",

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
