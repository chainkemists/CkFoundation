using UnrealBuildTool;

// Engine-only module (NO Ck deps, NOT CkModuleRules — CkModuleRules pulls AngelscriptCode/ApplicationCore which
// cannot load at PostConfigInit). Loaded at PostConfigInit (see CkFoundation.uplugin) so the batched-skinning
// FVertexFactory type registers at static-init BEFORE the engine seals the vertex-factory list.
public class CkIskmRendererVF : ModuleRules
{
    public CkIskmRendererVF(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Renderer",
            "Projects",
        });
    }
}
