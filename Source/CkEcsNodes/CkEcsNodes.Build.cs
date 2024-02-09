using System.IO;
using UnrealBuildTool;

public class CkEcsNodes : CkModuleRules
{
    public CkEcsNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "BlueprintGraph",
            "Core",
            "CoreUObject",
            "Engine",
            "KismetCompiler",
            "PropertyEditor",
            "StructUtils",
            "UnrealEd",

            "CkCore",
            "CkEcs",
            "CkLog",
        });
    }
}
