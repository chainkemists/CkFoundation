using System.IO;
using UnrealBuildTool;

public class CkResourceLoaderEditor : CkModuleRules
{
    public CkResourceLoaderEditor(ReadOnlyTargetRules Target) : base(Target)
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
            "DeveloperSettings",
            "Slate",
            "SlateCore",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkResourceLoader",
            "CkSettings",
            "CkSignal",
        });
    }
}
