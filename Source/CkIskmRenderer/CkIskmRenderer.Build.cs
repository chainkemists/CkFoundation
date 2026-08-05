using System.IO;
using UnrealBuildTool;

public class CkIskmRenderer : CkModuleRules
{
    public CkIskmRenderer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // Plan-2: FScene::PrimitiveUpdates + FUpdate*Command live in these private renderer headers
            // (version-fragile — re-verify on engine bumps). Used by the batched cluster proxy's per-frame upload.
            System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Private"),
            System.IO.Path.Combine(GetModuleDirectory("Renderer"), "Internal"),
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
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
            "CkUsf",

            // ---- Plan-2 (batched GPU-skinned instancing) render-thread deps ----
            // RenderCore: FRenderTransform/FRenderBounds + ENQUEUE_RENDER_COMMAND (proxy + AnimCollection upload).
            // RHI:        uniform-buffer creation for the baked pose buffer.
            "RenderCore",
            "RHI",

            // The engine-only PostConfigInit module holding the vertex factory + render resources (so the VF type
            // registers before the engine seals the VF list).
            "CkIskmRendererVF",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            // Renderer: FScene / FScene::PrimitiveUpdates for the batched cluster proxy's per-frame instance upload.
            "Renderer",
        });
    }
}
