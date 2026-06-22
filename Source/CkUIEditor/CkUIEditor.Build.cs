using System.IO;
using UnrealBuildTool;

public class CkUIEditor : CkModuleRules
{
    public CkUIEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "UnrealEd",
            "PropertyEditor",
            "UMG",

            "CkUI",
            "CkGraphics",
            "CkEcs",
            "CkCore",
            "CkLog",
        });
    }
}
