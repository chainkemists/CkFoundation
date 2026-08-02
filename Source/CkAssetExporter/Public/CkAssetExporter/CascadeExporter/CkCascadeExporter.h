#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UParticleSystem;
class UParticleEmitter;
class UParticleLODLevel;
class UParticleModule;
class UDistributionFloat;
class UDistributionVector;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_CascadeExportResult
{
    bool    Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;

    // Chained by the corpus orchestrator into material (and from there texture) exports.
    TArray<FString> ReferencedMaterials;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_CascadeExporter
{
public:
    // InOutputDir: when set, writes <AssetName>.ckexport/.txt into that directory instead of next to the asset.
    static auto ExportParticleSystem (UParticleSystem* InSystem, const FString& InOutputDir = FString{}, ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonOnly) -> FCk_CascadeExportResult;
    static auto ExportParticleSystems(const TArray<UParticleSystem*>& InSystems) -> TArray<FCk_CascadeExportResult>;

private:
    static auto DoSerializeToJson    (UParticleSystem* InSystem, TSet<FString>& OutMaterials) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeEmitter_Json(const UParticleEmitter* InEmitter, TSet<FString>& OutMaterials) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeModule_Json (UParticleModule* InModule, TSet<FString>& OutMaterials) -> TSharedPtr<FJsonObject>;

    static auto DoSerializeToText    (const TSharedPtr<FJsonObject>& InJson) -> FString;

    static auto DoFormatDistributionFloat (const UDistributionFloat* InDistribution) -> FString;
    static auto DoFormatDistributionVector(const UDistributionVector* InDistribution) -> FString;

    static auto DoResolveOutputPath  (const UParticleSystem* InSystem, const FString& InExtension, const FString& InOutputDir) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
