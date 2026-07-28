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

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            // FFragment_SaveKey — the channel entity's stable save/load rendezvous identity. Private: only the
            // channel actor's .cpp stamps it, so consumers of this module do not inherit the link.
            "CkSnapshot",
        });
    }
}
