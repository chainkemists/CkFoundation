using System.IO;
using UnrealBuildTool;

public class CkDialog : CkModuleRules
{
    public CkDialog(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",     // UCk_Dialog_ProjectSettings_UE derives from UDeveloperSettingsBackedByCVars

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkEntityTag",
            "CkLabel",               // TUtils_RecordOfEntities (condition record) calls UCk_Utils_GameplayLabel_UE
            "CkLog",

            "CkProfile",
            "CkRecord",
            "CkSettings",
        });
    }
}
