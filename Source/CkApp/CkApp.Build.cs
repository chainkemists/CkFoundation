using System.IO;
using UnrealBuildTool;

public class CkApp : CkModuleRules
{
    public CkApp(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkAbility",
            "CkActor",
            "CkAnimation",
            "CkAttribute",
            "CkCamera",
            "CkCore",
            "CkEcs",
            "CkEcsBasics",
            "CkIntent",
            "CkNet",
            "CkLog",
            "CkOverlapBody",
            "CkPhysics",
            "CkProjectile",
            "CkRecord",
            "CkResourceLoader",
            "CkTimer",
            "CkUnreal",
        });
    }
}
