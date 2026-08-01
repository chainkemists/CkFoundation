#pragma once

#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UNiagaraSystem;
class UNiagaraEmitter;
class UNiagaraScript;
class UNiagaraNodeFunctionCall;
class UNiagaraNodeOutput;
class UNiagaraRendererProperties;
class UNiagaraDataInterfaceCurveBase;
class UEdGraphNode;
class UEdGraphPin;
struct FVersionedNiagaraEmitterData;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_NiagaraExportResult
{
    bool    Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;

    TArray<FString> ReferencedMaterials;
    TArray<FString> ReferencedMeshes;
};

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_NiagaraExporter
{
public:
    // InOutputDir: when set, writes <AssetName>.ckexport/.txt into that directory instead of next to the asset.
    static auto ExportNiagaraSystem (UNiagaraSystem* InSystem, const FString& InOutputDir = FString{}, ECk_AssetExporter_SidecarFormats InFormats = ECk_AssetExporter_SidecarFormats::JsonAndText) -> FCk_NiagaraExportResult;
    static auto ExportNiagaraSystems(const TArray<UNiagaraSystem*>& InSystems) -> TArray<FCk_NiagaraExportResult>;

private:
    // ---- JSON ----
    static auto DoSerializeToJson      (const UNiagaraSystem* InSystem) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeEmitter_Json(const FVersionedNiagaraEmitterData* InData, const FString& InName, bool bInEnabled) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeStack_Json  (UNiagaraScript* InScript, const FString& InStageName) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeModule_Json (const UNiagaraNodeFunctionCall* InModule, const TArray<TSharedPtr<FJsonObject>>& InOverrides) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeRenderer_Json(const UNiagaraRendererProperties* InRenderer) -> TSharedPtr<FJsonObject>;

    // ---- Plain text ----
    static auto DoSerializeToText      (const UNiagaraSystem* InSystem) -> FString;

    // ---- Stack walking ----
    // Override nodes are plain UEdGraphNode because UNiagaraNodeParameterMapSet lives in a Private NiagaraEditor
    // header; detection is by class name.
    static auto DoWalkStack            (UNiagaraScript* InScript, TArray<UNiagaraNodeFunctionCall*>& OutModules, TArray<UEdGraphNode*>& OutOverrideNodes) -> bool;

    // Keyed by owner call name, taken from the "<OwnerCallName>.<Input>" pin name — the owner is either a stack
    // module OR a dynamic-input node, and the latter must be re-associated via DoExpandDynamicInputOverrides.
    static auto DoHarvestOverrides     (const TArray<UEdGraphNode*>& InOverrideNodes) -> TMultiMap<FString, TSharedPtr<FJsonObject>>;

    // Consumed keys accumulate so the caller can surface any override nobody claimed (never drop silently).
    static auto DoExpandDynamicInputOverrides(const TMultiMap<FString, TSharedPtr<FJsonObject>>& InOverrides, const TArray<TSharedPtr<FJsonObject>>& InEntries, TSet<FString>& InOutConsumedKeys) -> void;
    static auto DoResolveOverridePin   (const UEdGraphPin* InPin, const FString& InInputPath) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeCurveChannels(UNiagaraDataInterfaceCurveBase* InCurveDI) -> TArray<TSharedPtr<FJsonValue>>;
    static auto DoFormatOverride_Text  (const TSharedPtr<FJsonObject>& InOverride) -> FString;

    static auto DoGetModuleInputs      (const UNiagaraNodeFunctionCall* InModule) -> TArray<TPair<FString, FString>>;
    static auto DoCollectMaterials     (const UNiagaraSystem* InSystem) -> TArray<FString>;
    static auto DoCollectMeshes        (const UNiagaraSystem* InSystem) -> TArray<FString>;

    // ---- Helpers ----
    static auto DoResolveOutputPath    (const UNiagaraSystem* InSystem, const FString& InExtension, const FString& InOutputDir) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
