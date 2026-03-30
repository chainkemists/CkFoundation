using System.IO;
using UnrealBuildTool;

public class CkCVarEditor : CkModuleRules
{
    public CkCVarEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Slate",
            "SlateCore",
            "Engine",
            "InputCore",
            "BlueprintGraph",
            "GraphEditor",
            "UnrealEd",
            "AssetTools",
            "KismetCompiler",

            "CkCore",
            "CkCVar",
            "CkEditorGraph",
            "CkLog",
        });
    }
}
