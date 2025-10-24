using System.IO;
using UnrealBuildTool;

public class CkAudioEditor : CkModuleRules
{
    public CkAudioEditor(ReadOnlyTargetRules Target) : base(Target)
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

            "CkAudio",
            "CkCore",
            "CkCue",
            "CkCueEditor",
            "CkEcs",
            "CkEditorGraph",
            "CkEditorStyle",
            "CkLog",
        });
    }
}
