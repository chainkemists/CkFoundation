using System.IO;
using UnrealBuildTool;

public class CkK2Nodes : CkModuleRules
{
    public CkK2Nodes(ReadOnlyTargetRules Target) : base(Target)
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
            "ToolMenus",
            "GameplayTags",
            "GameplayTagsEditor",

            "CkAbility",
            "CkCore",
            "CkEcs",
            "CkEditorGraph",
            "CkLog",
            "CkMessaging",
            "CkRecord",
            "CkVariables",
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