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
            "StructUtils",

            "CkAttribute",
            "CkCore",
            "CkEcs",
            "CkEcsBasics",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
            "CkSignal",
            "CkNet",
            "CkEntityBridge",
        });
    }
}
