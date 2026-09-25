using UnrealBuildTool;

public class CkProceduralAnimation : CkModuleRules
{
    public CkProceduralAnimation(ReadOnlyTargetRules Target) : base(Target)
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
            "CkJolt",
            "CkLog",
        });

        PrivateDependencyModuleNames.Add("AnimationCore");
    }
}
