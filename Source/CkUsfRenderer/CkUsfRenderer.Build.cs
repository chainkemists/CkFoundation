using UnrealBuildTool;

// Global shaders must register at PostConfigInit. Keep CK/AngelScript dependencies in CkUsf,
// just as CkPixelArtRenderer keeps its early renderer separate from the gameplay module.
public class CkUsfRenderer : ModuleRules
{
    public CkUsfRenderer(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "Renderer", "RenderCore", "RHI", "Projects" });
    }
}
