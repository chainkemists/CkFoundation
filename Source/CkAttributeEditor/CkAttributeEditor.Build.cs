using System.IO;
using UnrealBuildTool;

public class CkAttributeEditor : CkModuleRules
{
    public CkAttributeEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "GameplayTags",
            "UnrealEd",
            "EditorStyle",
            "AssetTools",
            "KismetCompiler",
            "GraphEditor",
            "BlueprintGraph",
            "ComponentVisualizers",

            "CkAttribute",
            "CkCore",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkRecord",
            "CkLog",
        });
    }
}
