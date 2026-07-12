#pragma once

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

    // Object paths of every material any renderer of this system uses — the corpus orchestrator chains
    // these into material (and from there texture) exports.
    TArray<FString> ReferencedMaterials;

    // Object paths of every static mesh any mesh renderer uses — chained into mesh (OBJ + stats) exports.
    TArray<FString> ReferencedMeshes;
};

// --------------------------------------------------------------------------------------------------------------------
// Exports a UNiagaraSystem's full authoring structure (emitters, their module stacks, spawn config, and renderers)
// to JSON + plain text. Built to deconstruct professional VFX into a readable recipe: the layering (which emitters),
// the composition (which modules in which order), the authored numbers (Rapid Iteration constants AND the override
// pins that carry dynamic inputs / curve data interfaces / attribute links), and how each is drawn (renderers).
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_NiagaraExporter
{
public:
    // InOutputDir: when set, writes <AssetName>.json/.txt into that directory instead of next to the asset.
    static auto ExportNiagaraSystem (UNiagaraSystem* InSystem, const FString& InOutputDir = FString{}) -> FCk_NiagaraExportResult;
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
    // Walks the stage's parameter-map chain backward from the Output node, collecting the ordered module
    // (function-call) nodes AND the override ParameterMapSet nodes between them (which carry the authored
    // dynamic inputs, data interfaces, and attribute links as dotted-path pins). Override nodes are held as
    // plain UEdGraphNode — UNiagaraNodeParameterMapSet is a Private NiagaraEditor header, and only the pin
    // list is needed anyway; detection is by class name.
    static auto DoWalkStack            (UNiagaraScript* InScript, TArray<UNiagaraNodeFunctionCall*>& OutModules, TArray<UEdGraphNode*>& OutOverrideNodes) -> bool;

    // Flattens every override pin of a stage into { ownerCallName -> override description } entries.
    // Pin names are "<OwnerCallName>.<Input>" where the owner is either a stack module OR a dynamic-input
    // node — nested dynamic-input parameters are keyed by the dynamic input's own call name and must be
    // re-associated to their module via each entry's "id" field (DoExpandDynamicInputOverrides).
    static auto DoHarvestOverrides     (const TArray<UEdGraphNode*>& InOverrideNodes) -> TMultiMap<FString, TSharedPtr<FJsonObject>>;

    // Recursively attaches overrides keyed by a dynamic input's call name onto that dynamic input's entry
    // (as a nested "overrides" array) — this is where curve DIs authored on dynamic inputs are recovered.
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
