using System.IO;
using UnrealBuildTool;

public class CkVisualLod : CkModuleRules
{
    public CkVisualLod(ReadOnlyTargetRules Target) : base(Target)
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

            "CkCamera",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkIskmRenderer",
            "CkLog",
            "CkResourceLoader",
        });
    }
}
