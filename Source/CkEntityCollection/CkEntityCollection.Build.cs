using System.IO;
using UnrealBuildTool;

public class CkEntityCollection : CkModuleRules
{
    public CkEntityCollection(ReadOnlyTargetRules Target) : base(Target)
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

            "IrisCore",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkRecord",
            "CkSettings",
        });
    }
}
