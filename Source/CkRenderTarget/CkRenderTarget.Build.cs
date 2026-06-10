using System.IO;
using UnrealBuildTool;

public class CkRenderTarget : CkModuleRules
{
    public CkRenderTarget(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "GameplayTags",
            "NetCore",
            "RenderCore",
            "RHI",

            "CkActorRelay",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkProfile",
            "CkRecord",
            "CkSettings",
            "CkTimer",
        });
    }
}
