using System.IO;
using UnrealBuildTool;

public class CkJoltEditor : CkModuleRules
{
    public CkJoltEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorSubsystem",
            "AssetRegistry",
            "ToolMenus",
            "Landscape",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkJolt",
            "CkLog",
            "CkThirdParty",
        });
    }
}
