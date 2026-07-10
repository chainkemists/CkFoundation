using System.IO;
using UnrealBuildTool;

public class CkVat : CkModuleRules
{
    public CkVat(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLog",
        });
    }
}
