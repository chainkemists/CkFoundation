using System.IO;
using UnrealBuildTool;

public class CkEcsExt : CkModuleRules
{
    public CkEcsExt(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivatePCHHeaderFile = "../CkEcs_PCH.h";
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
                        "GameplayTags",

            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",
            "NetCore",
            "PhysicsCore",

            "CkActor",
            "CkCore",
            "CkEcs",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
        });
    }
}
