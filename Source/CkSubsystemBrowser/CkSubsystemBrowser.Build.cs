using System.IO;
using UnrealBuildTool;

public class CkSubsystemBrowser : CkModuleRules
{
    public CkSubsystemBrowser(ReadOnlyTargetRules Target) : base(Target)
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
            "UnrealEd",
            "EditorSubsystem",
            "PropertyEditor",
            "WorkspaceMenuStructure",

            "CkCore",
            "CkLog",
        });
    }
}
