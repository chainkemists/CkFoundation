#include "CkParticlesEditor/Generator/CkParticles_TemplateBuilder.h"

#include "CkParticlesEditor_Log.h"

#include "CkParticles/DataInterface/CkParticles_DataInterface.h"

#include "CkCore/Validation/CkIsValid.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraTypes.h"
#include "NiagaraEditorUtilities.h"

#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOutput.h"
#include "EdGraphSchema_Niagara.h"
#include "EdGraph/EdGraph.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeWithDynamicPins.h"
#include "NiagaraDataInterface.h"

#include "NiagaraSpriteRendererProperties.h"

#include "CkParticlesEditor/Generator/CkParticles_TextureGenerator.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Engine/Texture2D.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "SceneTypes.h"
#include "MaterialTypes.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------
// Builds the SCAFFOLD of the template Niagara System from C++: a GPU emitter (default spawn/init modules + sprite
// renderer), a User.BehaviorId int, and the DI wired as a User.ParticleScript parameter.
//
// The actual behavior-call MODULE (Particle Update calling ParticleScript.ExecuteStage and writing
// Position/Velocity/Color) is NOT built here: it requires NiagaraEditor internals that Epic does not export
// (FNiagaraStackGraphUtilities::SetCustomExpressionForFunctionInput, UNiagaraGraph::FindOutputNode,
// UNiagaraNodeCustomHlsl::VirtualIncludeFilePaths are all non-public to external modules). That one module is
// added once in-editor (see CkParticles/Claude.md). The DI param is already in place so it's a 1-field hookup.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor
{
    namespace TemplateBuilderLocal
    {
        static const TCHAR* PkgPath   = TEXT("/CkFoundation/CkParticles/Templates/PS_CkParticles_Template");
        static const TCHAR* AssetName = TEXT("PS_CkParticles_Template");

        // Code-builds + saves the VFX master material: samples a baked CkParticles texture (the shape/detail) via a
        // "BaseTexture" parameter, tints it by the per-particle Color the behaviors write, additive + unlit. The
        // texture parameter lets per-effect material instances swap the look later (glow/smoke/electric/...).
        // Returns nullptr on failure so the template falls back to the default sprite material. Pure UMaterial API.
        static auto Build_VfxMasterMaterial(UTexture2D* InBaseTexture) -> UMaterialInterface*
        {
            static const TCHAR* MatPkgPath = TEXT("/CkFoundation/CkParticles/Templates/M_CkParticles_VfxMaster");
            static const TCHAR* MatName    = TEXT("M_CkParticles_VfxMaster");

            UPackage* Package = FPackageName::DoesPackageExist(MatPkgPath)
                ? LoadPackage(nullptr, MatPkgPath, LOAD_None)
                : nullptr;
            if (Package == nullptr) { Package = CreatePackage(MatPkgPath); }
            if (Package == nullptr) { return nullptr; }

            if (auto* Old = StaticFindObject(UMaterial::StaticClass(), Package, MatName))
            {
                Old->ClearFlags(RF_Standalone | RF_Public);
                Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
            }

            auto* Mat = NewObject<UMaterial>(Package, MatName, RF_Public | RF_Standalone);
            if (Mat == nullptr) { return nullptr; }

            Mat->MaterialDomain = MD_Surface;
            Mat->BlendMode      = BLEND_Additive;
            Mat->SetShadingModel(MSM_Unlit);

            auto* TexSample = Cast<UMaterialExpressionTextureSampleParameter2D>(
                UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -650, 0));
            auto* PColor = Cast<UMaterialExpressionParticleColor>(
                UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionParticleColor::StaticClass(), -650, 280));
            auto* MulRgb = Cast<UMaterialExpressionMultiply>(
                UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -320, 120));

            if (TexSample == nullptr || PColor == nullptr || MulRgb == nullptr)
            { return nullptr; }

            // Auto-pick the sampler type so it matches our linear (SRGB=false) textures — a mismatch fails compilation.
            TexSample->ParameterName = TEXT("BaseTexture");
            if (InBaseTexture != nullptr)
            {
                TexSample->Texture     = InBaseTexture;
                TexSample->SamplerType = UMaterialExpressionTextureBase::GetSamplerTypeForTexture(InBaseTexture);
            }

            // Emissive = texture (RGBA, default output) x ParticleColor ; Opacity = texture alpha (for translucent
            // variants — additive ignores it). Default "" outputs avoid pin-name guessing on the colour chain.
            UMaterialEditingLibrary::ConnectMaterialExpressions(TexSample, TEXT(""),  MulRgb, TEXT("A"));
            UMaterialEditingLibrary::ConnectMaterialExpressions(PColor,    TEXT(""),  MulRgb, TEXT("B"));
            UMaterialEditingLibrary::ConnectMaterialProperty(MulRgb,    TEXT(""),  MP_EmissiveColor);
            UMaterialEditingLibrary::ConnectMaterialProperty(TexSample, TEXT("A"), MP_Opacity);

            UMaterialEditingLibrary::LayoutMaterialExpressions(Mat);
            UMaterialEditingLibrary::RecompileMaterial(Mat);

            Mat->MarkPackageDirty();
            FAssetRegistryModule::AssetCreated(Mat);

            const auto FileName = FPackageName::LongPackageNameToFilename(MatPkgPath, FPackageName::GetAssetPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            UPackage::SavePackage(Package, Mat, *FileName, SaveArgs);

            return Mat;
        }

        // Builds one Material Instance per baked texture (parented to the VFX master, overriding BaseTexture), saved
        // under /CkFoundation/CkParticles/Materials/. A spawn picks one per effect via the sprite renderer's
        // user-param binding (UNiagaraComponent::SetVariableMaterial). Null-tolerant.
        static auto Build_TextureMaterialInstances(UMaterialInterface* InMaster) -> void
        {
            if (InMaster == nullptr) { return; }

            static const TCHAR* TexNames[] = { TEXT("Glow"), TEXT("Flare"), TEXT("Smoke"), TEXT("Electric"), TEXT("Streak"), TEXT("Ring") };
            for (const TCHAR* Tex : TexNames)
            {
                const FString MicName    = FString::Printf(TEXT("MI_CkParticles_%s"), Tex);
                const FString MicPkgPath = FString::Printf(TEXT("/CkFoundation/CkParticles/Materials/%s"), *MicName);

                UPackage* Package = FPackageName::DoesPackageExist(MicPkgPath) ? LoadPackage(nullptr, *MicPkgPath, LOAD_None) : nullptr;
                if (Package == nullptr) { Package = CreatePackage(*MicPkgPath); }
                if (Package == nullptr) { continue; }

                if (auto* Old = StaticFindObject(UMaterialInstanceConstant::StaticClass(), Package, *MicName))
                {
                    Old->ClearFlags(RF_Standalone | RF_Public);
                    Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
                }

                auto* Mic = NewObject<UMaterialInstanceConstant>(Package, *MicName, RF_Public | RF_Standalone);
                if (Mic == nullptr) { continue; }

                Mic->SetParentEditorOnly(InMaster);
                if (auto* Tx = LoadObject<UTexture>(nullptr, *ck::particles::Get_VfxTextureObjectPath(FName(Tex))))
                { Mic->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(TEXT("BaseTexture")), Tx); }
                Mic->PostEditChange();

                Mic->MarkPackageDirty();
                FAssetRegistryModule::AssetCreated(Mic);

                const auto FileName = FPackageName::LongPackageNameToFilename(MicPkgPath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(Package, Mic, *FileName, SaveArgs);
            }
        }

