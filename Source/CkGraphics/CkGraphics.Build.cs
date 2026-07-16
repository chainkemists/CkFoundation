using System.IO;
using UnrealBuildTool;

public class CkGraphics : CkModuleRules
{
    public CkGraphics(ReadOnlyTargetRules Target) : base(Target)
    {
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
            "CkLog",
            "CkVariables"
        });
    }
}
