using System.IO;
using UnrealBuildTool;

public class CkEditorToolbar : CkModuleRules
{
    public CkEditorToolbar(ReadOnlyTargetRules Target) : base(Target)
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
            "EditorSubsystem",
            "InputCore",
            "ToolMenus",
            "DeveloperSettings",
            "DesktopPlatform",
            "ClassViewer",
            "Blutility",
            "UMG",

            "CkCore",
            "CkLog",
            "CkSettings",
            "CkUI",
        });
    }
}
