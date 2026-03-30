using System.IO;
using UnrealBuildTool;

public class CkCVar : CkModuleRules
{
    public CkCVar(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",

            "CkCore",
            "CkLog",
        });
    }
}
