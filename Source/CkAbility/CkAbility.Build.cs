using System.IO;
using UnrealBuildTool;

public class CkAbility : CkModuleRules
{
    public CkAbility(ReadOnlyTargetRules Target) : base(Target)
    {
	    bUseUnity = false;

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
            "StructUtils",

            "IrisCore",
            "NetCore",

            "BlueprintNodeTemplate",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkEntityExtension",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
            "CkSignal",
            "CkNet",
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
