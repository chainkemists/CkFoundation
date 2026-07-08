#include "CkNiagaraExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterStore.h"
#include "EdGraphSchema_Niagara.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>

// --------------------------------------------------------------------------------------------------------------------
// The four authoring scripts of an emitter, in editor display order. Walking each gives the ordered module stack the
// VFX artist sees in the System Overview — the "recipe".
namespace ck::asset_exporter::niagara
{
    struct FStage { const TCHAR* Name; FNiagaraEmitterScriptProperties FVersionedNiagaraEmitterData::* Member; };

    static auto Get_Stages() -> TArray<FStage>
    {
        return {
            { TEXT("Emitter Spawn"),  &FVersionedNiagaraEmitterData::EmitterSpawnScriptProps },
            { TEXT("Emitter Update"), &FVersionedNiagaraEmitterData::EmitterUpdateScriptProps },
            { TEXT("Particle Spawn"), &FVersionedNiagaraEmitterData::SpawnScriptProps },
            { TEXT("Particle Update"),&FVersionedNiagaraEmitterData::UpdateScriptProps },
        };
    }

    static auto SimTargetToString(ENiagaraSimTarget InTarget) -> FString
    { return InTarget == ENiagaraSimTarget::GPUComputeSim ? TEXT("GPU") : TEXT("CPU"); }

    // Formats a single Rapid Iteration Parameter value (the authored constant a module input pin hides) for the
    // common POD/vector/color types. Returns unset for anything else (data interfaces, UObjects, structs we don't
    // decode) so the caller skips it without ever reading bytes at a non-ParameterData offset.
    static auto FormatStoreValue(
        const FNiagaraParameterStore& InStore,
        const FNiagaraVariableWithOffset& InVar)
        -> TOptional<FString>
    {
        const auto& Type = InVar.GetType();

        if (Type == FNiagaraTypeDefinition::GetFloatDef())
        { return FString::Printf(TEXT("%g"), *reinterpret_cast<const float*>(InStore.GetParameterData(InVar.Offset, Type))); }
        if (Type == FNiagaraTypeDefinition::GetIntDef())
        { return FString::FromInt(*reinterpret_cast<const int32*>(InStore.GetParameterData(InVar.Offset, Type))); }
        if (Type == FNiagaraTypeDefinition::GetBoolDef())
        { return FString(*reinterpret_cast<const int32*>(InStore.GetParameterData(InVar.Offset, Type)) != 0 ? TEXT("true") : TEXT("false")); }
        if (Type == FNiagaraTypeDefinition::GetVec2Def())
        { const auto* V = reinterpret_cast<const FVector2f*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("(%g, %g)"), V->X, V->Y); }
        if (Type == FNiagaraTypeDefinition::GetVec3Def())
        { const auto* V = reinterpret_cast<const FVector3f*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("(%g, %g, %g)"), V->X, V->Y, V->Z); }
        if (Type == FNiagaraTypeDefinition::GetVec4Def())
        { const auto* V = reinterpret_cast<const FVector4f*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("(%g, %g, %g, %g)"), V->X, V->Y, V->Z, V->W); }
        if (Type == FNiagaraTypeDefinition::GetColorDef())
        { const auto* C = reinterpret_cast<const FLinearColor*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("RGBA(%g, %g, %g, %g)"), C->R, C->G, C->B, C->A); }

        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Public API (mirrors the other CkAssetExporter exporters: JSON + text written next to the asset).

