using System.IO;
using UnrealBuildTool;

public class CkGameSettings : CkModuleRules
{
    public CkGameSettings(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",

            "CkCore",
            "CkCVar",
            "CkLog",
            "CkSettings",
        });
    }
}
