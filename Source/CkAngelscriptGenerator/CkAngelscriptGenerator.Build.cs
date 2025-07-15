using System.IO;
using UnrealBuildTool;

public class CkAngelscriptGenerator : CkModuleRules
{
    public CkAngelscriptGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        SharedPCHHeaderFile = "../CkEcs_PCH.h";
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
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
            "CkEcs",
            "CkEcs",
            "CkEntityExtension",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
            "CkEntityBridge",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "ToolWidgets",
            "EditorSubsystem"
        });
    }
}