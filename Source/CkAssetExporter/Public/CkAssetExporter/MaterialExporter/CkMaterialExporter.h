#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInterface;
class UMaterial;
class UTexture;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_MaterialExportResult
{
    bool    Succeeded = false;
    FString JsonFilePath;
    FString ErrorMessage;
    FString AssetName;

    // Chained by the corpus orchestrator into texture exports.
    TArray<FString> ReferencedTextures;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_MaterialExporter
{
public:
    // Set InOutputDir for VFX-corpus mode; empty writes the sibling <Asset>.ckexport next to the source .uasset.
    static auto ExportMaterial (UMaterialInterface* InMaterial, const FString& InOutputDir = FString{}) -> FCk_MaterialExportResult;
    static auto ExportMaterials(const TArray<UMaterialInterface*>& InMaterials) -> TArray<FCk_MaterialExportResult>;

private:
    static auto DoSerializeToJson(UMaterialInterface* InMaterial, const TArray<UTexture*>& InUsedTextures, TSet<FString>& OutTextures) -> TSharedPtr<FJsonObject>;
};

// --------------------------------------------------------------------------------------------------------------------
