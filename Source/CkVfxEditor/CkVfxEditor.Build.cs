using System.IO;
using UnrealBuildTool;

public class CkVfxEditor : CkModuleRules
{
    public CkVfxEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
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

            "CkCore",
            "CkCue",
            "CkCueEditor",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkLog",
            "CkVfx",
        });
    }
}
