using System.IO;
using UnrealBuildTool;

public class CkIskmRenderer : CkModuleRules
{
    public CkIskmRenderer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            "AnimGraphRuntime",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",
            "CkGraphics",

            "CkProvider",
            "CkRecord",
            "CkSettings",

            "CkAnimation",
            "CkPhysics",

            // ---- Plan-2 (batched GPU-skinned instancing) render-thread deps ----
            // RenderCore: FRenderResource / shader-dir mapping (ShaderCore.h).
            // RHI:        GPU buffers + SRVs for the baked bone-matrix pose buffer.
            "RenderCore",
            "RHI",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            // Projects: IPluginManager, used by StartupModule to resolve the shader dir.
            "Projects",
        });
    }
}
