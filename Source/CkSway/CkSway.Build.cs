using UnrealBuildTool;

public class CkSway : CkModuleRules
{
    public CkSway(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLog",
            "CkSettings"
        });
    }
}
