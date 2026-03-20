using System.IO;
using UnrealBuildTool;

public class CkInventory : CkModuleRules
{
    public CkInventory(ReadOnlyTargetRules Target) : base(Target)
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
            "StructUtils",

            "IrisCore",
            "NetCore",

            "CkAttribute",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGrid",
            "CkLabel",
            "CkLog",

            "CkRecord",
            "CkSettings",
        });
    }
}
