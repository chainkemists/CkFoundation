using UnrealBuildTool;

public class CkChain : CkModuleRules
{
    public CkChain(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "GameplayTags",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings"
        });
    }
}
