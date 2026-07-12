#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_VfxCorpusSummary
{
    bool    Succeeded = false;
    FString ErrorMessage;
    FString CorpusRoot;
    FString IndexPath;

    int32 Systems = 0;
    int32 Niagara = 0;
    int32 Cascade = 0;
    int32 FailedSystems = 0;
    int32 Materials = 0;
    int32 FailedMaterials = 0;
    int32 Textures = 0;
    int32 FailedTextures = 0;
    int32 Meshes = 0;
    int32 FailedMeshes = 0;
};

// --------------------------------------------------------------------------------------------------------------------
// Batch-exports every Niagara + Cascade particle system under the given content roots into a corpus layout:
//
//   <CorpusRoot>/index.json                    manifest (every system/material/texture/mesh, failures included)
//   <CorpusRoot>/systems/<Pack>/<Name>.json|txt
//   <CorpusRoot>/materials/<Pack>/<Name>.json
//   <CorpusRoot>/textures/<Pack>/<Name>.png|json
//   <CorpusRoot>/meshes/<Pack>/<Name>.obj|json  (VFX carrier meshes referenced by mesh renderers)
//
// Discovery is by asset class via the asset registry (pack naming conventions are unreliable). Referenced
// materials, textures, and meshes are exported once (deduplicated across systems). Per-asset failures are
// recorded in the index, never silently skipped. The corpus root is wiped before a run — it is regenerable output.
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_VfxCorpusExporter
{
public:
    static auto ExportCorpus(const TArray<FString>& InContentRoots, const FString& InCorpusRoot) -> FCk_VfxCorpusSummary;

    // <ProjectSaved>/CkVfxCorpus
    static auto Get_DefaultCorpusRoot() -> FString;

private:
    // "/Game/<Pack>/..." -> "<Pack>"; "/<PluginOrEngine>/..." -> "<PluginOrEngine>" (index grouping only)
    static auto DoGet_PackName(const FString& InPackagePath) -> FString;

    // Output directory subpath mirroring the FULL package folder chain — "/Game/VFX/Fire/P_Sparks" ->
    // "VFX/Fire" — so same-named assets in different folders can never overwrite each other's exports.
    static auto DoGet_OutputSubDir(const FString& InPackageName) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
