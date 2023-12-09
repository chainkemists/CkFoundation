using System.IO;
using UnrealBuildTool;

public class CkEditorStyle : CkModuleRules
{
    public CkEditorStyle(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "AssetTools",
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "BlueprintGraph",
            "Kismet",
            "KismetCompiler",
            "DetailCustomizations",
            "GraphEditor",
            "Projects",
            "PropertyEditor",
            "EditorStyle",
            "InputCore",
            "ToolMenus",

            "CkCore",
        });
    }
}
