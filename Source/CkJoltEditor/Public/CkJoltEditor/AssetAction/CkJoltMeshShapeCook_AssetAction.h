#pragma once

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    /// Adds "Cook Jolt Mesh Shape" to the Content Browser's asset context menu for static meshes.
    /// The entry stays VISIBLE but disabled for meshes the pre-bake does not cover, with a tooltip
    /// naming the reason — a hidden entry teaches nobody why the option is missing.
    void RegisterMeshShapeCookContextMenu();
}

// --------------------------------------------------------------------------------------------------------------------
