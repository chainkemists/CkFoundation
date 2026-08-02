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
#include "NiagaraNodeInput.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceCurveBase.h"
#include "NiagaraTypes.h"
#include "NiagaraParameterStore.h"
#include "EdGraphSchema_Niagara.h"

#include "Curves/RichCurve.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::niagara
{
    struct FStage { const TCHAR* Name; FNiagaraEmitterScriptProperties FVersionedNiagaraEmitterData::* Member; };

    static auto Get_Stages_InEditorDisplayOrder() -> TArray<FStage>
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

    static auto InterpModeToLetter(ERichCurveInterpMode InMode) -> FString
    {
        switch (InMode)
        {
            case RCIM_Linear:   return TEXT("L");
            case RCIM_Constant: return TEXT("S");
            case RCIM_Cubic:    return TEXT("C");
            default:            return TEXT("N");
        }
    }

    // Unset for anything not decoded here (data interfaces, UObjects, other structs) — the caller must skip those
    // rather than read bytes at a non-ParameterData offset.
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
        if (Type == FNiagaraTypeDefinition::GetVec3Def() || Type == FNiagaraTypeDefinition::GetPositionDef())
        { const auto* V = reinterpret_cast<const FVector3f*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("(%g, %g, %g)"), V->X, V->Y, V->Z); }
        if (Type == FNiagaraTypeDefinition::GetVec4Def())
        { const auto* V = reinterpret_cast<const FVector4f*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("(%g, %g, %g, %g)"), V->X, V->Y, V->Z, V->W); }
        if (Type == FNiagaraTypeDefinition::GetQuatDef())
        { const auto* Q = reinterpret_cast<const FQuat4f*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("quat(%g, %g, %g, %g)"), Q->X, Q->Y, Q->Z, Q->W); }
        if (Type == FNiagaraTypeDefinition::GetColorDef())
        { const auto* C = reinterpret_cast<const FLinearColor*>(InStore.GetParameterData(InVar.Offset, Type)); return FString::Printf(TEXT("RGBA(%g, %g, %g, %g)"), C->R, C->G, C->B, C->A); }

        return {};
    }

    static auto FormatPinDefault(const UEdGraphPin* InPin) -> FString
    {
        if (InPin->DefaultObject != nullptr)
        { return InPin->DefaultObject->GetPathName(); }

        auto Value = InPin->DefaultValue;
        const auto* Enum = Cast<UEnum>(InPin->PinType.PinSubCategoryObject.Get());
        if (Enum == nullptr || Value.IsEmpty())
        { return Value; }

        const auto Index = Enum->GetIndexByNameString(Value);
        if (Index == INDEX_NONE)
        { return Value; }

        const auto Display = Enum->GetDisplayNameTextByIndex(Index).ToString();
        if (Display.IsEmpty() || Display == Value)
        { return Value; }

        return ck::Format_UE(TEXT("{} ({})"), Value, Display);
    }

    static auto IsParameterMapPin(const UEdGraphPin* InPin) -> bool
    {
        if (InPin == nullptr || InPin->GetOwningNode() == nullptr)
        { return false; }
        const auto* Schema = Cast<UEdGraphSchema_Niagara>(InPin->GetOwningNode()->GetSchema());
        if (Schema == nullptr)
        { return false; }
        return Schema->PinToTypeDefinition(InPin) == FNiagaraTypeDefinition::GetParameterMapDef();
    }

    // UNiagaraNodeInput is UCLASS(MinimalAPI): its GetDataInterface()/GetObjectAsset() accessors are unexported and
    // will not link from this module, so the UPROPERTYs are read by reflection instead.
    static auto Get_NodeObjectProperty(const UNiagaraNodeInput* InNode, const TCHAR* InPropertyName) -> UObject*
    {
        const auto* Property = FindFProperty<FObjectProperty>(UNiagaraNodeInput::StaticClass(), InPropertyName);
        return Property != nullptr ? Property->GetObjectPropertyValue_InContainer(InNode) : nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_NiagaraExporter::
    ExportNiagaraSystem(
        UNiagaraSystem* InSystem,
        const FString& InOutputDir,
        ECk_AssetExporter_SidecarFormats InFormats)
    -> FCk_NiagaraExportResult
{
    auto Result = FCk_NiagaraExportResult{};
    if (ck::Is_NOT_Valid(InSystem))
    {
        Result.ErrorMessage = TEXT("Invalid Niagara System asset");
        return Result;
    }

    Result.AssetName = InSystem->GetName();
    Result.ReferencedMaterials = DoCollectMaterials(InSystem);
    Result.ReferencedMeshes = DoCollectMeshes(InSystem);

    const auto JsonObject = DoSerializeToJson(InSystem);
    auto JsonString = FString{};
    if (JsonObject.IsValid())
    {
        const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);
    }

    const auto WriteText = ck::asset_exporter::ShouldWrite_SummaryText(InFormats);

    const auto TextString = WriteText ? DoSerializeToText(InSystem) : FString{};

    // Corpus mode writes outside Content/, where the auto-reimport monitor never looks and nothing is committed —
    // plain .json is friendlier for those consumers. Only the sibling written next to a .uasset needs to be invisible.
    const auto StructuredExtension = InOutputDir.IsEmpty() ? ck::asset_exporter::extension::Sidecar : FString{TEXT(".json")};

    const auto JsonPath = DoResolveOutputPath(InSystem, StructuredExtension, InOutputDir);
    const auto TextPath = WriteText ? DoResolveOutputPath(InSystem, ck::asset_exporter::extension::SummaryText, InOutputDir) : FString{};
    if (JsonPath.IsEmpty() || (WriteText && TextPath.IsEmpty()))
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    if (NOT InOutputDir.IsEmpty())
    {
        constexpr auto CreateTree = true;
        IFileManager::Get().MakeDirectory(*InOutputDir, CreateTree);
    }

    const auto JsonWritten = FFileHelper::SaveStringToFile(JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = NOT WriteText || FFileHelper::SaveStringToFile(TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
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

auto
    FCk_NiagaraExporter::
    DoWalkStack(
        UNiagaraScript* InScript,
        TArray<UNiagaraNodeFunctionCall*>& OutModules,
        TArray<UEdGraphNode*>& OutOverrideNodes)
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

    // FNiagaraStackGraphUtilities::GetOrderedModuleNodes is unexported; this walks the same single parameter-map
    // chain (Input -> module -> module -> Output) backward using public symbols only.
    constexpr auto MaxChainLength = 512;
    const UEdGraphNode* Current = OutputNode;
    auto Guard = int32{0};
    while (Current != nullptr && Guard++ < MaxChainLength)
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
        else if (Prev->GetClass()->GetName() == TEXT("NiagaraNodeParameterMapSet"))
        { OutOverrideNodes.Add(Prev); }
        Current = Prev;
    }
    return true;
}

auto
    FCk_NiagaraExporter::
    DoHarvestOverrides(
        const TArray<UEdGraphNode*>& InOverrideNodes)
    -> TMultiMap<FString, TSharedPtr<FJsonObject>>
{
    auto Overrides = TMultiMap<FString, TSharedPtr<FJsonObject>>{};

    for (const auto* Node : InOverrideNodes)
    {
        if (Node == nullptr)
        { continue; }

        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin == nullptr || Pin->Direction != EGPD_Input || Pin->PinName.IsNone())
            { continue; }
            if (ck::asset_exporter::niagara::IsParameterMapPin(Pin))
            { continue; }

            const auto PinName = Pin->PinName.ToString();
            auto ModuleName = FString{};
            auto InputPath = FString{};
            if (NOT PinName.Split(TEXT("."), &ModuleName, &InputPath))
            { continue; }

            Overrides.Add(ModuleName, DoResolveOverridePin(Pin, InputPath));
        }
    }
    return Overrides;
}

