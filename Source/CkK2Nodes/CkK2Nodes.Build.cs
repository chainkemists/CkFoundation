using System.IO;
using UnrealBuildTool;

public class CkK2Nodes : CkModuleRules
{
    public CkK2Nodes(ReadOnlyTargetRules Target) : base(Target)
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
