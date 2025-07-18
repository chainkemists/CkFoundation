using System.IO;
using UnrealBuildTool;

public class CkActorProxy : CkModuleRules
{
    public CkActorProxy(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "../CkEcs_PCH.h";
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkCore",
            "CkEcs",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
            "CkEntityBridge",
        });
    }
}