auto
    FCk_NiagaraExporter::
    ExportNiagaraSystem(
        UNiagaraSystem* InSystem)
    -> FCk_NiagaraExportResult
{
    auto Result = FCk_NiagaraExportResult{};
    if (ck::Is_NOT_Valid(InSystem))
    {
        Result.ErrorMessage = TEXT("Invalid Niagara System asset");
        return Result;
    }

    Result.AssetName = InSystem->GetName();

    const auto JsonObject = DoSerializeToJson(InSystem);
    auto JsonString = FString{};
    if (JsonObject.IsValid())
    {
        const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
    }

    const auto TextString = DoSerializeToText(InSystem);

    const auto JsonPath = DoResolveOutputPath(InSystem, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InSystem, TEXT(".txt"));
    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    const auto JsonWritten = FFileHelper::SaveStringToFile(JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = FFileHelper::SaveStringToFile(TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    if (NOT JsonWritten || NOT TextWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, Text: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"), TextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_NiagaraExporter::
    ExportNiagaraSystems(
        const TArray<UNiagaraSystem*>& InSystems)
    -> TArray<FCk_NiagaraExportResult>
{
    auto Results = TArray<FCk_NiagaraExportResult>{};
    Results.Reserve(InSystems.Num());
    ck::algo::ForEach(InSystems, [&](UNiagaraSystem* Sys){ Results.Add(ExportNiagaraSystem(Sys)); });
    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// Stack walking

auto
    FCk_NiagaraExporter::
    DoGetOrderedModules(
        UNiagaraScript* InScript,
        TArray<UNiagaraNodeFunctionCall*>& OutModules)
    -> bool
{
    if (ck::Is_NOT_Valid(InScript))
    { return false; }

    auto* Source = Cast<UNiagaraScriptSource>(InScript->GetLatestSource());
    if (Source == nullptr || Source->NodeGraph == nullptr)
    { return false; }

    auto* OutputNode = Source->NodeGraph->FindEquivalentOutputNode(InScript->GetUsage(), InScript->GetUsageId());
    if (OutputNode == nullptr)
    { return false; }

    const auto* Schema = Cast<UEdGraphSchema_Niagara>(Source->NodeGraph->GetSchema());
    if (Schema == nullptr)
    { return false; }

    // Replicate FNiagaraStackGraphUtilities::GetOrderedModuleNodes (which isn't exported) with public symbols: the
    // stack threads a single parameter map Input -> module -> module -> Output; walk that chain backward from the
    // Output node, collecting the function-call (module) nodes in order. (Override Map-Set nodes between modules are
    // simply skipped — they aren't function calls.)
    const UEdGraphNode* Current = OutputNode;
    auto Guard = int32{0};
    while (Current != nullptr && Guard++ < 512)
    {
        const UEdGraphPin* MapIn = nullptr;
        for (const UEdGraphPin* Pin : Current->Pins)
        {
            if (Pin != nullptr && Pin->Direction == EGPD_Input &&
                Schema->PinToTypeDefinition(Pin) == FNiagaraTypeDefinition::GetParameterMapDef())
            { MapIn = Pin; break; }
        }
        if (MapIn == nullptr || MapIn->LinkedTo.Num() == 0)
        { break; }

        UEdGraphNode* Prev = MapIn->LinkedTo[0]->GetOwningNode();
        if (Prev == nullptr)
        { break; }
        if (auto* Func = Cast<UNiagaraNodeFunctionCall>(Prev))
        { OutModules.Insert(Func, 0); }
        Current = Prev;
    }
    return true;
}

auto
    FCk_NiagaraExporter::
    DoGetModuleInputs(
        const UNiagaraNodeFunctionCall* InModule)
    -> TArray<TPair<FString, FString>>
{
    auto Inputs = TArray<TPair<FString, FString>>{};
    if (InModule == nullptr)
    { return Inputs; }

    for (const UEdGraphPin* Pin : InModule->Pins)
    {
        if (Pin == nullptr || Pin->Direction != EGPD_Input)
        { continue; }
        // Skip the parameter-map plumbing pins (they carry the stack, not authored values).
        if (Pin->PinName == NAME_None || Pin->PinType.PinCategory == TEXT("ParameterMap"))
        { continue; }

        const auto Name = Pin->PinName.ToString();
        if (Name.IsEmpty())
        { continue; }

        auto Value = FString{};
        if (Pin->LinkedTo.Num() > 0)
        {
            // A linked pin is driven by an upstream node. If that's a function-call node it's a dynamic input —
            // name it (its title is the DI's shape, e.g. "Float from Curve"). Otherwise just mark it linked.
            const auto* Source = Pin->LinkedTo[0] != nullptr ? Pin->LinkedTo[0]->GetOwningNode() : nullptr;
            if (const auto* DynInput = Cast<UNiagaraNodeFunctionCall>(Source))
            { Value = ck::Format_UE(TEXT("[dynamic input: {}]"), DynInput->GetNodeTitle(ENodeTitleType::ListView).ToString()); }
            else
            { Value = TEXT("<linked>"); }
        }
        else
        { Value = Pin->DefaultValue; }
        Inputs.Add({ Name, Value });
    }
    return Inputs;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain text (the human-readable recipe)

auto
    FCk_NiagaraExporter::
    DoSerializeToText(
        const UNiagaraSystem* InSystem)
    -> FString
{
    auto* System = const_cast<UNiagaraSystem*>(InSystem);
    auto Out = FString{};

    Out += ck::Format_UE(TEXT("NIAGARA SYSTEM: {}\n"), System->GetName());
    Out += TEXT("====================================================================\n\n");

    // ---- System-level user parameters ----
    Out += TEXT("USER PARAMETERS\n");
    for (const auto& Var : System->GetExposedParameters().ReadParameterVariables())
    {
        Out += ck::Format_UE(TEXT("  - {} : {}\n"), Var.GetName().ToString(), Var.GetType().GetName());
    }
    Out += TEXT("\n");

    // ---- Emitters ----
    const auto& Handles = System->GetEmitterHandles();
    Out += ck::Format_UE(TEXT("EMITTERS ({})\n"), FString::FromInt(Handles.Num()));
    Out += TEXT("--------------------------------------------------------------------\n");

    for (const auto& Handle : Handles)
    {
        auto* Data = Handle.GetEmitterData();
        Out += ck::Format_UE(TEXT("\n[EMITTER] {}{}\n"),
            Handle.GetName().ToString(), Handle.GetIsEnabled() ? TEXT("") : TEXT("  (DISABLED)"));

        if (Data == nullptr)
        { Out += TEXT("  <no emitter data>\n"); continue; }

        Out += ck::Format_UE(TEXT("  Sim: {}   LocalSpace: {}\n"),
            ck::asset_exporter::niagara::SimTargetToString(Data->SimTarget),
            Data->bLocalSpace ? TEXT("true") : TEXT("false"));

        // Module stacks (the layering recipe).
        for (const auto& Stage : ck::asset_exporter::niagara::Get_Stages())
        {
            UNiagaraScript* Script = (Data->*(Stage.Member)).Script;
            auto Modules = TArray<UNiagaraNodeFunctionCall*>{};
            if (NOT DoGetOrderedModules(Script, Modules) || Modules.Num() == 0)
            { continue; }

            Out += ck::Format_UE(TEXT("  {} ({} modules):\n"), FString(Stage.Name), FString::FromInt(Modules.Num()));
            auto Index = int32{0};
            for (const auto* Module : Modules)
            {
                if (Module == nullptr) { continue; }
                Out += ck::Format_UE(TEXT("    {}. {}\n"), FString::FromInt(++Index),
                    Module->GetNodeTitle(ENodeTitleType::ListView).ToString());
                for (const auto& Input : DoGetModuleInputs(Module))
                {
                    if (Input.Value.IsEmpty()) { continue; }
                    Out += ck::Format_UE(TEXT("         {} = {}\n"), Input.Key, Input.Value);
                }
            }

            // The authored constant values for this stage's modules (sizes, colors, counts, lifetimes) live in the
            // script's Rapid Iteration Parameter store, keyed Constants.[Emitter].[Module].[Input]. The module graph
            // pins only expose the static-switch selectors, so this is where the real "recipe numbers" are.
            const auto& Rip = Script->RapidIterationParameters;
            auto bWroteValuesHeader = false;
            for (const auto& Var : Rip.ReadParameterVariables())
            {
                const auto Formatted = ck::asset_exporter::niagara::FormatStoreValue(Rip, Var);
                if (NOT Formatted.IsSet()) { continue; }
                if (NOT bWroteValuesHeader)
                { Out += TEXT("    [values]\n"); bWroteValuesHeader = true; }
                auto VarName = Var.GetName().ToString();
                VarName.RemoveFromStart(TEXT("Constants."));
                Out += ck::Format_UE(TEXT("       {} = {}\n"), VarName, Formatted.GetValue());
            }
        }

        // Renderers (how it's drawn).
        const auto& Renderers = Data->GetRenderers();
        if (Renderers.Num() > 0)
        {
            Out += ck::Format_UE(TEXT("  Renderers ({}):\n"), FString::FromInt(Renderers.Num()));
            for (const auto* Renderer : Renderers)
            {
                if (Renderer == nullptr) { continue; }
                Out += ck::Format_UE(TEXT("    - {}"), Renderer->GetClass()->GetName());
                if (const auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
                {
                    Out += ck::Format_UE(TEXT("  Material: {}  Alignment: {}  Facing: {}"),
                        Sprite->Material ? Sprite->Material->GetName() : FString(TEXT("<none>")),
                        StaticEnum<ENiagaraSpriteAlignment>()->GetNameStringByValue((int64)Sprite->Alignment),
                        StaticEnum<ENiagaraSpriteFacingMode>()->GetNameStringByValue((int64)Sprite->FacingMode));
                    if (Sprite->SubImageSize != FVector2D(1.0, 1.0))
                    { Out += FString::Printf(TEXT("  SubUV: %dx%d"), (int32)Sprite->SubImageSize.X, (int32)Sprite->SubImageSize.Y); }
                }
                else if (const auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
                {
                    Out += ck::Format_UE(TEXT("  Material: {}"), Ribbon->Material ? Ribbon->Material->GetName() : FString(TEXT("<none>")));
                }
                else if (const auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
                {
                    // The shaped elements (slashes, spikes, strips) are oriented stretched meshes — name the mesh(es),
                    // non-unit scale, facing, and override materials so the recipe is reproducible.
                    Out += ck::Format_UE(TEXT("  Facing: {}"),
                        StaticEnum<ENiagaraMeshFacingMode>()->GetNameStringByValue((int64)Mesh->FacingMode));
                    if (Mesh->SubImageSize != FVector2D(1.0, 1.0))
                    { Out += FString::Printf(TEXT("  SubUV: %dx%d"), (int32)Mesh->SubImageSize.X, (int32)Mesh->SubImageSize.Y); }
                    for (const auto& M : Mesh->Meshes)
                    {
                        Out += ck::Format_UE(TEXT("\n        mesh: {}"), M.Mesh ? M.Mesh->GetName() : FString(TEXT("<none>")));
                        if (NOT M.Scale.Equals(FVector::OneVector))
                        { Out += FString::Printf(TEXT("  scale: (%g, %g, %g)"), M.Scale.X, M.Scale.Y, M.Scale.Z); }
                    }
                    if (Mesh->bOverrideMaterials)
                    {
                        for (const auto& Ov : Mesh->OverrideMaterials)
                        { Out += ck::Format_UE(TEXT("\n        material: {}"), Ov.ExplicitMat ? Ov.ExplicitMat->GetName() : FString(TEXT("<none>"))); }
                    }
                }
                else if (const auto* Light = Cast<UNiagaraLightRendererProperties>(Renderer))
                {
                    Out += FString::Printf(TEXT("  RadiusScale: %g  AffectsTranslucency: %s"),
                        Light->RadiusScale, Light->bAffectsTranslucency ? TEXT("true") : TEXT("false"));
                }
                Out += TEXT("\n");
            }
        }
    }

    return Out;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON (structured mirror)

auto
    FCk_NiagaraExporter::
    DoSerializeModule_Json(
        const UNiagaraNodeFunctionCall* InModule)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), InModule->GetNodeTitle(ENodeTitleType::ListView).ToString());

    auto InputsArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Input : DoGetModuleInputs(InModule))
    {
        auto In = MakeShared<FJsonObject>();
        In->SetStringField(TEXT("name"), Input.Key);
        In->SetStringField(TEXT("value"), Input.Value);
        InputsArr.Add(MakeShared<FJsonValueObject>(In));
    }
    Obj->SetArrayField(TEXT("inputs"), InputsArr);
    return Obj;
}

auto
    FCk_NiagaraExporter::
    DoSerializeStack_Json(
        UNiagaraScript* InScript,
        const FString& InStageName)
    -> TSharedPtr<FJsonObject>
{
    auto Modules = TArray<UNiagaraNodeFunctionCall*>{};
    if (NOT DoGetOrderedModules(InScript, Modules) || Modules.Num() == 0)
    { return nullptr; }

    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("stage"), InStageName);
    auto Arr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Module : Modules)
    {
        if (Module == nullptr) { continue; }
        Arr.Add(MakeShared<FJsonValueObject>(DoSerializeModule_Json(Module)));
    }
    Obj->SetArrayField(TEXT("modules"), Arr);

    // The authored constant values (sizes, colors, counts, lifetimes) from the script's Rapid Iteration store.
    auto ValuesArr = TArray<TSharedPtr<FJsonValue>>{};
    const auto& Rip = InScript->RapidIterationParameters;
    for (const auto& Var : Rip.ReadParameterVariables())
    {
        const auto Formatted = ck::asset_exporter::niagara::FormatStoreValue(Rip, Var);
        if (NOT Formatted.IsSet()) { continue; }
        auto ValObj = MakeShared<FJsonObject>();
        auto VarName = Var.GetName().ToString();
        VarName.RemoveFromStart(TEXT("Constants."));
        ValObj->SetStringField(TEXT("name"), VarName);
        ValObj->SetStringField(TEXT("value"), Formatted.GetValue());
        ValuesArr.Add(MakeShared<FJsonValueObject>(ValObj));
    }
    Obj->SetArrayField(TEXT("values"), ValuesArr);
    return Obj;
}

auto
    FCk_NiagaraExporter::
    DoSerializeRenderer_Json(
        const UNiagaraRendererProperties* InRenderer)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("type"), InRenderer->GetClass()->GetName());
    if (const auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(InRenderer))
    {
        Obj->SetStringField(TEXT("material"), Sprite->Material ? Sprite->Material->GetName() : FString(TEXT("<none>")));
        Obj->SetStringField(TEXT("alignment"), StaticEnum<ENiagaraSpriteAlignment>()->GetNameStringByValue((int64)Sprite->Alignment));
        Obj->SetStringField(TEXT("facing"), StaticEnum<ENiagaraSpriteFacingMode>()->GetNameStringByValue((int64)Sprite->FacingMode));
        if (Sprite->SubImageSize != FVector2D(1.0, 1.0))
        { Obj->SetStringField(TEXT("subImage"), FString::Printf(TEXT("%dx%d"), (int32)Sprite->SubImageSize.X, (int32)Sprite->SubImageSize.Y)); }
    }
    else if (const auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(InRenderer))
    {
        Obj->SetStringField(TEXT("material"), Ribbon->Material ? Ribbon->Material->GetName() : FString(TEXT("<none>")));
    }
    else if (const auto* Mesh = Cast<UNiagaraMeshRendererProperties>(InRenderer))
    {
        Obj->SetStringField(TEXT("facingMode"), StaticEnum<ENiagaraMeshFacingMode>()->GetNameStringByValue((int64)Mesh->FacingMode));
        auto MeshesArr = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& M : Mesh->Meshes)
        {
            auto MeshObj = MakeShared<FJsonObject>();
            MeshObj->SetStringField(TEXT("mesh"), M.Mesh ? M.Mesh->GetName() : FString(TEXT("<none>")));
            MeshObj->SetStringField(TEXT("scale"), FString::Printf(TEXT("(%g, %g, %g)"), M.Scale.X, M.Scale.Y, M.Scale.Z));
            MeshesArr.Add(MakeShared<FJsonValueObject>(MeshObj));
        }
        Obj->SetArrayField(TEXT("meshes"), MeshesArr);
        if (Mesh->bOverrideMaterials)
        {
            auto MatsArr = TArray<TSharedPtr<FJsonValue>>{};
            for (const auto& Ov : Mesh->OverrideMaterials)
            { MatsArr.Add(MakeShared<FJsonValueString>(Ov.ExplicitMat ? Ov.ExplicitMat->GetName() : FString(TEXT("<none>")))); }
            Obj->SetArrayField(TEXT("overrideMaterials"), MatsArr);
        }
        if (Mesh->SubImageSize != FVector2D(1.0, 1.0))
        { Obj->SetStringField(TEXT("subImage"), FString::Printf(TEXT("%dx%d"), (int32)Mesh->SubImageSize.X, (int32)Mesh->SubImageSize.Y)); }
    }
    else if (const auto* Light = Cast<UNiagaraLightRendererProperties>(InRenderer))
    {
        Obj->SetNumberField(TEXT("radiusScale"), Light->RadiusScale);
        Obj->SetBoolField(TEXT("affectsTranslucency"), Light->bAffectsTranslucency != 0);
    }
    return Obj;
}

auto
    FCk_NiagaraExporter::
    DoSerializeEmitter_Json(
        const FVersionedNiagaraEmitterData* InData,
        const FString& InName,
        bool bInEnabled)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), InName);
    Obj->SetBoolField(TEXT("enabled"), bInEnabled);
    if (InData == nullptr)
    { return Obj; }

    Obj->SetStringField(TEXT("sim"), ck::asset_exporter::niagara::SimTargetToString(InData->SimTarget));
    Obj->SetBoolField(TEXT("localSpace"), InData->bLocalSpace);

    auto Stacks = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Stage : ck::asset_exporter::niagara::Get_Stages())
    {
        UNiagaraScript* Script = (InData->*(Stage.Member)).Script;
        if (auto Stack = DoSerializeStack_Json(Script, Stage.Name))
        { Stacks.Add(MakeShared<FJsonValueObject>(Stack)); }
    }
    Obj->SetArrayField(TEXT("stacks"), Stacks);

    auto Renderers = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Renderer : InData->GetRenderers())
    {
        if (Renderer == nullptr) { continue; }
        Renderers.Add(MakeShared<FJsonValueObject>(DoSerializeRenderer_Json(Renderer)));
    }
    Obj->SetArrayField(TEXT("renderers"), Renderers);
    return Obj;
}

