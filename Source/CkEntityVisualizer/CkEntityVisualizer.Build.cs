using UnrealBuildTool;

public class CkEntityVisualizer : CkModuleRules
{
    public CkEntityVisualizer(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGraphics",
            "CkIsmRenderer",
            "CkLog",
            "CkPmg",
            "CkShapes",
            "CkSpatialQuery",
            "CkSettings",
        });
    }
}
