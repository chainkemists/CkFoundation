using System.IO;
using UnrealBuildTool;

// Engine-only module (NO Ck deps, NOT CkModuleRules — CkModuleRules publicly adds AngelscriptCode, whose plugin
// module loads at PostDefault, and ApplicationCore; neither can load at PostConfigInit). Loaded at PostConfigInit
// (see CkFoundation.uplugin) so the /CkPixelArt shader directory mapping and the upscale global shader type exist
// before the engine builds its global shader map. Same shape and same reason as CkIskmRendererVF.
public class CkPixelArtRender : ModuleRules
{
    public CkPixelArtRender(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // ISpatialUpscaler is declared in the renderer's PRIVATE headers, and the engine explicitly provides no
        // backward compatibility for it across versions. This module is therefore recompile-coupled to the pinned
        // fork — budget for it in every engine upgrade.
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"));

        if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 6)
        {
            PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal"));
        }

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
