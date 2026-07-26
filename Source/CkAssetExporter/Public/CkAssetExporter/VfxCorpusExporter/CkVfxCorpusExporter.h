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

class CKASSETEXPORTER_API FCk_VfxCorpusExporter
{
public:
    // InCorpusRoot is WIPED before the run — the corpus is regenerable output.
    static auto ExportCorpus(const TArray<FString>& InContentRoots, const FString& InCorpusRoot) -> FCk_VfxCorpusSummary;

    // <ProjectSaved>/CkVfxCorpus
    static auto Get_DefaultCorpusRoot() -> FString;

private:
    // "/Game/<Pack>/..." -> "<Pack>"; "/<PluginOrEngine>/..." -> "<PluginOrEngine>" (index grouping only)
    static auto DoGet_PackName(const FString& InPackagePath) -> FString;

    // Mirrors the FULL package folder chain ("/Game/VFX/Fire/P_Sparks" -> "VFX/Fire") so same-named assets in
    // different folders cannot overwrite each other's exports.
    static auto DoGet_OutputSubDir(const FString& InPackageName) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
