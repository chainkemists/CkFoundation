using System.IO;
using UnrealBuildTool;

public class CkParticles : CkModuleRules
{
    public CkParticles(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add(ModuleDirectory);

        // CK_WITH_PARTICLES is 1 only when the engine fork exposes the NiagaraEditor pin-authoring API, detected by
        // the presence of the CkNiagaraAuthoring.h marker. On a stock engine the marker is absent, CK_WITH_PARTICLES
        // is 0, and this module compiles inert so CkFoundation still builds cleanly. (See the marker header.)
        bool bCkNiagaraAuthoring = File.Exists(Path.Combine(EngineDirectory,
            "Plugins", "FX", "Niagara", "Source", "NiagaraEditor", "Public", "CkNiagaraAuthoring.h"));
        PublicDefinitions.Add("CK_WITH_PARTICLES=" + (bCkNiagaraAuthoring ? "1" : "0"));

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
