using System.IO;
using UnrealBuildTool;

public class CkAbility : CkModuleRules
{
    public CkAbility(ReadOnlyTargetRules Target) : base(Target)
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
            "GameplayAbilities",
            "DeveloperSettings",

            "IrisCore",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkEntityExtension",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
            "CkEntityBridge",
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "Slate",
                "UnrealEd",
            });
        }
    }
}
