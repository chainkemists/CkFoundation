#pragma once

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

    // Object paths of every material any emitter uses — the corpus orchestrator chains these into
    // material (and from there texture) exports.
    TArray<FString> ReferencedMaterials;
};

// --------------------------------------------------------------------------------------------------------------------
// Exports a legacy Cascade UParticleSystem to JSON + plain text (LOD 0 only). Modules are dumped generically via
// reflection — every UPROPERTY on every module — with UDistribution properties rasterized to their authored form
// (constant / uniform min-max / curve keys / particle parameter), which is where Cascade keeps the recipe numbers.
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_CascadeExporter
{
public:
    // InOutputDir: when set, writes <AssetName>.json/.txt into that directory instead of next to the asset.
    static auto ExportParticleSystem (UParticleSystem* InSystem, const FString& InOutputDir = FString{}) -> FCk_CascadeExportResult;
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
