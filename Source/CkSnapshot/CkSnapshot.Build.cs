using System.IO;
using UnrealBuildTool;

public class CkSnapshot : CkModuleRules
{
    public CkSnapshot(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings", // UCk_Snapshot_Settings : UDeveloperSettings

            "CkCore",
            "CkEcs",
            "CkEcsExt", // M2b-1: FFragment_ActorSpawnIntent + Request_RebindActor for the respawn pass
            "CkLog",
            "CkThirdParty",
        });
    }
}
