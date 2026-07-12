#pragma once

#include <Dom/JsonObject.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInterface;
class UMaterial;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_MaterialExportResult
{
    bool    Succeeded = false;
    FString JsonFilePath;
    FString ErrorMessage;
    FString AssetName;

    // Object paths of every texture this material (or its instance chain) references — the corpus
    // orchestrator chains these into texture exports.
    TArray<FString> ReferencedTextures;
};

// --------------------------------------------------------------------------------------------------------------------
// Exports a material recipe: base properties (domain, blend mode, shading model, two-sided, connected outputs),
// effective parameter values through the instance chain, and an expression histogram of the base material graph —
// the "material tricks" (DepthFade, Fresnel, Panner, DynamicParameter, SubUV...) a faithful recreation needs.
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_MaterialExporter
{
public:
    static auto ExportMaterial(UMaterialInterface* InMaterial, const FString& InOutputDir) -> FCk_MaterialExportResult;

private:
    static auto DoSerializeToJson(UMaterialInterface* InMaterial, TSet<FString>& OutTextures) -> TSharedPtr<FJsonObject>;
};

// --------------------------------------------------------------------------------------------------------------------
