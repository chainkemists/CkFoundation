using System.IO;
using UnrealBuildTool;

public class CkAttribute : CkModuleRules
{
    public CkAttribute(ReadOnlyTargetRules Target) : base(Target)
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
                        "NetCore",
            "GameplayAbilities",

            "CkCore",
            "CkEcs",
            "CkEcs",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
        });
    }
}
