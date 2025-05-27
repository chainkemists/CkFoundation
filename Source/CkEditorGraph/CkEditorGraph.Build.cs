using System.IO;
using UnrealBuildTool;

public class CkEditorGraph : CkModuleRules
{
    public CkEditorGraph(ReadOnlyTargetRules Target) : base(Target)
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
            "InputCore",
                        "ToolMenus",

            "CkCore",
            "CkLog",
        });

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "EditorStyle",
                    "AssetTools",
                    "KismetCompiler",
                    "GraphEditor",
                    "BlueprintGraph"
                });
        }
    }
}