auto
    FCk_NiagaraExporter::
    DoResolveOverridePin(
        const UEdGraphPin* InPin,
        const FString& InInputPath)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("path"), InInputPath);

    if (InPin->LinkedTo.Num() == 0)
    {
        Obj->SetStringField(TEXT("kind"), TEXT("value"));
        Obj->SetStringField(TEXT("value"), ck::asset_exporter::niagara::FormatPinDefault(InPin));
        return Obj;
    }

    const auto* SrcPin = InPin->LinkedTo[0];
    auto* SrcNode = SrcPin != nullptr ? SrcPin->GetOwningNode() : nullptr;

    if (const auto* InputNode = Cast<UNiagaraNodeInput>(SrcNode))
    {
        if (auto* DataInterface = Cast<UNiagaraDataInterface>(ck::asset_exporter::niagara::Get_NodeObjectProperty(InputNode, TEXT("DataInterface"))))
        {
            if (auto* CurveDI = Cast<UNiagaraDataInterfaceCurveBase>(DataInterface))
            {
                Obj->SetStringField(TEXT("kind"), TEXT("curve"));
                Obj->SetStringField(TEXT("diClass"), DataInterface->GetClass()->GetName());
                Obj->SetArrayField(TEXT("channels"), DoSerializeCurveChannels(CurveDI));
            }
            else
            {
                Obj->SetStringField(TEXT("kind"), TEXT("dataInterface"));
                Obj->SetStringField(TEXT("value"), DataInterface->GetClass()->GetName());
            }
        }
        else if (auto* ObjectAsset = ck::asset_exporter::niagara::Get_NodeObjectProperty(InputNode, TEXT("ObjectAsset")); ObjectAsset != nullptr)
        {
            Obj->SetStringField(TEXT("kind"), TEXT("asset"));
            Obj->SetStringField(TEXT("value"), ObjectAsset->GetPathName());
        }
        else
        {
            Obj->SetStringField(TEXT("kind"), TEXT("input"));
            Obj->SetStringField(TEXT("value"), InputNode->Input.GetName().ToString());
        }
        return Obj;
    }

    if (const auto* DynInput = Cast<UNiagaraNodeFunctionCall>(SrcNode))
    {
        Obj->SetStringField(TEXT("kind"), TEXT("dynamicInput"));
        Obj->SetStringField(TEXT("value"), DynInput->GetNodeTitle(ENodeTitleType::ListView).ToString());
        Obj->SetStringField(TEXT("id"), DynInput->GetFunctionName());
        return Obj;
    }

    // Anything else is a linked parameter read (Map Get pin etc.) — the source pin name IS the parameter.
    Obj->SetStringField(TEXT("kind"), TEXT("linked"));
    Obj->SetStringField(TEXT("value"), SrcPin != nullptr ? SrcPin->PinName.ToString() : FString{TEXT("<unknown>")});
    return Obj;
}

