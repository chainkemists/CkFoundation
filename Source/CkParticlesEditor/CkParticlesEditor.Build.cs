using UnrealBuildTool;

public class CkParticlesEditor : CkModuleRules
{
    public CkParticlesEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RHI",
            "UnrealEd",
            "EditorSubsystem",
            "AssetTools",
            "AssetRegistry",

            "Niagara",
            "NiagaraEditor",

            "CkCore",
            "CkLog",
            "CkParticles",
        });
    }
}
