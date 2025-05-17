using System.IO;
using UnrealBuildTool;

public class CkGraphics : CkModuleRules
{
    public CkGraphics(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseSharedPCHs;
        PrivatePCHHeaderFile = "../CkEcs_PCH.h";
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

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSignal",
            "CkVariables"
        });
    }
}
