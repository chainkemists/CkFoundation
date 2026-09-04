using UnrealBuildTool;

public class CkGroundNavEditor : CkModuleRules
{
    public CkGroundNavEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            // This module exists to WRITE ground-nav assets from the editor, which is UnrealEd's
            // package and asset-tools surface - the same dependency CkJoltEditor carries to write its own.
            "UnrealEd",

            "CkCore",
            "CkEcs",
            "CkGroundNav",
            "CkLog",
        });
    }
}
