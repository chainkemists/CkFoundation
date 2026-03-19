using System.IO;
using UnrealBuildTool;

public class CkInventoryEditor : CkModuleRules
{
    public CkInventoryEditor(ReadOnlyTargetRules Target) : base(Target)
    {
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
            "ToolMenus",
            "UnrealEd",
            "EditorStyle",
            "AssetTools",
            "KismetCompiler",
            "GraphEditor",
            "BlueprintGraph",
            "StructUtils",

            "CkCore",
            "CkDynamic",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkInventory",
            "CkLog",
        });
    }
}
