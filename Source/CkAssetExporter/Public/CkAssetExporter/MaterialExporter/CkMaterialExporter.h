#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInterface;
class UMaterial;
class UMaterialFunctionInterface;
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

    // UMaterialFunction is NOT a UMaterialInterface — it has no parameters, no blend mode and no material
    // output pins, only a graph whose FunctionInput/FunctionOutput nodes are its signature. It therefore gets
    // its own entry point rather than a branch inside ExportMaterial. Same "graph" shape, so a consumer that
    // reads a material's graph reads a function's unchanged; this is what makes a MaterialFunctionCall in some
    // other material's graph followable instead of a dead end.
    static auto ExportMaterialFunction (UMaterialFunctionInterface* InFunction, const FString& InOutputDir = FString{}) -> FCk_MaterialExportResult;
    static auto ExportMaterialFunctions(const TArray<UMaterialFunctionInterface*>& InFunctions) -> TArray<FCk_MaterialExportResult>;

private:
    static auto DoSerializeToJson(UMaterialInterface* InMaterial, const TArray<UTexture*>& InUsedTextures, TSet<FString>& OutTextures) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeFunctionToJson(UMaterialFunctionInterface* InFunction) -> TSharedPtr<FJsonObject>;
};

// --------------------------------------------------------------------------------------------------------------------
