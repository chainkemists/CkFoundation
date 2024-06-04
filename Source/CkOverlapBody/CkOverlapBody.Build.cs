using System.IO;
using UnrealBuildTool;

public class CkOverlapBody : CkModuleRules
{
    public CkOverlapBody(ReadOnlyTargetRules Target) : base(Target)
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
            "DeveloperSettings",
            "StructUtils",

            "CkActor",
            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGraphics",
            "CkLabel",
            "CkLog",
            "CkNet",
            "CkPhysics",
            "CkRecord",
            "CkSettings",
            "CkSignal",
        });
    }
}
