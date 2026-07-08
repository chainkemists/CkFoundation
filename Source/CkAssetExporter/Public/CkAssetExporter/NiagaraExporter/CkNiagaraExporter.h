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
struct FVersionedNiagaraEmitterData;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_NiagaraExportResult
{
    bool    Succeeded = false;
    FString JsonFilePath;
    FString TextFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------
// Exports a UNiagaraSystem's full authoring structure (emitters, their module stacks, spawn config, and renderers)
// to JSON + plain text next to the asset. Built to deconstruct professional VFX into a readable recipe: the layering
// (which emitters), the composition (which modules in which order), and how each is drawn (renderers + bindings).
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_NiagaraExporter
{
public:
    static auto ExportNiagaraSystem (UNiagaraSystem* InSystem) -> FCk_NiagaraExportResult;
    static auto ExportNiagaraSystems(const TArray<UNiagaraSystem*>& InSystems) -> TArray<FCk_NiagaraExportResult>;

private:
    // ---- JSON ----
    static auto DoSerializeToJson    (const UNiagaraSystem* InSystem) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeEmitter_Json(const FVersionedNiagaraEmitterData* InData, const FString& InName, bool bInEnabled) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeStack_Json  (UNiagaraScript* InScript, const FString& InStageName) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeModule_Json (const UNiagaraNodeFunctionCall* InModule) -> TSharedPtr<FJsonObject>;
    static auto DoSerializeRenderer_Json(const UNiagaraRendererProperties* InRenderer) -> TSharedPtr<FJsonObject>;

    // ---- Plain text ----
    static auto DoSerializeToText      (const UNiagaraSystem* InSystem) -> FString;

    // ---- Helpers ----
    static auto DoResolveOutputPath    (const UNiagaraSystem* InSystem, const FString& InExtension) -> FString;
    static auto DoGetOrderedModules    (UNiagaraScript* InScript, TArray<UNiagaraNodeFunctionCall*>& OutModules) -> bool;
    static auto DoGetModuleInputs      (const UNiagaraNodeFunctionCall* InModule) -> TArray<TPair<FString, FString>>;
};

// --------------------------------------------------------------------------------------------------------------------
