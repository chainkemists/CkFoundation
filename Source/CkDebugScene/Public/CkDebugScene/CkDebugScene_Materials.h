#pragma once

#include <CoreMinimal.h>

class UMaterialInterface;

// Runtime-safe access to the three materials shipped with the retained debug-scene backend. A missing package is
// explicitly represented by nullptr; callers must reject it rather than substituting arbitrary engine content.
namespace ck::debug_scene::materials
{
CKDEBUGSCENE_API auto TryGet_Opaque() -> UMaterialInterface*;
CKDEBUGSCENE_API auto TryGet_Translucent() -> UMaterialInterface*;
CKDEBUGSCENE_API auto TryGet_Wireframe() -> UMaterialInterface*;
CKDEBUGSCENE_API auto Is_IsmCompatible(UMaterialInterface* InMaterial) -> bool;
} // namespace ck::debug_scene::materials
