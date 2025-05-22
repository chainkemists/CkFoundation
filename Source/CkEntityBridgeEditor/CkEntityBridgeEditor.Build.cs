using System.IO;
using UnrealBuildTool;

public class CkEntityBridgeEditor : CkModuleRules
{
    public CkEntityBridgeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        SharedPCHHeaderFile = "../CkEcs_PCH.h";
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
            "StructUtils",
            "ToolMenus",
            "GameplayTags",
            "UnrealEd",
            "EditorStyle",
            "AssetTools",
            "KismetCompiler",
            "GraphEditor",
            "BlueprintGraph",
            "StructUtilsEngine",

            "CkCore",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkEntityBridge",
            "CkLog",
        });
    }
}
