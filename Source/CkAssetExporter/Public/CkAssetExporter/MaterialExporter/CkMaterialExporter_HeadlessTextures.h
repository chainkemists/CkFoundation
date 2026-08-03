#pragma once

#include <Containers/Array.h>
#include <Containers/UnrealString.h>

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInterface;
class UTexture;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_MaterialHeadlessTextures_Result
{
    bool Supported = false;
    FString UnsupportedReason;
    TArray<UTexture*> Textures;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Enumerates every texture a material uses WITHOUT compiled FMaterialResources, so material exports
 * work from render-incapable processes (commandlet/nullrhi) where UMaterialInterface::GetUsedTextures
 * silently returns nothing.
 *
 * Emulates the HLSL translator's traversal so the result matches GetUsedTextures byte-for-byte:
 * properties in translator compile order (Normal first), depth-first through the expression graph,
 * recursing material function calls, taking only the live branch of static switches (resolved through
 * the instance chain), resolving texture parameter values through the instance chain, and bucketing
 * by texture parameter type (2D before Cube before Array...) the way the uniform-expression set does.
 *
 * Returns Supported = false for constructs the walk does not model (material-attributes mode,
 * material layers, quality/feature-level switches, custom outputs...) — callers fall back to
 * refusing the export, exactly as before this walk existed.
 */
class CKASSETEXPORTER_API FCk_MaterialExporter_HeadlessTextures
{
public:
    static auto EnumerateUsedTextures(UMaterialInterface* InMaterial) -> FCk_MaterialHeadlessTextures_Result;
};

// --------------------------------------------------------------------------------------------------------------------
