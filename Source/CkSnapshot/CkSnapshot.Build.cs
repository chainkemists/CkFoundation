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
            "GameplayTags",      // FFragment_GameplayLabel::SerializeSnapshot -> FGameplayTag::StaticStruct

            "CkCore",
            "CkEcs",
            "CkLabel", // FFragment_GameplayLabel snapshot registration
            "CkLog",
            "CkThirdParty",
        });
    }
}
