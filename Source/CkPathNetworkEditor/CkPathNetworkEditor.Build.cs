using UnrealBuildTool;

public class CkPathNetworkEditor : CkModuleRules
{
    public CkPathNetworkEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorFramework",

            "InputCore",
            "LevelEditor",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "ToolMenus",

            "CkCore",
            "CkEditorTools",
            "CkEcs",
            "CkLog",
            "CkNavigation",
            "CkPathNetwork",
        });
    }
}
