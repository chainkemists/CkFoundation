using System.IO;
using UnrealBuildTool;

public class CkLogEditor : CkModuleRules
{
    public CkLogEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Slate",
            "SlateCore",
            "Engine",
            "InputCore",
            "BlueprintGraph",
            "GraphEditor",
            "UnrealEd",
            "EditorStyle",
            "AssetTools",
            "KismetCompiler",

            "CkCore",
            "CkLog",
        });
    }
}
