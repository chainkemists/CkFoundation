using System.IO;
using UnrealBuildTool;

public class CkEcsExtEditor : CkModuleRules
{
    public CkEcsExtEditor(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCore",
            "CkEcs",
            "CkEcsExt",
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
                });
        }
    }
}
