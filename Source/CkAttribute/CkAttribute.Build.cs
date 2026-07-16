using System.IO;
using UnrealBuildTool;

public class CkAttribute : CkModuleRules
{
    public CkAttribute(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