auto
    FCk_NiagaraExporter::
    DoExpandDynamicInputOverrides(
        const TMultiMap<FString, TSharedPtr<FJsonObject>>& InOverrides,
        const TArray<TSharedPtr<FJsonObject>>& InEntries,
        TSet<FString>& InOutConsumedKeys)
    -> void
{
    for (const auto& Entry : InEntries)
    {
        if (NOT Entry.IsValid() || Entry->GetStringField(TEXT("kind")) != TEXT("dynamicInput"))
        { continue; }

        auto DynId = FString{};
        if (NOT Entry->TryGetStringField(TEXT("id"), DynId) || DynId.IsEmpty() || InOutConsumedKeys.Contains(DynId))
        { continue; }
        InOutConsumedKeys.Add(DynId);

        auto Nested = TArray<TSharedPtr<FJsonObject>>{};
        constexpr auto MaintainOrder = true;
        InOverrides.MultiFind(DynId, Nested, MaintainOrder);
        if (Nested.Num() == 0)
        { continue; }

        DoExpandDynamicInputOverrides(InOverrides, Nested, InOutConsumedKeys);

        auto NestedArr = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& NestedEntry : Nested)
        { NestedArr.Add(MakeShared<FJsonValueObject>(NestedEntry)); }
        Entry->SetArrayField(TEXT("overrides"), NestedArr);
    }
}

auto
    FCk_NiagaraExporter::
    DoSerializeCurveChannels(
        UNiagaraDataInterfaceCurveBase* InCurveDI)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Channels = TArray<TSharedPtr<FJsonValue>>{};

    auto CurveData = TArray<UNiagaraDataInterfaceCurveBase::FCurveData>{};
    InCurveDI->GetCurveData(CurveData);

    for (const auto& Data : CurveData)
    {
        if (Data.Curve == nullptr)
        { continue; }

        auto Channel = MakeShared<FJsonObject>();
        Channel->SetStringField(TEXT("name"), Data.Name.ToString());

        auto Keys = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& Key : Data.Curve->Keys)
        {
            auto KeyObj = MakeShared<FJsonObject>();
            KeyObj->SetNumberField(TEXT("t"), Key.Time);
            KeyObj->SetNumberField(TEXT("v"), Key.Value);
            KeyObj->SetStringField(TEXT("i"), ck::asset_exporter::niagara::InterpModeToLetter(Key.InterpMode));
            Keys.Add(MakeShared<FJsonValueObject>(KeyObj));
        }
        Channel->SetArrayField(TEXT("keys"), Keys);
        Channels.Add(MakeShared<FJsonValueObject>(Channel));
    }
    return Channels;
}

