using System.IO;
using UnrealBuildTool;

public class CkResolver : CkModuleRules
{
    public CkResolver(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        SharedPCHHeaderFile = "../CkEcs_PCH.h";
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

            "CkAttribute",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkTargeting"
        });
    }
}
