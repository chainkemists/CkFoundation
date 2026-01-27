using System.IO;
using UnrealBuildTool;

public class CkDynamicEditor : CkModuleRules
{
    public CkDynamicEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            // Core Engine
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "GameplayTags",

            // Editor
            "UnrealEd",
            "EditorStyle",
            "EditorWidgets",
            "EditorSubsystem",

            // UI
            "Slate",
            "SlateCore",
            "UMG",
            "CommonUI",

            // Tools & Utilities
            "ToolMenus",
            "AssetTools",
            "AssetRegistry",
            "ContentBrowser",
            "PropertyEditor",
            "DetailCustomizations",
            "ToolWidgets",
            "WorkspaceMenuStructure",
            "LevelEditor",
            "DesktopPlatform",
            "Projects",

            // Blueprint System
            "KismetCompiler",
            "GraphEditor",
            "BlueprintGraph",

            // Editor Utilities
            "EditorScriptingUtilities",
            "Blutility",

            // Custom Modules
            "CkCore",
            "CkDynamic",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkLog",
            "CkUI"
        });
    }
}
