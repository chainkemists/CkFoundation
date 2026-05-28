using System.IO;
using UnrealBuildTool;

public class CkAssetExporter : CkModuleRules
{
    public CkAssetExporter(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            ModuleDirectory,
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
            "AssetTools",
            "AssetRegistry",
            "ContentBrowser",
            "WorkspaceMenuStructure",
            "AIModule",
            "GameplayTasks",
            "Json",
            "JsonUtilities",
            "BlueprintGraph",
            "KismetCompiler",
            "GameplayTags",
            "StructUtils",
            "UMG",
            "UMGEditor",

            "CkCore",
            "CkEcs",
            "CkAi",
            "CkLog",
        });
    }
}
