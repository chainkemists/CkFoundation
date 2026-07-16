using System.IO;
using UnrealBuildTool;

public class CkTagSet : CkModuleRules
{
    public CkTagSet(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "NetCore",

            "CkCore",
            "CkEcs",
            "CkLog",
        });
    }
}