auto
    FCk_NiagaraExporter::
    DoFormatOverride_Text(
        const TSharedPtr<FJsonObject>& InOverride)
    -> FString
{
    const auto Path = InOverride->GetStringField(TEXT("path"));
    const auto Kind = InOverride->GetStringField(TEXT("kind"));

    if (Kind == TEXT("curve"))
    {
        auto ChannelsText = TArray<FString>{};
        const auto* Channels = static_cast<const TArray<TSharedPtr<FJsonValue>>*>(nullptr);
        if (InOverride->TryGetArrayField(TEXT("channels"), Channels))
        {
            for (const auto& ChannelValue : *Channels)
            {
                const auto& Channel = ChannelValue->AsObject();
                auto KeysText = TArray<FString>{};
                for (const auto& KeyValue : Channel->GetArrayField(TEXT("keys")))
                {
                    const auto& Key = KeyValue->AsObject();
                    KeysText.Add(FString::Printf(TEXT("(%g, %g)%s"),
                        Key->GetNumberField(TEXT("t")), Key->GetNumberField(TEXT("v")), *Key->GetStringField(TEXT("i"))));
                }
                ChannelsText.Add(ck::Format_UE(TEXT("{}: {}"),
                    Channel->GetStringField(TEXT("name")), FString::Join(KeysText, TEXT(" "))));
            }
        }
        auto DiClass = InOverride->GetStringField(TEXT("diClass"));
        DiClass.RemoveFromStart(TEXT("NiagaraDataInterface"));
        return ck::Format_UE(TEXT("{} = curve<{}> {}"), Path, DiClass, FString::Join(ChannelsText, TEXT(" | ")));
    }

    const auto Value = InOverride->GetStringField(TEXT("value"));
    if (Kind == TEXT("value"))
    { return ck::Format_UE(TEXT("{} = {}"), Path, Value); }
    if (Kind == TEXT("dynamicInput"))
    {
        auto Text = ck::Format_UE(TEXT("{} = dyn:{}"), Path, Value);
        const auto* Nested = static_cast<const TArray<TSharedPtr<FJsonValue>>*>(nullptr);
        if (InOverride->TryGetArrayField(TEXT("overrides"), Nested))
        {
            auto NestedText = TArray<FString>{};
            for (const auto& NestedValue : *Nested)
            { NestedText.Add(DoFormatOverride_Text(NestedValue->AsObject())); }
            Text += TEXT(" { ") + FString::Join(NestedText, TEXT("; ")) + TEXT(" }");
        }
        return Text;
    }
    if (Kind == TEXT("linked"))
    { return ck::Format_UE(TEXT("{} = linked:{}"), Path, Value); }
    if (Kind == TEXT("asset"))
    { return ck::Format_UE(TEXT("{} = asset:{}"), Path, Value); }
    if (Kind == TEXT("dataInterface"))
    { return ck::Format_UE(TEXT("{} = DI<{}>"), Path, Value); }
    return ck::Format_UE(TEXT("{} = {}:{}"), Path, Kind, Value);
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
        if (Pin->PinName == NAME_None || ck::asset_exporter::niagara::IsParameterMapPin(Pin))
        { continue; }

        const auto Name = Pin->PinName.ToString();
        if (Name.IsEmpty())
        { continue; }

        auto Value = FString{};
        if (Pin->LinkedTo.Num() > 0)
        {
            const auto* Source = Pin->LinkedTo[0] != nullptr ? Pin->LinkedTo[0]->GetOwningNode() : nullptr;
            if (const auto* DynInput = Cast<UNiagaraNodeFunctionCall>(Source))
            { Value = ck::Format_UE(TEXT("[dynamic input: {}]"), DynInput->GetNodeTitle(ENodeTitleType::ListView).ToString()); }
            else
            { Value = TEXT("<linked>"); }
        }
        else
        { Value = ck::asset_exporter::niagara::FormatPinDefault(Pin); }
        Inputs.Add({ Name, Value });
    }
    return Inputs;
}

