using System.IO;
using UnrealBuildTool;

public class CkUnrealComponent : CkModuleRules
{
    public CkUnrealComponent(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.Add("CkProfile");

        PrivateIncludePaths.AddRange(new string[] {
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
            // The Jolt static-world bake opt-in (Request_BakeIntoJoltStaticWorld) — same-tier dep.
            "CkJolt",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",
        });
    }
}
