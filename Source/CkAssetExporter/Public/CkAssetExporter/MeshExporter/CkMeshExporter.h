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

class CKASSETEXPORTER_API FCk_MeshExporter
{
public:
    // InOutputDir: when set, writes <AssetName>.obj/.json into that directory instead of next to the asset.
    static auto ExportStaticMesh(UStaticMesh* InMesh, const FString& InOutputDir = FString{}) -> FCk_MeshExportResult;
};

// --------------------------------------------------------------------------------------------------------------------
