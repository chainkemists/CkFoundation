using System.IO;
using UnrealBuildTool;

public class CkObjectiveEditor : CkModuleRules
{
    public CkObjectiveEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "CkObjective",
        });
    }
}
