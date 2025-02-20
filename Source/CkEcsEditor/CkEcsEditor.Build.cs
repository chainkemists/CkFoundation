using System.IO;
using UnrealBuildTool;

public class CkEcsEditor : CkModuleRules
{
    public CkEcsEditor(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkEditorGraph",
            "CkLog",
        });

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "EditorStyle",
                    "AssetTools",
                    "KismetCompiler",
                    "GraphEditor",
                    "BlueprintGraph",
                    "StructUtilsEngine",
                });
        }
    }
}