#if CK_WITH_PARTICLES
        // Construct a Private dynamic-pin node type (Map Get / Map Set) via reflection — its header isn't public,
        // so we resolve the UClass by path, NewObject as the public base UNiagaraNodeWithDynamicPins, and add it
        // to the graph manually (FGraphNodeCreator<T> needs the compile-time type).
        static auto Create_DynamicPinNode(
            UNiagaraGraph* InGraph,
            const TCHAR*   InClassPath) -> UNiagaraNodeWithDynamicPins*
        {
            UClass* NodeClass = FindObject<UClass>(nullptr, InClassPath);
            if (NodeClass == nullptr)
            { NodeClass = StaticLoadClass(UNiagaraNodeWithDynamicPins::StaticClass(), nullptr, InClassPath); }
            if (NodeClass == nullptr)
            { return nullptr; }

            auto* Node = NewObject<UNiagaraNodeWithDynamicPins>(InGraph, NodeClass, NAME_None, RF_Transactional);
            InGraph->AddNode(Node, false, false);
            Node->CreateNewGuid();
            Node->PostPlacedNewNode();
            Node->AllocateDefaultPins();
            return Node;
        }

        static auto Find_ParameterMapPin(
            UEdGraphNode*                 InNode,
            EEdGraphPinDirection          InDirection,
            const UEdGraphSchema_Niagara* InSchema) -> UEdGraphPin*
        {
            for (UEdGraphPin* Pin : InNode->Pins)
            {
                if (Pin->Direction == InDirection &&
                    InSchema->PinToTypeDefinition(Pin) == FNiagaraTypeDefinition::GetParameterMapDef())
                { return Pin; }
            }
            return nullptr;
        }

        static auto Find_PinByName(
            UEdGraphNode*        InNode,
            FName                InName,
            EEdGraphPinDirection InDirection) -> UEdGraphPin*
        {
            for (UEdGraphPin* Pin : InNode->Pins)
            {
                if (Pin->Direction == InDirection && Pin->PinName == InName)
                { return Pin; }
            }
            return nullptr;
        }

        // Builds the per-particle behavior module ENTIRELY in C++ (no hand-authoring): a Module-usage script whose
        // graph is Input -> Map Get (reads DI + params) -> ExecuteStage (DI member fn) -> Map Set (Particles.*) ->
        // Output. Requires the forked-engine pin-authoring export (gated by CK_WITH_PARTICLES). Mirrors the
        // hand-authored scratch module; the Particles.Position (LWC) vs DI Vec3 difference is auto-bridged by
        // TryCreateConnection (same conversion the in-editor module used).
        static auto Build_BehaviorModuleScript(
            UObject* InOuter) -> UNiagaraScript*
        {
            auto* Script = NewObject<UNiagaraScript>(InOuter, TEXT("CkParticles_ApplyBehavior_Module"), RF_Transactional);
            Script->SetUsage(ENiagaraScriptUsage::Module);

            auto* Source = NewObject<UNiagaraScriptSource>(Script, NAME_None, RF_Transactional);
            auto* Graph  = NewObject<UNiagaraGraph>(Source, NAME_None, RF_Transactional);
            Source->NodeGraph = Graph;

            const auto* Schema = Cast<UEdGraphSchema_Niagara>(Graph->GetSchema());
            if (Schema == nullptr)
            { return nullptr; }

            // ---- Input / Output parameter-map nodes ----
            FGraphNodeCreator<UNiagaraNodeOutput> OutputCreator(*Graph);
            auto* OutputNode = OutputCreator.CreateNode();
            OutputNode->SetUsage(ENiagaraScriptUsage::Module);
            OutputNode->Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("Output")));
            OutputCreator.Finalize();

            FGraphNodeCreator<UNiagaraNodeInput> InputCreator(*Graph);
            auto* InputNode = InputCreator.CreateNode();
            InputNode->Usage = ENiagaraInputNodeUsage::Parameter;
            InputNode->Input = FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("MapIn"));
            InputCreator.Finalize();

            // ---- Map Get / Map Set (Private node types via reflection) ----
            auto* MapGet = Create_DynamicPinNode(Graph, TEXT("/Script/NiagaraEditor.NiagaraNodeParameterMapGet"));
            auto* MapSet = Create_DynamicPinNode(Graph, TEXT("/Script/NiagaraEditor.NiagaraNodeParameterMapSet"));
            if (MapGet == nullptr || MapSet == nullptr)
            { return nullptr; }

            // ---- ExecuteStage DI function-call node (Signature-based member call, no FunctionScript) ----
            auto* DiCdo = CastChecked<UNiagaraDataInterface>(UCkParticles_DataInterface::StaticClass()->GetDefaultObject());
            TArray<FNiagaraFunctionSignature> DiSignatures;
            DiCdo->GetFunctionSignatures(DiSignatures);
            const auto* ExecSig = DiSignatures.FindByPredicate(
                [](const FNiagaraFunctionSignature& InSig){ return InSig.Name == TEXT("ExecuteStage"); });
            if (ExecSig == nullptr)
            { return nullptr; }

            FGraphNodeCreator<UNiagaraNodeFunctionCall> FuncCreator(*Graph);
            auto* FuncNode = FuncCreator.CreateNode();
            FuncNode->FunctionScript = nullptr;
            FuncNode->Signature = *ExecSig;
            FuncNode->Signature.bMemberFunction = true;
            FuncCreator.Finalize();

            // ---- Read pins on Map Get (outputs = values read FROM the map) ----
            const FNiagaraTypeDefinition DiType(UCkParticles_DataInterface::StaticClass());
            UEdGraphPin* GetScript   = MapGet->RequestNewTypedPin(EGPD_Output, DiType,                                  TEXT("User.ParticleScript"));
            UEdGraphPin* GetBehavior = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetIntDef(),      TEXT("User.BehaviorId"));
            UEdGraphPin* GetDelta    = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Engine.DeltaTime"));
            UEdGraphPin* GetAge      = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Particles.Age"));
            UEdGraphPin* GetLifetime = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Particles.Lifetime"));
            UEdGraphPin* GetPosition = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetPositionDef(), TEXT("Particles.Position"));
            UEdGraphPin* GetVelocity = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.Velocity"));
            UEdGraphPin* GetSeed     = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetIntDef(),      TEXT("Particles.UniqueID"));

            // ---- Write pins on Map Set (inputs = values written TO the map) ----
            UEdGraphPin* SetPosition = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetPositionDef(), TEXT("Particles.Position"));
            UEdGraphPin* SetVelocity = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.Velocity"));
            UEdGraphPin* SetColor    = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetColorDef(),    TEXT("Particles.Color"));
            UEdGraphPin* SetSize     = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec2Def(),     TEXT("Particles.SpriteSize"));

            // Null-tolerant connect: if a pin lookup misses (e.g. a signature name changed), skip rather than crash.
            const auto Wire = [Schema](UEdGraphPin* InFrom, UEdGraphPin* InTo)
            {
                if (InFrom != nullptr && InTo != nullptr)
                { Schema->TryCreateConnection(InFrom, InTo); }
            };

            // ---- Thread the parameter map (matches Engine's NiagaraScriptFactory module skeleton) ----
            // The Input map FANS OUT to both Map Get's and Map Set's Source pins; it threads to Output
            // *through Map Set* (Source -> Dest). Map Get has NO map output pin — it only taps the map to
            // read values, so the old MapGet-output -> MapSet-Source wire connected nothing and severed the chain.
            Wire(InputNode->GetOutputPin(0),                        Find_ParameterMapPin(MapGet, EGPD_Input,  Schema));
            Wire(InputNode->GetOutputPin(0),                        Find_ParameterMapPin(MapSet, EGPD_Input,  Schema));
            Wire(Find_ParameterMapPin(MapSet, EGPD_Output, Schema), OutputNode->GetInputPin(0));

            // ---- Wire Map Get reads -> ExecuteStage inputs (by signature pin name) ----
            Wire(GetScript,   Find_PinByName(FuncNode, TEXT("ParticleScript"), EGPD_Input));
            Wire(GetBehavior, Find_PinByName(FuncNode, TEXT("BehaviorId"),     EGPD_Input));
            Wire(GetDelta,    Find_PinByName(FuncNode, TEXT("DeltaTime"),      EGPD_Input));
            Wire(GetAge,      Find_PinByName(FuncNode, TEXT("Age"),            EGPD_Input));
            Wire(GetLifetime, Find_PinByName(FuncNode, TEXT("Lifetime"),       EGPD_Input));
            Wire(GetPosition, Find_PinByName(FuncNode, TEXT("Position"),       EGPD_Input));
            Wire(GetVelocity, Find_PinByName(FuncNode, TEXT("Velocity"),       EGPD_Input));
            Wire(GetSeed,     Find_PinByName(FuncNode, TEXT("Seed"),           EGPD_Input));

            // ---- Wire ExecuteStage outputs -> Map Set writes ----
            Wire(Find_PinByName(FuncNode, TEXT("OutPosition"), EGPD_Output), SetPosition);
            Wire(Find_PinByName(FuncNode, TEXT("OutVelocity"), EGPD_Output), SetVelocity);
            Wire(Find_PinByName(FuncNode, TEXT("OutColor"),    EGPD_Output), SetColor);
            Wire(Find_PinByName(FuncNode, TEXT("OutSize"),     EGPD_Output), SetSize);

            Graph->NotifyGraphChanged();
            Script->SetLatestSource(Source);
            return Script;
        }

        static auto Try_AddCodeBuiltBehaviorModule(
            UNiagaraSystem* InSystem) -> bool
        {
            if (InSystem->GetEmitterHandles().Num() == 0)
            { return false; }

            const auto& Handle = InSystem->GetEmitterHandle(0);
            auto* EmitterData = Handle.GetEmitterData();
            if (EmitterData == nullptr)
            { return false; }

            UNiagaraScript* UpdateScript = EmitterData->UpdateScriptProps.Script;
            if (UpdateScript == nullptr)
            { return false; }

            auto* UpdateSource = Cast<UNiagaraScriptSource>(UpdateScript->GetLatestSource());
            if (UpdateSource == nullptr || UpdateSource->NodeGraph == nullptr)
            { return false; }

            auto* OutputNode = UpdateSource->NodeGraph->FindEquivalentOutputNode(
                ENiagaraScriptUsage::ParticleUpdateScript, UpdateScript->GetUsageId());
            if (OutputNode == nullptr)
            { return false; }

            auto* ModuleScript = Build_BehaviorModuleScript(InSystem);
            if (ModuleScript == nullptr)
            { return false; }

            auto* AddedModule = FNiagaraStackGraphUtilities::AddScriptModuleToStack(
                ModuleScript, *OutputNode, INDEX_NONE, TEXT("CkParticles Apply Behavior"));
            return AddedModule != nullptr;
        }
