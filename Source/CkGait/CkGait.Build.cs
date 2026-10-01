using UnrealBuildTool;

public class CkGait : CkModuleRules
{
    public CkGait(ReadOnlyTargetRules Target) : base(Target)
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
