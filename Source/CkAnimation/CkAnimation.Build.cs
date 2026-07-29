using System.IO;
using UnrealBuildTool;

public class CkAnimation : CkModuleRules
{
    public CkAnimation(ReadOnlyTargetRules Target) : base(Target)
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
            "AnimGraphRuntime",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProvider",
            "CkRecord",
            "CkResourceLoader",
        });
    }
}
