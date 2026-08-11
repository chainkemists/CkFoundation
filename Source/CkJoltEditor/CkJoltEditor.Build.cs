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
            // The mesh-shape cook reads UBodySetupCore trace flags directly.
            "PhysicsCore",
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
