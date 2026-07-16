using System.IO;
using UnrealBuildTool;

public class CkProfile : CkModuleRules
{
    public CkProfile(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkCore",
            "CkLog"
        });
    }
}