#endif // CK_WITH_PARTICLES
    }

    auto Build_TemplateSystem() -> UNiagaraSystem*
    {
        using namespace TemplateBuilderLocal;

        // ---- Package (idempotent refresh) ----
        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr) { Package = CreatePackage(PkgPath); }

        if (auto* Old = StaticFindObject(UNiagaraSystem::StaticClass(), Package, AssetName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
        }

        // ---- System + GPU emitter (default modules + sprite renderer come from InitializeEmitter) ----
        auto* System = NewObject<UNiagaraSystem>(Package, AssetName, RF_Public | RF_Standalone);
        UNiagaraSystemFactoryNew::InitializeSystem(System, /*bCreateDefaultNodes*/ true);

        auto* Emitter = NewObject<UNiagaraEmitter>(GetTransientPackage(), TEXT("CkParticles"), RF_Transactional);
        UNiagaraEmitterFactoryNew::InitializeEmitter(Emitter, /*bAddDefaultModulesAndRenderers*/ true);

        // Procedural VFX textures + the master material that samples the soft Glow texture (code-built; null-tolerant
        // — falls back to the default sprite material). Generating here keeps Create Template System self-contained.
        Generate_AllVfxTextures();
        auto* BaseTex = LoadObject<UTexture2D>(nullptr,
            TEXT("/CkFoundation/CkParticles/Textures/T_CkParticles_Glow.T_CkParticles_Glow"));
        auto* VfxMaterial = Build_VfxMasterMaterial(BaseTex);
        Build_TextureMaterialInstances(VfxMaterial); // one MI per texture; spawns swap per effect

        if (auto* EmitterData = Emitter->GetLatestEmitterData())
        {
            EmitterData->SimTarget = ENiagaraSimTarget::GPUComputeSim;

            // LOCAL space: behaviors write absolute positions (e.g. the swirl helix). In world space every spawned
            // system collapses onto world origin; in local space those positions are relative to the spawn component,
            // so each effect renders where it is spawned. Gravity (which integrates from the spawn position) works in
            // either, but local space keeps it consistent with the self-driving behaviors.
            EmitterData->bLocalSpace = true;

            // GPU emitters don't auto-compute bounds cheaply; without generous fixed bounds the whole system is frustum-
            // culled when its (tiny default) box leaves view. Cover the behavior extents + a few seconds of gravity fall.
            EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
            EmitterData->FixedBounds = FBox(FVector(-3000.0), FVector(3000.0));

            if (VfxMaterial != nullptr)
            {
                for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
                {
                    if (auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
                    {
                        Sprite->Material = VfxMaterial; // fallback if the user-param binding is unset/unresolved
                        Sprite->MaterialUserParamBinding.Parameter =
                            FNiagaraVariable(FNiagaraTypeDefinition::GetUMaterialDef(), ck::particles::Get_SpriteMaterialParameterName());
                    }
                }
            }
        }

        // Use the editor utility (exported) instead of the raw runtime UNiagaraSystem::AddEmitterHandle. The raw
        // call only appends to the EmitterHandles array; it does NOT build the system-script emitter nodes
        // (RebuildEmitterNodes -> simulation) or create the System Overview node
        // (SynchronizeOverviewGraphWithSystem -> visible emitter track). AddEmitterToSystem does all three, so the
        // emitter is both wired and visible. It copies the emitter into the System (bCreateCopy) and derives the
        // track name from the emitter's FName ("CkParticles").
        FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Emitter, FGuid(), /*bCreateCopy*/ true);

        // ---- User parameters: BehaviorId int (generator patches it) + the DI as ParticleScript ----
        auto& Exposed = System->GetExposedParameters();

        const FNiagaraVariable BehaviorVar(FNiagaraTypeDefinition::GetIntDef(), TEXT("User.BehaviorId"));
        Exposed.AddParameter(BehaviorVar);
        constexpr auto AddIfMissing = true;
        Exposed.SetParameterValue<int32>(0, BehaviorVar, AddIfMissing);

        const FNiagaraVariable DiVar(FNiagaraTypeDefinition(UCkParticles_DataInterface::StaticClass()), TEXT("User.ParticleScript"));
        Exposed.AddParameter(DiVar);
        Exposed.SetDataInterface(NewObject<UCkParticles_DataInterface>(System), DiVar);

        // Material the sprite renderer is bound to; spawns swap it per-effect via SetVariableMaterial. Default = the
        // master material (Glow), so an unset/failed override still renders the soft glow — never invisible.
        const FNiagaraVariable SpriteMatVar(FNiagaraTypeDefinition::GetUMaterialDef(), ck::particles::Get_SpriteMaterialParameterName());
        Exposed.AddParameter(SpriteMatVar);
        if (VfxMaterial != nullptr) { Exposed.SetUObject(VfxMaterial, SpriteMatVar); }

        // ---- Code-built behavior module (forked engine only; inert on stock via CK_WITH_PARTICLES) ----
#if CK_WITH_PARTICLES
        const auto bModuleAdded = TemplateBuilderLocal::Try_AddCodeBuiltBehaviorModule(System);
        ck::particles_editor::Log(TEXT("Code-built behavior module added to Particle Update: {}"),
            bModuleAdded ? FString(TEXT("YES")) : FString(TEXT("NO")));
#endif

        // ---- Compile (incl. GPU) + save ----
        constexpr auto ForceCompile = true;
        System->RequestCompile(ForceCompile);
        constexpr auto IncludingGpuShaders = true;
        constexpr auto ShowProgress = false;
        System->WaitForCompilationComplete(IncludingGpuShaders, ShowProgress);

        System->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(System);

        const auto FileName = FPackageName::LongPackageNameToFilename(PkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, System, *FileName, SaveArgs);

        ck::particles_editor::Log(
            TEXT("Built template SCAFFOLD [{}] (GPU emitter + sprite renderer + User.BehaviorId + User.ParticleScript). "
                 "Add the ExecuteStage call in Particle Update to finish it (see CkParticles/Claude.md)."),
            FString(AssetName));
        return System;
    }
}

// --------------------------------------------------------------------------------------------------------------------
