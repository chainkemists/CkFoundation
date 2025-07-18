using System.IO;
using UnrealBuildTool;

public class CkCamera : CkModuleRules
{
    public CkCamera(ReadOnlyTargetRules Target) : base(Target)
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
