using System.IO;
using UnrealBuildTool;

public class CkIsmRenderer : CkModuleRules
{
    public CkIsmRenderer(ReadOnlyTargetRules Target) : base(Target)
    {
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
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkGraphics",

            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
