using UnrealBuildTool;

public class CkVoxelNavEditor : CkModuleRules
{
    public CkVoxelNavEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorFramework",
            "EditorSubsystem",
            "LevelEditor",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkJolt",
            "CkJoltEditor",
            "CkLog",
            "CkVoxelNav",
        });
    }
}
