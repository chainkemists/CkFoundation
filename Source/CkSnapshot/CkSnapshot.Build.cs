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
            "CoreOnline", // FUniqueNetIdWrapper::ToString — v3 EngineOwned player rendezvous (spec §4.2)
            "Engine",
            "DeveloperSettings", // UCk_Snapshot_Settings and UCk_Plugin_ProjectSettings_UE both derive UDeveloperSettings
            "ImageCore",         // FImage::ChangeFormat — reading back the HDR screenshot path's .exr

            "CkCore",
            "CkEcs",
            "CkEcsExt", // FFragment_ActorSpawnIntent + FTag_ActorRespawn_Pending for the respawn marker pass
            "CkLabel",  // GameplayLabel — v3 ConstructSpawned identity (spec §4.2)
            "CkLog",
            "CkSettings", // UCk_Plugin_ProjectSettings_UE — the posture fence's project-supplied inputs
            "CkThirdParty",
        });
    }
}
