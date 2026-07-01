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
            // RenderCore: FRenderTransform/FRenderBounds + ENQUEUE_RENDER_COMMAND (proxy + AnimCollection upload).
            // RHI:        uniform-buffer creation for the baked pose buffer.
            "RenderCore",
            "RHI",

            // The engine-only PostConfigInit module holding the vertex factory + render resources (so the VF type
            // registers before the engine seals the VF list). Phase 3 will add Renderer/Private here for PrimitiveUpdates.
            "CkIskmRendererVF",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
