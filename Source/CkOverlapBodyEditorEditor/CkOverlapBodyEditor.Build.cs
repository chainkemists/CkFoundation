using System.IO;
using UnrealBuildTool;

public class CkOverlapBodyEditor : CkModuleRules
{
    public CkOverlapBodyEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "UnrealEd",

            "CkCore",
            "CkLog",
        });
    }
}
