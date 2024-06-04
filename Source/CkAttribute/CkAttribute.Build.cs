using System.IO;
using UnrealBuildTool;

public class CkAttribute : CkModuleRules
{
    public CkAttribute(ReadOnlyTargetRules Target) : base(Target)
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
            "StructUtils",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkNet",
            "CkProvider",
            "CkRecord",
            "CkSignal",
        });
    }
}
