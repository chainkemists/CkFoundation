using UnrealBuildTool;

public class CkParticles : CkModuleRules
{
    public CkParticles(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
            "Projects",
            "GameplayTags",

            "Niagara",

            "CkCore",
            "CkLog",
        });
    }
}
