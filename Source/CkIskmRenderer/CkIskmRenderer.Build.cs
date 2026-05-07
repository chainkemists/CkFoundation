using System.IO;
using UnrealBuildTool;

public class CkIskmRenderer : CkModuleRules
{
    public CkIskmRenderer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            "AnimGraphRuntime",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkGraphics",

            "CkProvider",
            "CkRecord",
            "CkSettings",

            "CkAnimation",
            "CkPhysics",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