auto
    FCk_NiagaraExporter::
    DoSerializeToJson(
        const UNiagaraSystem* InSystem)
    -> TSharedPtr<FJsonObject>
{
    auto* System = const_cast<UNiagaraSystem*>(InSystem);
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("system"), System->GetName());

    auto Params = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Var : System->GetExposedParameters().ReadParameterVariables())
    {
        auto P = MakeShared<FJsonObject>();
        P->SetStringField(TEXT("name"), Var.GetName().ToString());
        P->SetStringField(TEXT("type"), Var.GetType().GetName());
        Params.Add(MakeShared<FJsonValueObject>(P));
    }
    Root->SetArrayField(TEXT("userParameters"), Params);

    auto Emitters = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Handle : System->GetEmitterHandles())
    {
        Emitters.Add(MakeShared<FJsonValueObject>(
            DoSerializeEmitter_Json(Handle.GetEmitterData(), Handle.GetName().ToString(), Handle.GetIsEnabled())));
    }
    Root->SetArrayField(TEXT("emitters"), Emitters);
    return Root;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_NiagaraExporter::
    DoResolveOutputPath(
        const UNiagaraSystem* InSystem,
        const FString& InExtension)
    -> FString
{
    const auto PackageName = InSystem->GetOutermost()->GetName();
    auto FilePath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, FilePath, InExtension))
    { return FString{}; }
    return FilePath;
}
