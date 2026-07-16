using System.IO;
using UnrealBuildTool;

public class CkEntityExtension : CkModuleRules
{
    public CkEntityExtension(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
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
            "CkProvider",
            "CkRecord",
            "CkSettings",
        });
    }
}
