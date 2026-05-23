using System.IO;
using UnrealBuildTool;

public class CkAngelscriptGenerator : CkModuleRules
{
    public CkAngelscriptGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        // FPackageReader (used by self-heal Tier 2.6) lives in AssetRegistry's
        // Internal/ folder. Engine modules access it implicitly via their
        // location under Engine/Source/; plugins need the explicit path.
        PrivateIncludePaths.AddRange(new string[] {
            Path.Combine(EngineDirectory, "Source", "Runtime", "AssetRegistry", "Internal"),
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "AppFramework",
            "Core",
            "CoreUObject",
            "FieldNotification",
            "ApplicationCore",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "EditorWidgets",
            "Engine",
            "Json",
            "Merge",
            "MessageLog",
            "EditorFramework",
            "UnrealEd",
            "GraphEditor",
            "KismetWidgets",
            "KismetCompiler",
            "BlueprintGraph",
            "BlueprintEditorLibrary",
            "AnimGraph",
            "PropertyEditor",
            "SourceControl",
            "SharedSettingsWidgets",
            "InputCore",
            "EngineSettings",
            "Projects",
            "JsonUtilities",
            "DesktopPlatform",
            "HotReload",
            "JsonObjectGraph",
            "UMGEditor",
            "UMG", // for SBlueprintDiff
            "WorkspaceMenuStructure",
            "DeveloperSettings",
            "ToolMenus",
            "SubobjectEditor",
            "SubobjectDataInterface",
            "ToolWidgets",
            "TraceLog",

            "CkCore",
            "CkCVar",
            "CkDynamic",
            "CkEcs",
            "CkEcsExt",
            "CkEntityExtension",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "ToolWidgets",
            "EditorSubsystem",
            "AssetRegistry"
        });
    }
}