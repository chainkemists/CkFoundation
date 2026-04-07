using System.IO;
using UnrealBuildTool;

public class CkCueEditor : CkModuleRules
{
    public CkCueEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "CkCue",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkLog",
            "CkUI"
        });
    }
}
