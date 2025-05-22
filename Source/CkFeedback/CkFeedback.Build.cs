using System.IO;
using UnrealBuildTool;

public class CkFeedback : CkModuleRules
{
    public CkFeedback(ReadOnlyTargetRules Target) : base(Target)
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

            "CkAi",
            "CkCamera",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkFx",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
