using UnrealBuildTool;

public class CkEntitySpawnerEditor : CkModuleRules
{
    public CkEntitySpawnerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",

            "ImageWrapper",
            "Projects",

            "InputCore",
            "PropertyEditor",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkEntitySpawner",
            "CkLog",
        });
    }
}
