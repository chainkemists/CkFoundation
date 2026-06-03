using UnrealBuildTool;

public class CkGridEditor : CkModuleRules
{
    public CkGridEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorFramework",

            "InputCore",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "LevelEditor",
            "PropertyEditor",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGrid",
            "CkEntitySpawner",
            "CkLog",
        });
    }
}
