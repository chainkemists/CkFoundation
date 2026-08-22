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
            // A re-cook overwrites assets that a Git-LFS/Perforce workspace checked out read-only;
            // the cook checks them out first rather than clobbering the read-only flag.
            "SourceControl",
            "AssetRegistry",
            "ToolMenus",
            // The Content Browser context-menu entry reads the selection as FAssetData (never loading it).
            "ContentBrowser",
            // The per-user auto-cook toggles are a UDeveloperSettings subclass (CkSettings' user-settings base).
            "DeveloperSettings",
            "Landscape",
            // The mesh-shape cook reads UBodySetupCore trace flags directly.
            "PhysicsCore",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkJolt",
            "CkLog",
            "CkSettings",
            "CkThirdParty",
        });
    }
}