auto
    FCk_NiagaraExporter::
    DoCollectMaterials(
        const UNiagaraSystem* InSystem)
    -> TArray<FString>
{
    auto Materials = TSet<FString>{};
    auto* System = const_cast<UNiagaraSystem*>(InSystem);

    for (const auto& Handle : System->GetEmitterHandles())
    {
        const auto* Data = Handle.GetEmitterData();
        if (Data == nullptr)
        { continue; }

        for (const auto* Renderer : Data->GetRenderers())
        {
            if (const auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
            {
                if (Sprite->Material != nullptr)
                { Materials.Add(Sprite->Material->GetPathName()); }
            }
            else if (const auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
            {
                if (Ribbon->Material != nullptr)
                { Materials.Add(Ribbon->Material->GetPathName()); }
            }
            else if (const auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
            {
                if (Mesh->bOverrideMaterials)
                {
                    for (const auto& Override : Mesh->OverrideMaterials)
                    {
                        if (Override.ExplicitMat != nullptr)
                        { Materials.Add(Override.ExplicitMat->GetPathName()); }
                    }
                }
                for (const auto& MeshEntry : Mesh->Meshes)
                {
                    if (MeshEntry.Mesh == nullptr)
                    { continue; }
                    for (const auto& StaticMaterial : MeshEntry.Mesh->GetStaticMaterials())
                    {
                        if (StaticMaterial.MaterialInterface != nullptr)
                        { Materials.Add(StaticMaterial.MaterialInterface->GetPathName()); }
                    }
                }
            }
        }
    }
    return Materials.Array();
}

auto
    FCk_NiagaraExporter::
    DoCollectMeshes(
        const UNiagaraSystem* InSystem)
    -> TArray<FString>
{
    auto Meshes = TSet<FString>{};
    auto* System = const_cast<UNiagaraSystem*>(InSystem);

    for (const auto& Handle : System->GetEmitterHandles())
    {
        const auto* Data = Handle.GetEmitterData();
        if (Data == nullptr)
        { continue; }

        for (const auto* Renderer : Data->GetRenderers())
        {
            const auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer);
            if (Mesh == nullptr)
            { continue; }

            for (const auto& MeshEntry : Mesh->Meshes)
            {
                if (MeshEntry.Mesh != nullptr)
                { Meshes.Add(MeshEntry.Mesh->GetPathName()); }
            }
        }
    }
    return Meshes.Array();
}

// --------------------------------------------------------------------------------------------------------------------

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

    // ---- System-level user parameters (with authored default values) ----
    Out += TEXT("USER PARAMETERS\n");
    const auto& Exposed = System->GetExposedParameters();
    for (const auto& Var : Exposed.ReadParameterVariables())
    {
        auto Value = FString{};
        if (Var.IsDataInterface())
        {
            const auto* DataInterface = Exposed.GetDataInterface(Var.Offset);
            Value = DataInterface != nullptr ? ck::Format_UE(TEXT("DI<{}>"), DataInterface->GetClass()->GetName()) : FString{};
        }
        else if (const auto Formatted = ck::asset_exporter::niagara::FormatStoreValue(Exposed, Var); Formatted.IsSet())
        { Value = Formatted.GetValue(); }

        if (Value.IsEmpty())
        { Out += ck::Format_UE(TEXT("  - {} : {}\n"), Var.GetName().ToString(), Var.GetType().GetName()); }
        else
        { Out += ck::Format_UE(TEXT("  - {} : {} = {}\n"), Var.GetName().ToString(), Var.GetType().GetName(), Value); }
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

        Out += ck::Format_UE(TEXT("  Sim: {}   LocalSpace: {}   Determinism: {}{}\n"),
            ck::asset_exporter::niagara::SimTargetToString(Data->SimTarget),
            Data->bLocalSpace ? TEXT("true") : TEXT("false"),
            Data->bDeterminism ? TEXT("true") : TEXT("false"),
            Data->bDeterminism ? *FString::Printf(TEXT(" (seed %d)"), Data->RandomSeed) : TEXT(""));

        Out += ck::Format_UE(TEXT("  Bounds: {}{}\n"),
            StaticEnum<ENiagaraEmitterCalculateBoundMode>()->GetNameStringByValue(static_cast<int64>(Data->CalculateBoundsMode)),
            Data->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed
                ? *ck::Format_UE(TEXT("  {} to {}"), Data->FixedBounds.Min.ToString(), Data->FixedBounds.Max.ToString())
                : TEXT(""));

        for (const auto& Stage : ck::asset_exporter::niagara::Get_Stages_InEditorDisplayOrder())
        {
            UNiagaraScript* Script = (Data->*(Stage.Member)).Script;
            auto Modules = TArray<UNiagaraNodeFunctionCall*>{};
            auto OverrideNodes = TArray<UEdGraphNode*>{};
            if (NOT DoWalkStack(Script, Modules, OverrideNodes) || Modules.Num() == 0)
            { continue; }

            const auto Overrides = DoHarvestOverrides(OverrideNodes);
            auto ConsumedKeys = TSet<FString>{};

            Out += ck::Format_UE(TEXT("  {} ({} modules):\n"), FString(Stage.Name), FString::FromInt(Modules.Num()));
            auto Index = int32{0};
            for (const auto* Module : Modules)
            {
                if (Module == nullptr) { continue; }
                Out += ck::Format_UE(TEXT("    {}. {}{}\n"), FString::FromInt(++Index),
                    Module->GetNodeTitle(ENodeTitleType::ListView).ToString(),
                    Module->IsNodeEnabled() ? TEXT("") : TEXT("  (DISABLED)"));
                for (const auto& Input : DoGetModuleInputs(Module))
                {
                    if (Input.Value.IsEmpty()) { continue; }
                    Out += ck::Format_UE(TEXT("         {} = {}\n"), Input.Key, Input.Value);
                }

                auto ModuleOverrides = TArray<TSharedPtr<FJsonObject>>{};
                constexpr auto MaintainOrder = true;
                Overrides.MultiFind(Module->GetFunctionName(), ModuleOverrides, MaintainOrder);
                ConsumedKeys.Add(Module->GetFunctionName());
                DoExpandDynamicInputOverrides(Overrides, ModuleOverrides, ConsumedKeys);
                for (const auto& Override : ModuleOverrides)
                {
                    Out += ck::Format_UE(TEXT("         [override] {}\n"), DoFormatOverride_Text(Override));
                }
            }

            // Never drop data silently: overrides keyed to a node no module or dynamic input claimed.
            auto bWroteUnattachedHeader = false;
            for (auto It = Overrides.CreateConstIterator(); It; ++It)
            {
                if (ConsumedKeys.Contains(It.Key())) { continue; }
                if (NOT bWroteUnattachedHeader)
                { Out += TEXT("    [unattached overrides]\n"); bWroteUnattachedHeader = true; }
                Out += ck::Format_UE(TEXT("       {}: {}\n"), It.Key(), DoFormatOverride_Text(It.Value()));
            }

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
                    Out += ck::Format_UE(TEXT("  Material: {}  Alignment: {}  Facing: {}  Sort: {}"),
                        Sprite->Material ? Sprite->Material->GetPathName() : FString(TEXT("<none>")),
                        StaticEnum<ENiagaraSpriteAlignment>()->GetNameStringByValue((int64)Sprite->Alignment),
                        StaticEnum<ENiagaraSpriteFacingMode>()->GetNameStringByValue((int64)Sprite->FacingMode),
                        StaticEnum<ENiagaraSortMode>()->GetNameStringByValue((int64)Sprite->SortMode));
                    if (Sprite->SubImageSize != FVector2D(1.0, 1.0))
                    { Out += FString::Printf(TEXT("  SubUV: %dx%d"), (int32)Sprite->SubImageSize.X, (int32)Sprite->SubImageSize.Y); }
                }
                else if (const auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
                {
                    Out += ck::Format_UE(TEXT("  Material: {}"), Ribbon->Material ? Ribbon->Material->GetPathName() : FString(TEXT("<none>")));
                }
                else if (const auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
                {
                    Out += ck::Format_UE(TEXT("  Facing: {}  Sort: {}"),
                        StaticEnum<ENiagaraMeshFacingMode>()->GetNameStringByValue((int64)Mesh->FacingMode),
                        StaticEnum<ENiagaraSortMode>()->GetNameStringByValue((int64)Mesh->SortMode));
                    if (Mesh->SubImageSize != FVector2D(1.0, 1.0))
                    { Out += FString::Printf(TEXT("  SubUV: %dx%d"), (int32)Mesh->SubImageSize.X, (int32)Mesh->SubImageSize.Y); }
                    for (const auto& M : Mesh->Meshes)
                    {
                        Out += ck::Format_UE(TEXT("\n        mesh: {}"), M.Mesh ? M.Mesh->GetPathName() : FString(TEXT("<none>")));
                        if (NOT M.Scale.Equals(FVector::OneVector))
                        { Out += FString::Printf(TEXT("  scale: (%g, %g, %g)"), M.Scale.X, M.Scale.Y, M.Scale.Z); }
                    }
                    if (Mesh->bOverrideMaterials)
                    {
                        for (const auto& Ov : Mesh->OverrideMaterials)
                        { Out += ck::Format_UE(TEXT("\n        material: {}"), Ov.ExplicitMat ? Ov.ExplicitMat->GetPathName() : FString(TEXT("<none>"))); }
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

auto
    FCk_NiagaraExporter::
    DoSerializeModule_Json(
        const UNiagaraNodeFunctionCall* InModule,
        const TArray<TSharedPtr<FJsonObject>>& InOverrides)
    -> TSharedPtr<FJsonObject>
{
    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("name"), InModule->GetNodeTitle(ENodeTitleType::ListView).ToString());
    Obj->SetStringField(TEXT("id"), InModule->GetFunctionName());
    if (NOT InModule->IsNodeEnabled())
    { Obj->SetBoolField(TEXT("enabled"), false); }

    auto InputsArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Input : DoGetModuleInputs(InModule))
    {
        auto In = MakeShared<FJsonObject>();
        In->SetStringField(TEXT("name"), Input.Key);
        In->SetStringField(TEXT("value"), Input.Value);
        InputsArr.Add(MakeShared<FJsonValueObject>(In));
    }
    Obj->SetArrayField(TEXT("inputs"), InputsArr);

    if (InOverrides.Num() > 0)
    {
        auto OverridesArr = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& Override : InOverrides)
        { OverridesArr.Add(MakeShared<FJsonValueObject>(Override)); }
        Obj->SetArrayField(TEXT("overrides"), OverridesArr);
    }
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
    auto OverrideNodes = TArray<UEdGraphNode*>{};
    if (NOT DoWalkStack(InScript, Modules, OverrideNodes) || Modules.Num() == 0)
    { return nullptr; }

    const auto Overrides = DoHarvestOverrides(OverrideNodes);
    auto ConsumedKeys = TSet<FString>{};

    auto Obj = MakeShared<FJsonObject>();
    Obj->SetStringField(TEXT("stage"), InStageName);
    auto Arr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Module : Modules)
    {
        if (Module == nullptr) { continue; }
        auto ModuleOverrides = TArray<TSharedPtr<FJsonObject>>{};
        constexpr auto MaintainOrder = true;
        Overrides.MultiFind(Module->GetFunctionName(), ModuleOverrides, MaintainOrder);
        ConsumedKeys.Add(Module->GetFunctionName());
        DoExpandDynamicInputOverrides(Overrides, ModuleOverrides, ConsumedKeys);
        Arr.Add(MakeShared<FJsonValueObject>(DoSerializeModule_Json(Module, ModuleOverrides)));
    }
    Obj->SetArrayField(TEXT("modules"), Arr);

    // Never drop data silently: overrides keyed to a node no module or dynamic input claimed.
    auto Unattached = TArray<TSharedPtr<FJsonValue>>{};
    for (auto It = Overrides.CreateConstIterator(); It; ++It)
    {
        if (ConsumedKeys.Contains(It.Key()))
        { continue; }
        auto Orphan = MakeShared<FJsonObject>();
        Orphan->SetStringField(TEXT("owner"), It.Key());
        Orphan->SetObjectField(TEXT("override"), It.Value());
        Unattached.Add(MakeShared<FJsonValueObject>(Orphan));
    }
    if (Unattached.Num() > 0)
    { Obj->SetArrayField(TEXT("unattachedOverrides"), Unattached); }

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
        Obj->SetStringField(TEXT("material"), Sprite->Material ? Sprite->Material->GetPathName() : FString(TEXT("<none>")));
        Obj->SetStringField(TEXT("alignment"), StaticEnum<ENiagaraSpriteAlignment>()->GetNameStringByValue((int64)Sprite->Alignment));
        Obj->SetStringField(TEXT("facing"), StaticEnum<ENiagaraSpriteFacingMode>()->GetNameStringByValue((int64)Sprite->FacingMode));
        Obj->SetStringField(TEXT("sortMode"), StaticEnum<ENiagaraSortMode>()->GetNameStringByValue((int64)Sprite->SortMode));
        if (Sprite->SubImageSize != FVector2D(1.0, 1.0))
        { Obj->SetStringField(TEXT("subImage"), FString::Printf(TEXT("%dx%d"), (int32)Sprite->SubImageSize.X, (int32)Sprite->SubImageSize.Y)); }
    }
    else if (const auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(InRenderer))
    {
        Obj->SetStringField(TEXT("material"), Ribbon->Material ? Ribbon->Material->GetPathName() : FString(TEXT("<none>")));
    }
    else if (const auto* Mesh = Cast<UNiagaraMeshRendererProperties>(InRenderer))
    {
        Obj->SetStringField(TEXT("facingMode"), StaticEnum<ENiagaraMeshFacingMode>()->GetNameStringByValue((int64)Mesh->FacingMode));
        Obj->SetStringField(TEXT("sortMode"), StaticEnum<ENiagaraSortMode>()->GetNameStringByValue((int64)Mesh->SortMode));
        auto MeshesArr = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& M : Mesh->Meshes)
        {
            auto MeshObj = MakeShared<FJsonObject>();
            MeshObj->SetStringField(TEXT("mesh"), M.Mesh ? M.Mesh->GetPathName() : FString(TEXT("<none>")));
            MeshObj->SetStringField(TEXT("scale"), FString::Printf(TEXT("(%g, %g, %g)"), M.Scale.X, M.Scale.Y, M.Scale.Z));
            MeshesArr.Add(MakeShared<FJsonValueObject>(MeshObj));
        }
        Obj->SetArrayField(TEXT("meshes"), MeshesArr);
        if (Mesh->bOverrideMaterials)
        {
            auto MatsArr = TArray<TSharedPtr<FJsonValue>>{};
            for (const auto& Ov : Mesh->OverrideMaterials)
            { MatsArr.Add(MakeShared<FJsonValueString>(Ov.ExplicitMat ? Ov.ExplicitMat->GetPathName() : FString(TEXT("<none>")))); }
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
    Obj->SetBoolField(TEXT("determinism"), InData->bDeterminism);
    if (InData->bDeterminism)
    { Obj->SetNumberField(TEXT("randomSeed"), InData->RandomSeed); }
    Obj->SetStringField(TEXT("boundsMode"),
        StaticEnum<ENiagaraEmitterCalculateBoundMode>()->GetNameStringByValue(static_cast<int64>(InData->CalculateBoundsMode)));
    if (InData->CalculateBoundsMode == ENiagaraEmitterCalculateBoundMode::Fixed)
    { Obj->SetStringField(TEXT("fixedBounds"), ck::Format_UE(TEXT("{} to {}"), InData->FixedBounds.Min.ToString(), InData->FixedBounds.Max.ToString())); }

    auto Stacks = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Stage : ck::asset_exporter::niagara::Get_Stages_InEditorDisplayOrder())
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
    Root->SetStringField(TEXT("packagePath"), System->GetOutermost()->GetName());

    auto Params = TArray<TSharedPtr<FJsonValue>>{};
    const auto& Exposed = System->GetExposedParameters();
    for (const auto& Var : Exposed.ReadParameterVariables())
    {
        auto P = MakeShared<FJsonObject>();
        P->SetStringField(TEXT("name"), Var.GetName().ToString());
        P->SetStringField(TEXT("type"), Var.GetType().GetName());
        if (Var.IsDataInterface())
        {
            if (const auto* DataInterface = Exposed.GetDataInterface(Var.Offset))
            { P->SetStringField(TEXT("value"), ck::Format_UE(TEXT("DI<{}>"), DataInterface->GetClass()->GetName())); }
        }
        else if (const auto Formatted = ck::asset_exporter::niagara::FormatStoreValue(Exposed, Var); Formatted.IsSet())
        { P->SetStringField(TEXT("value"), Formatted.GetValue()); }
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
        const FString& InExtension,
        const FString& InOutputDir)
    -> FString
{
    if (NOT InOutputDir.IsEmpty())
    { return FPaths::Combine(InOutputDir, InSystem->GetName() + InExtension); }

    const auto PackageName = InSystem->GetOutermost()->GetName();
    auto FilePath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, FilePath, InExtension))
    { return FString{}; }
    return FilePath;
}
