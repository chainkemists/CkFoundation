using System.IO;
using UnrealBuildTool;

public class CkEntityExtension : CkModuleRules
{
    public CkEntityExtension(ReadOnlyTargetRules Target) : base(Target)
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
            "StructUtils",
            "GameplayTags",

            "IrisCore",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
