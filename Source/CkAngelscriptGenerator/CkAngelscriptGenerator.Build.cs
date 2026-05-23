using System.IO;
using UnrealBuildTool;

public class CkAngelscriptGenerator : CkModuleRules
{
    public CkAngelscriptGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        // The self-heal Tier 2.6 fallback uses FPackageReader (declared in
        // Engine/Source/Runtime/AssetRegistry/Internal/) to read parent-class
        // info directly from a .uasset when AR's tag cache is poisoned (see
        // CkAngelscriptGenerator_AssetRegistryStub.cpp Resolve_AssetClass_ViaPackageReader).
        // UnrealEd accesses it via its `Engine/Source/`-inside location; plugin
        // modules outside Engine/Source need an explicit path.
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

        // FPackageReader lives under AssetRegistry's Internal/ folder.
        // The self-heal Tier 2.6 fallback uses it to read parent-class
        // info directly from a .uasset when AR's tag cache is poisoned
        // (see CkAngelscriptGenerator_AssetRegistryStub.cpp).
        PrivateIncludePathModuleNames.Add("AssetRegistry");
    }
}