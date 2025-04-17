using System.IO;
using UnrealBuildTool;

public class CkAbilityEditor : CkModuleRules
{
    public CkAbilityEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "CkAbility",
            "CkEditorStyle",
            "CkEditorGraph",
            "CkLog",
        });
    }
}
