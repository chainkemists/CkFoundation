using System.IO;
using UnrealBuildTool;

public class CkPieLayoutEditor : CkModuleRules
{
    public CkPieLayoutEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "ApplicationCore",
            "Slate",
            "SlateCore",
            "UnrealEd",
            "EditorSubsystem",
            "ToolMenus",
            "DeveloperSettings",
            "Settings",
            "MainFrame",

            "CkCore",
            "CkLog",
            "CkSettings",
        });
    }
}
