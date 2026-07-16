using System.IO;
using UnrealBuildTool;

public class CkEcsEditor : CkModuleRules
{
    public CkEcsEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "InputCore",
            "ToolMenus",
            "GameplayTags",
            "UnrealEd",
            "EditorStyle",
            "AssetTools",
            "KismetCompiler",
            "GraphEditor",
            "BlueprintGraph",
            "PropertyEditor",
            "AssetRegistry",
            "ContentBrowser",
            "EditorSubsystem",
            "WorkspaceMenuStructure",
            "LevelEditor",

            "CkCore",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkLog",
        });
    }
}
