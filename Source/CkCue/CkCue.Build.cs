using UnrealBuildTool;

public class CkCue : CkModuleRules
{
    public CkCue(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "UMG",
            "Slate",
            "SlateCore",
            "CommonUI",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkProvider",
            "CkSettings",
            "CkTimer",
            "CkUI",
        });
    }
}
