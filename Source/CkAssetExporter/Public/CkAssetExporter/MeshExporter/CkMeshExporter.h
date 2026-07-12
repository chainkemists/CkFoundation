#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UStaticMesh;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_MeshExportResult
{
    bool    Succeeded = false;
    FString JsonFilePath;
    FString ObjFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------
// Exports a UStaticMesh's LOD-0 render geometry to a Wavefront OBJ (positions, normals, UV0, faces grouped per
// material section) plus a stats JSON (bounds, counts, UV ranges, section->material map). Built so VFX carrier
// meshes (slash sweeps, cones, shells, trails) can be analyzed and procedurally re-authored from their recipe.
// UVs are written in UE convention (V down) — noted in the OBJ header so downstream parsers don't double-flip.
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_MeshExporter
{
public:
    // InOutputDir: when set, writes <AssetName>.obj/.json into that directory instead of next to the asset.
    static auto ExportStaticMesh(UStaticMesh* InMesh, const FString& InOutputDir = FString{}) -> FCk_MeshExportResult;
};

// --------------------------------------------------------------------------------------------------------------------
