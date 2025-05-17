using System.IO;
using UnrealBuildTool;

public class CkEcsExtEditor : CkModuleRules
{
    public CkEcsExtEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseSharedPCHs;
        PrivatePCHHeaderFile = "../CkEcs_PCH.h";
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
                    "StructUtilsEngine",
                });
        }
    }
}
