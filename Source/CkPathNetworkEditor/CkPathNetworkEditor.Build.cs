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

            "InputCore",
            "NavigationSystem",
            "PropertyEditor",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkPathNetwork",
        });
    }
}
