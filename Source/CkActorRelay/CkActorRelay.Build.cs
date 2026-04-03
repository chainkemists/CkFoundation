using UnrealBuildTool;

public class CkActorRelay : CkModuleRules
{
    public CkActorRelay(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "NetCore",

            "DeveloperSettings",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkSettings",
        });
    }
}
