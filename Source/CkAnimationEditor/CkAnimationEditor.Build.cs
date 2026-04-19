using System.IO;
using UnrealBuildTool;

public class CkAnimationEditor : CkModuleRules
{
    public CkAnimationEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        // Expose the module root so deep files under Public/ can include the _Log / _Module headers
        // that live at the module root (matches the CkCueEditor layout).
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            // Core Engine
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",

            // Editor
            "UnrealEd",
            "EditorStyle",
            "EditorWidgets",
            "EditorSubsystem",
            "DeveloperSettings",

            // UI
            "Slate",
            "SlateCore",
            "UMG",

            // Tools & Utilities
            "ToolMenus",
            "AssetTools",
            "AssetRegistry",
            "ContentBrowser",
            "PropertyEditor",
            "ToolWidgets",
            "WorkspaceMenuStructure",
            "LevelEditor",
            "DesktopPlatform",
            "Projects",
            "SourceControl",

            // Animation
            "AnimationCore",
            "AnimGraph",
            "AnimGraphRuntime",
            "AnimationBlueprintLibrary",
            "AnimationEditor",

            // Editor Utilities
            "EditorScriptingUtilities",
            "Blutility",

            // Custom Modules
            "CkCore",
            "CkEditorStyle",
            "CkLog",
        });
    }
}
