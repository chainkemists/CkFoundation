using UnrealBuildTool;

public class CkEntitySpawner : CkModuleRules
{
    public CkEntitySpawner(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",

            "CkActorRelay",
            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
        });
    }
}
