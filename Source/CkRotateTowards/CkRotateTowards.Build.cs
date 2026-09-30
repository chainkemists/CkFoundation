using UnrealBuildTool;

public class CkRotateTowards : CkModuleRules
{
    public CkRotateTowards(ReadOnlyTargetRules Target) : base(Target)
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
